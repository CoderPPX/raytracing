#pragma once
#include <memory>
#include <concepts>
#include "raytracing.hpp"

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

template <typename T>
concept hittable = requires(T t, const ray3d &r, interval t_interval, hit_record &rec) {
	{ t.hit(r, t_interval, rec) } -> std::same_as<bool>;
};

struct sphere3d {
public:
	vec3 center;
	float radius;
	std::shared_ptr<material> mat;

public:
	sphere3d(vec3 c, float r, std::shared_ptr<material> mat) : center(c), radius(r), mat(mat) {}
	inline bool hit(const ray3d &r, interval t_interval, hit_record &rec) const {
		vec3 oc = center - r.origin;
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
		vec3 outward_normal = normalize(rec.point - center);
		rec.set_face_normal(r, outward_normal);
		rec.mat = mat;
		return true;
	}
};

struct plane3d {
public:
	vec3 center;
	vec3 normal;

public:
	plane3d(vec3 c, vec3 n = vec3(0.0, 1.0, 0.0)) : center(c), normal(n) {}
	inline bool hit(const ray3d &r, interval t_interval, hit_record &rec) const {
		float a = dot(normal, r.direction);
		if (a == 0.0) {
			return false;
		}
		float t = dot(normal, center - r.origin) / a;
		if (!t_interval.surrounds(t)) {
			return false;
		}
		rec.t = t;
		rec.point = r.at(t);
		rec.set_face_normal(r, normalize(normal));
		return true;
	}
};

template <hittable T> struct hittable_list {
	std::vector<T> objects;
	hittable_list() = default;
	hittable_list(std::initializer_list<T> hittables) : objects(hittables) {}
	inline void clear() { objects.clear(); }
	inline void add(const T &object) { objects.push_back(object); }
	inline void add(T &&object) { objects.push_back(std::move(object)); }
	inline bool hit(const ray3d &r, interval ray_t, hit_record &rec) const {
		hit_record temp_rec;
		bool hit_anything = false;
		float closest_so_far = ray_t.max_val;
		for (const auto &object : objects) {
			if (object.hit(r, interval(ray_t.min_val, closest_so_far), temp_rec)) {
				hit_anything = true;
				closest_so_far = temp_rec.t;
				rec = temp_rec;
			}
		}
		return hit_anything;
	}
};

inline bool hit(const ray3d &r, interval t_interval, hit_record &rec) { return false; }
template <hittable T1, typename... Ts>
inline bool hit(const ray3d &r, interval t_interval, hit_record &rec, const T1 &t1,
				const Ts &...ts) {
	bool hit_anything = false;
	float closest_so_far = t_interval.max_val;
	if (t1.hit(r, t_interval, rec)) {
		hit_anything = true;
		closest_so_far = rec.t;
	}
	if constexpr (sizeof...(ts) > 0) {
		if (hit(r, interval(t_interval.min_val, closest_so_far), rec, ts...)) {
			hit_anything = true;
		}
	}
	return hit_anything;
}