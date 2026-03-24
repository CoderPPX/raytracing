#pragma once
#include <random>
#include "raytracing.hpp"

struct random_generator {
	std::mt19937 gen;
	std::uniform_real_distribution<float> distribution{0.0, 1.0};

	inline random_generator() {
		std::random_device rd;
		gen.seed(rd());
	}
	inline float random_float() { return distribution(gen); }
	inline float random_float(float min_val, float max_val) {
		return min_val + (max_val - min_val) * random_float();
	}
	inline float random_float(interval i) { return i.min_val + i.size() * random_float(); }
	inline vec3 random_vec3() { return vec3(random_float(), random_float(), random_float()); }
	inline vec3 random_vec3(float min_val, float max_val) {
		return vec3(random_float(min_val, max_val), random_float(min_val, max_val),
					random_float(min_val, max_val));
	}
	inline vec3 random_unit_vec3() {
		// Archimedes's Hat-Box Theorem
		float z = random_float(-1.0, 1.0);
		float a = random_float(0.0, 2.0 * std::numbers::pi);
		float r = sqrt(1.0 - z * z);
		return vec3(r * cos(a), r * sin(a), z);
	}
	inline vec3 random_unit_vec3_hemisphere(vec3 normal) {
		// Archimedes's Hat-Box Theorem
		float z = random_float(-1.0, 1.0);
		float a = random_float(0.0, 2.0 * std::numbers::pi);
		float r = sqrt(1.0 - z * z);
		vec3 v(r * cos(a), r * sin(a), z);
		return dot(v, normal) > 0.0 ? v : -v;
	}
};
