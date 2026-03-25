#pragma once
#include "random.hpp"
#include "hittable.hpp"

struct camera3d {
public:
	image2d &image;
	vec3 center;
	float fov, focal_length;
	int samples_per_pixel = 32;

protected:
	vec3 pixel00_loc, pixel_du, pixel_dv;
	// Assume n is normalized
	static inline vec3 normal_to_color(vec3 n) { return (1.f + n) / 2.f; }
	inline ray3d random_ray_sample(int x, int y, random_generator &generator) const {
		float dx = generator.random_float() - 0.5;
		float dy = generator.random_float() - 0.5;
		vec3 pixel_sample = pixel00_loc + ((x + dx) * pixel_du) + ((y + dy) * pixel_dv);
		return ray3d(center, pixel_sample - center);
	}
	template <hittable T1, typename... Ts>
	static vec3 ray_color(const ray3d &ray, random_generator &generator, int recursion_depth,
						  const T1 &t1, const Ts &...ts) {
		constexpr int max_recursion_depth = 32;
		hit_record record;
		if (recursion_depth >= max_recursion_depth) {
			return vec3(0.0);
		}
		if (hit(ray, interval(0.001, +INFINITY), record, t1, ts...)) {
			ray3d scattered;
			vec3 attenuation;
			if (record.mat->scatter(ray, record, attenuation, scattered, generator)) {
				return attenuation *
					   ray_color(scattered, generator, recursion_depth + 1, t1, ts...);
			}
			return vec3(0.0);
		}
		vec3 unit_direction = normalize(ray.direction);
		float a = 0.5 * (unit_direction.y + 1.0);
		return mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), a);
	}

public:
	inline camera3d(image2d &render_image, vec3 camera_center = vec3(0.0), float fov_deg = 90.0,
					float focal_len = 0.1)
		: image(render_image), center(camera_center), fov(radians(fov_deg)),
		  focal_length(focal_len) {
		update();
	}
	inline void update() {
		const float viewport_width = 2.0 * focal_length * tan(fov / 2.0),
					viewport_height = viewport_width / image.aspect_ratio();
		const vec3 viewport_u(viewport_width, 0, 0);
		const vec3 viewport_v(0, -viewport_height, 0);
		pixel_du = viewport_u / (float)image.width;
		pixel_dv = viewport_v / (float)image.height;
		const vec3 viewport_upper_left =
			center - vec3(0, 0, focal_length) - viewport_u / 2.f - viewport_v / 2.f;
		pixel00_loc = viewport_upper_left + 0.5f * (pixel_du + pixel_dv);
	}
	template <hittable T1, typename... Ts> inline void render(const T1 &t1, const Ts &...ts) {
#pragma omp parallel for collapse(2)
		for (int y = 0; y < image.height; ++y) {
			for (int x = 0; x < image.width; x++) {
				static thread_local random_generator generator;
				vec3 pixel_color(0.0);
				for (int i = 0; i < samples_per_pixel; ++i) {
					ray3d ray = random_ray_sample(x, y, generator);
					pixel_color += ray_color(ray, generator, 0, t1, ts...);
				}
				image.write_vec3(x, y, pixel_color / (float)samples_per_pixel);
			}
		}
	}
};