#pragma once
#include <memory>
#include "aabb.hpp"

struct material;
struct hit_record {
	vec3 point;
	int front_facing; // 0 for false, 1 for true
	vec3 normal;
	float t;
	vec2 tex_coord;
	std::shared_ptr<material> mat;
	// Assume outward_normal to be a unit vector
	inline void set_face_normal(const ray3d &r, vec3 outward_normal) {
		front_facing = dot(r.direction, outward_normal) < 0;
		normal = front_facing ? outward_normal : -outward_normal;
	}
};

// Include here to avoid circular dependency
#include "material.hpp"

struct hittable {
	inline virtual aabb bounding_box() const = 0;
	inline virtual bool hit(const ray3d &r, interval t_interval, hit_record &rec) const = 0;
};
using hittable_ptr = std::shared_ptr<hittable>;

struct sphere3d : public hittable {
public:
	aabb bbox;
	ray3d center;
	float radius;
	std::shared_ptr<material> mat;

public:
	sphere3d(vec3 static_center, float r, std::shared_ptr<material> mat)
		: center(static_center, vec3(0.0)), radius(max(r, 0.f)), mat(mat) {
		vec3 rvec(radius, radius, radius);
		bbox = aabb(static_center - rvec, static_center + rvec);
	}
	sphere3d(vec3 center1, vec3 center2, float r, std::shared_ptr<material> mat)
		: center(center1, center2 - center1), radius(r), mat(mat) {
		vec3 rvec(radius, radius, radius);
		aabb box1(center1 - rvec, center1 + rvec);
		aabb box2(center2 - rvec, center2 + rvec);
		bbox = aabb(box1, box2);
	}
	// Asuume xyz is normalized
	static inline vec2 sphere_xyz_to_uv(vec3 xyz) {
		// p: a given point on the sphere of radius one, centered at the origin.
		// u: returned value [0,1] of angle around the Y axis from X=-1.
		// v: returned value [0,1] of angle from Y=-1 to Y=+1.
		//     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
		//     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
		//     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>
		auto theta = std::acos(-xyz.y);
		auto phi = std::atan2(-xyz.z, xyz.x) + M_PI;
		return vec2(phi / (2.0 * M_PI), theta / M_PI);
	}
	inline bool hit(const ray3d &r, interval t_interval, hit_record &rec) const override {
		vec3 current_center = center.at(r.time);
		vec3 oc = current_center - r.origin;
		float a = dot(r.direction, r.direction);
		float h = dot(r.direction, oc);
		float c = dot(oc, oc) - radius * radius;
		float delta = h * h - a * c;
		if (delta < 0) {
			return false;
		}
		float sqrtd = sqrt(delta), root = (h - sqrtd) / a;
		if (!t_interval.surrounds(root)) {
			root = (h + sqrtd) / a;
			if (!t_interval.surrounds(root)) {
				return false;
			}
		}
		rec.t = root;
		rec.point = r.at(root);
		vec3 outward_normal = normalize(rec.point - current_center);
		rec.set_face_normal(r, outward_normal);
		rec.mat = mat;
		rec.tex_coord = sphere_xyz_to_uv(outward_normal);
		return true;
	}
	inline aabb bounding_box() const override { return bbox; };
};

struct quadrilateral : public hittable {
public:
	aabb bbox;
	vec3 q, u, v; // starting point and two edges
	std::shared_ptr<material> mat;

protected:
	vec3 n, w;

public:
	inline quadrilateral(vec3 q_, vec3 u_, vec3 v_,
						 std::shared_ptr<material> mat_ = std::make_shared<lambertian>(vec3(0.8)))
		: q(q_), u(u_), v(v_), n(cross(u, v)), mat(mat_) {
		float len = length_square(n);
		w = n / len;
		n /= sqrt(len);
		vec3 pts[4] = {q, q + u, q + v, q + u + v};
		for (auto &pt : pts) {
			bbox.x.extend_with(pt.x);
			bbox.y.extend_with(pt.y);
			bbox.z.extend_with(pt.z);
		}
		bbox.pad_to_minimums();
	}
	inline aabb bounding_box() const override { return bbox; }
	inline bool hit(const ray3d &r, interval ray_t, hit_record &record) const override {
		float denom = dot(n, r.direction);
		// Notes: 忘记abs
		if (abs(denom) < 1e-8) {
			return false;
		}
		float t = dot(n, q - r.origin) / denom;
		if (!ray_t.contains(t)) {
			return false;
		}
		vec3 p = r.at(t), p0 = p - q;
		float t1 = dot(w, cross(p0, v)), t2 = dot(w, cross(u, p0));
		interval unit(0, 1);
		if (!unit.contains(t1) || !unit.contains(t2)) {
			return false;
		}
		record.t = t;
		record.point = p;
		record.mat = mat;
		record.tex_coord = vec2(t1, t2);
		// Notes: n必须是单位向量
		record.set_face_normal(r, n);
		return true;
	}
};

struct triangle : public hittable {
public:
	aabb bbox; // TBD
	vec3 v0, u, v, n, w;
	std::shared_ptr<material> mat;

public:
	inline triangle(vec3 p1, vec3 p2, vec3 p3,
					std::shared_ptr<material> mat_ = std::make_shared<lambertian>(vec3(0.8)))
		: v0(p1), u(p2 - p1), v(p3 - p1), n(cross(u, v)), mat(mat_) {
		float len = length_square(n);
		w = n / len;
		n /= sqrt(len);
	}
	inline aabb bounding_box() const override { return bbox; }
	inline bool hit(const ray3d &r, interval ray_t, hit_record &rec) const override {
		vec3 edge1 = u; // V1 - V0
		vec3 edge2 = v; // V2 - V0
		vec3 pvec = cross(r.direction, edge2);
		float det = dot(edge1, pvec);
		// 如果 det 接近 0，说明光线与三角形平面平行
		if (std::abs(det) < 1e-8)
			return false;
		float inv_det = 1.0f / det;
		vec3 tvec = r.origin - v0;
		float u_coord = dot(tvec, pvec) * inv_det;
		if (u_coord < 0 || u_coord > 1)
			return false;
		vec3 qvec = cross(tvec, edge1);
		float v_coord = dot(r.direction, qvec) * inv_det;
		if (v_coord < 0 || u_coord + v_coord > 1)
			return false;
		float t = dot(edge2, qvec) * inv_det;
		if (!ray_t.contains(t))
			return false;
		rec.t = t;
		rec.point = r.at(t);
		rec.set_face_normal(r, n); // n 依然建议预计算并归一化
		rec.mat = mat;
		rec.tex_coord = vec2(u_coord, v_coord);
		return true;
	}
};

struct hittable_list : public hittable {
public:
	aabb bbox;
	std::vector<hittable_ptr> objects;

public:
	hittable_list() = default;
	hittable_list(std::initializer_list<hittable_ptr> hittables) : objects(hittables) {
		for (auto &object : hittables) {
			bbox = aabb(bbox, object->bounding_box());
		}
	}
	inline void clear() { objects.clear(); }
	inline void add(hittable_ptr object) {
		objects.push_back(object);
		bbox = aabb(bbox, object->bounding_box());
	}
	inline bool hit(const ray3d &r, interval ray_t, hit_record &rec) const override {
		hit_record temp_rec;
		bool hit_anything = false;
		float closest_so_far = ray_t.max_val;
		for (const auto &object : objects) {
			if (object->hit(r, interval(ray_t.min_val, closest_so_far), temp_rec)) {
				hit_anything = true;
				closest_so_far = temp_rec.t;
				rec = temp_rec;
			}
		}
		return hit_anything;
	}
	inline aabb bounding_box() const override { return bbox; }
};

struct box3d : public hittable_list {
	inline box3d(vec3 v1, vec3 v2, std::shared_ptr<material> mat) {
		vec3 min_v = min(v1, v2), max_v = max(v1, v2);
		auto dx = vec3(max_v.x - min_v.x, 0, 0);
		auto dy = vec3(0, max_v.y - min_v.y, 0);
		auto dz = vec3(0, 0, max_v.z - min_v.z);
		this->add(
			make_shared<quadrilateral>(vec3(min_v.x, min_v.y, max_v.z), dx, dy, mat)); // front
		this->add(
			make_shared<quadrilateral>(vec3(max_v.x, min_v.y, max_v.z), -dz, dy, mat)); // right
		this->add(
			make_shared<quadrilateral>(vec3(max_v.x, min_v.y, min_v.z), -dx, dy, mat));		 // back
		this->add(make_shared<quadrilateral>(vec3(min_v.x, min_v.y, min_v.z), dz, dy, mat)); // left
		this->add(make_shared<quadrilateral>(vec3(min_v.x, max_v.y, max_v.z), dx, -dz, mat)); // top
		this->add(
			make_shared<quadrilateral>(vec3(min_v.x, min_v.y, min_v.z), dx, dz, mat)); // bottom
	}
};