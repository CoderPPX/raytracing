#pragma once
#include "random.hpp"
#include "hittable.hpp"

struct material {
	virtual bool scatter(const ray3d &ray_in, vec3 hit_normal, vec3 hit_point, vec3 &attenuation,
						 ray3d &scattered, random_generator &generator) const {
		return false;
	};
};

struct metal : public material {
	vec3 albedo{1.0, 1.0, 1.0};
	virtual bool scatter(const ray3d &ray_in, vec3 hit_normal, vec3 hit_point, vec3 &attenuation,
						 ray3d &scattered, random_generator &generator) const override {
		vec3 reflected = reflect(ray_in.direction, hit_normal);
		scattered = ray3d(hit_point, reflected);
		attenuation = albedo;
		return true;
	}
};

struct lambertian : public material {
	vec3 albedo;
	inline lambertian(vec3 color = vec3{1.0, 0.0, 0.0}) : albedo(color) {}
	virtual bool scatter(const ray3d &ray_in, vec3 hit_normal, vec3 hit_point, vec3 &attenuation,
						 ray3d &scattered, random_generator &generator) const override {
		vec3 scatter_direction = hit_normal + generator.random_unit_vec3();
		if (near_zero(scatter_direction)) {
			scatter_direction = hit_normal;
		}
		scattered = ray3d(hit_point, scatter_direction);
		attenuation = albedo;
		return true;
	}
};
