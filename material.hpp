#pragma once
#include "random.hpp"
#include "texture.hpp"

struct hit_record;
struct material {
	virtual bool scatter(const ray3d &ray_in, const hit_record &record, vec3 &attenuation,
						 ray3d &scattered, random_generator &generator) const {
		return false;
	};
	virtual ~material() = default;
};

struct metal : public material {
	vec3 albedo;
	float fuzz_factor;
	inline metal(vec3 color = vec3{1.0, 0.0, 0.0}, float fuzz = 0.05)
		: albedo(color), fuzz_factor(fuzz) {}
	inline virtual bool scatter(const ray3d &ray_in, const hit_record &record, vec3 &attenuation,
								ray3d &scattered, random_generator &generator) const override {
		vec3 reflected = reflect(ray_in.direction, record.normal);
		reflected = normalize(reflected) + (fuzz_factor * generator.random_unit_vec3());
		scattered = ray3d(record.point, reflected, ray_in.time);
		attenuation = albedo;
		return dot(reflected, record.normal) > 0.0;
	}
};

struct lambertian : public material {
	texture_ptr tex;
	inline lambertian(vec3 color = vec3{1.0, 0.0, 0.0})
		: tex(std::make_shared<solid_color_texture>(color)) {}
	inline lambertian(texture_ptr tex) : tex(tex) {}
	inline virtual bool scatter(const ray3d &ray_in, const hit_record &record, vec3 &attenuation,
								ray3d &scattered, random_generator &generator) const override {
		vec3 scatter_direction = record.normal + generator.random_unit_vec3();
		if (near_zero(scatter_direction)) {
			scatter_direction = record.normal;
		}
		scattered = ray3d(record.point, scatter_direction, ray_in.time);
		attenuation = tex->color(record.tex_coord, record.point);
		return true;
	}
};

struct dielectric : public material {
	float refraction_index;
	inline dielectric(float refraction_index) : refraction_index(refraction_index) {}
	[[gnu::always_inline]] static float reflectance(float cosine, float refraction_index) {
		// Use Schlick's approximation for reflectance
		float r0 = (1 - refraction_index) / (1 + refraction_index);
		r0 = r0 * r0;
		return r0 + (1 - r0) * std::pow((1 - cosine), 5);
	}
	inline virtual bool scatter(const ray3d &ray_in, const hit_record &record, vec3 &attenuation,
								ray3d &scattered, random_generator &generator) const override {
		attenuation = vec3(1.0, 1.0, 1.0);
		float ri = record.front_facing ? (1.0 / refraction_index) : refraction_index;
		vec3 unit_direction = normalize(ray_in.direction);
		float cos_theta = min(dot(-unit_direction, record.normal), 1.0f);
		float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
		vec3 direction;
		if (ri * sin_theta > 1.0 || reflectance(cos_theta, ri) > generator.random_float()) {
			direction = reflect(unit_direction, record.normal);
		} else {
			direction = refract(unit_direction, record.normal, ri);
		}
		scattered = ray3d(record.point, direction, ray_in.time);
		return true;
	}
};
