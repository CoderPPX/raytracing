#pragma once
#include "raytracing.hpp"

struct aabb {
	union {
		struct {
			interval x, y, z;
		};
		interval xyz[3];
	};
	inline aabb(interval x = {}, interval y = {}, interval z = {}) : x(x), y(y), z(z) {
		pad_to_minimums();
	}
	inline aabb(vec3 a, vec3 b) {
		vec3 min_vec = min(a, b), max_vec = max(a, b);
		x = interval(min_vec.x, max_vec.x);
		y = interval(min_vec.y, max_vec.y);
		z = interval(min_vec.z, max_vec.z);
		pad_to_minimums();
	}
	inline aabb &union_(const aabb &other) {
		x = interval::union_(x, other.x);
		y = interval::union_(y, other.y);
		z = interval::union_(z, other.z);
		// result.pad_to_minimums();
		return *this;
	}
	inline static aabb union_(const aabb &a, const aabb &b) {
		aabb result;
		result.x = interval::union_(a.x, b.x);
		result.y = interval::union_(a.y, b.y);
		result.z = interval::union_(a.z, b.z);
		// result.pad_to_minimums();
		return result;
	}
	inline static aabb intersection(const aabb &a, const aabb &b) {
		aabb result;
		result.x = interval::intersection(a.x, b.x);
		result.y = interval::intersection(a.y, b.y);
		result.z = interval::intersection(a.z, b.z);
		// result.pad_to_minimums();
		return result;
	}
	inline int longest_axis() const {
		float max_length = max(x.size(), max(y.size(), z.size()));
		if (x.size() == max_length) {
			return 0;
		} else if (y.size() == max_length) {
			return 1;
		} else {
			return 2;
		}
	}
	inline bool hit(const ray3d &r, interval ray_t) const {
		for (int idx = 0; idx < 3; idx++) {
			float adinv = 1.0 / r.direction[idx];
			float t0 = (xyz[idx].min_val - r.origin[idx]) * adinv;
			float t1 = (xyz[idx].max_val - r.origin[idx]) * adinv;
			// Notes: TBD
			if (adinv < 0) {
				std::swap(t0, t1);
			}
			if (t0 > ray_t.min_val) {
				ray_t.min_val = t0;
			}
			if (t1 < ray_t.max_val) {
				ray_t.max_val = t1;
			}
			if (ray_t.max_val <= ray_t.min_val) {
				return false;
			}
		}
		return true;
	}
	inline bool hit(const ray3d &r, interval ray_t, float &hit_t) const {
		for (int idx = 0; idx < 3; idx++) {
			float adinv = 1.0 / r.direction[idx];
			float t0 = (xyz[idx].min_val - r.origin[idx]) * adinv;
			float t1 = (xyz[idx].max_val - r.origin[idx]) * adinv;
			// Notes: TBD
			if (adinv < 0) {
				std::swap(t0, t1);
			}
			if (t0 > ray_t.min_val) {
				ray_t.min_val = t0;
			}
			if (t1 < ray_t.max_val) {
				ray_t.max_val = t1;
			}
			if (ray_t.max_val <= ray_t.min_val) {
				return false;
			}
		}
		hit_t = ray_t.min_val;
		return true;
	}
	inline void pad_to_minimums() {
		constexpr float delta = 0.0001;
		if (x.size() < delta) {
			x.expand(delta);
		}
		if (y.size() < delta) {
			y.expand(delta);
		}
		if (z.size() < delta) {
			z.expand(delta);
		}
	}
	inline float surface_area() const {
		float sx = x.size(), sy = y.size(), sz = z.size();
		return 2.0 * (sx * sy + sy * sz + sz * sx);
	}
};