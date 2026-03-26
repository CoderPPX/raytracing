#pragma once
#include <memory>
#include <concepts>
#include "aabb.hpp"

struct material;
struct hit_record {
	vec3 point;
	int front_facing; // 0 for false, 1 for true
	vec3 normal;
	float t;
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
		return true;
	}
	inline aabb bounding_box() const override { return bbox; };
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