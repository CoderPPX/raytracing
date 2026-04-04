#pragma once
#include <atomic>
#include <thread>
#include "random.hpp"
#include "hittable.hpp"

struct camera3d {
public:
	image2d &image;
	vec3 center, up_vec{0.0, 1.0, 0.0};
	vec3 look_from{0.0, 0.0, 0.0}, look_at{0.0, 0.0, -1.0};
	float fov = radians(90.f);
	float defocus_angle = radians(0.0), focus_dist = 10.0;
	int samples_per_pixel = 16, max_depth = 32, sqrt_spp;
	float inv_sqrt_spp, pixel_samples_scale;
	vec3 background_color{0.70, 0.80, 1.00};

protected:
	vec3 u, v, w; // Camera frame basis vectors
	vec3 pixel00_loc, pixel_du, pixel_dv;
	vec3 defocus_disk_u, defocus_disk_v;

	// Assume n is normalized
	static inline vec3 normal_to_color(vec3 n) { return (1.f + n) / 2.f; }
	inline vec3 defocus_disk_sample(random_generator &generator) const {
		vec3 p = generator.random_in_unit_disk();
		return center + p[0] * defocus_disk_u + p[1] * defocus_disk_v;
	}
	inline ray3d random_ray_sample(int x, int y, random_generator &generator) const {
		float dx = generator.random_float() - 0.5;
		float dy = generator.random_float() - 0.5;
		vec3 pixel_sample = pixel00_loc + ((x + dx) * pixel_du) + ((y + dy) * pixel_dv);
		vec3 ray_origin = (defocus_angle <= 0.0) ? center : defocus_disk_sample(generator);
		return ray3d(ray_origin, pixel_sample - ray_origin);
	}
	inline ray3d random_ray_sample_stratified(int x, int y, int i, int j,
											  random_generator &generator) const {
		float dx = ((i + generator.random_float()) * inv_sqrt_spp) - 0.5;
		float dy = ((j + generator.random_float()) * inv_sqrt_spp) - 0.5;
		vec3 pixel_sample = pixel00_loc + ((x + dx) * pixel_du) + ((y + dy) * pixel_dv);
		vec3 ray_origin = (defocus_angle <= 0.0) ? center : defocus_disk_sample(generator);
		return ray3d(ray_origin, pixel_sample - ray_origin);
	}
	inline vec3 ray_color(const ray3d &ray, random_generator &generator, hittable_ptr object) {
		constexpr int max_depth = 16;
		ray3d current_ray = ray, scattered_ray;
		vec3 total_radiance(0.0), throughput(1.0), attenuation;
		for (int i = 0; i < max_depth; ++i) {
			hit_record record;
			// 使用 0.001 防止自相交 (Shadow Acne)
			if (!object->hit(current_ray, interval(0.001, 1e36), record, generator)) {
				// 没击中物体，返回背景色并结束
				total_radiance += throughput * background_color;
				break;
			}
			vec3 emitted = record.mat->emitted(record.tex_coord, record.point);
			// 1. 累加当前物体的自发光
			total_radiance += throughput * emitted;
			// 2. 尝试散射
			if (record.mat->scatter(current_ray, record, attenuation, scattered_ray, generator)) {
				throughput *= attenuation;
				current_ray = scattered_ray;
			} else {
				// 物体不散射（完全吸收），结束路径
				break;
			}
			// 3. 俄罗斯轮盘赌 (Russian Roulette)
			if (i > 5) {
				float p = max(throughput.x, max(throughput.y, throughput.z));
				if (generator.random_float() > p)
					break;
				throughput /= p;
			}
		}
		return total_radiance;
	}

public:
	inline camera3d(image2d &render_image) : image(render_image) { update(); }
	inline void update() {
		center = look_from;
		const float viewport_width = 2.0 * focus_dist * tan(fov / 2.0),
					viewport_height = viewport_width / image.aspect_ratio();
		w = normalize(look_from - look_at);
		u = normalize(cross(up_vec, w));
		v = normalize(cross(w, u));
		const vec3 viewport_u = viewport_width * u;
		const vec3 viewport_v = -viewport_height * v;
		pixel_du = viewport_u / (float)image.width;
		pixel_dv = viewport_v / (float)image.height;
		const vec3 viewport_upper_left =
			center - (focus_dist * w) - viewport_u / 2.f - viewport_v / 2.f;
		pixel00_loc = viewport_upper_left + 0.5f * (pixel_du + pixel_dv);
		float defocus_radius = focus_dist * tan(defocus_angle / 2.0);
		defocus_disk_u = u * defocus_radius;
		defocus_disk_v = v * defocus_radius;
	}
	inline void render(hittable_ptr object) {
		sqrt_spp = int(std::sqrt(samples_per_pixel));
		inv_sqrt_spp = 1.f / sqrt_spp;
		pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);
		std::atomic_bool render_done = false;
		std::atomic_int64_t pixel_counter = 0;
		std::thread display_thread([&] {
			const int64_t total_pixels = (int64_t)image.width * image.height;
			auto start_time = std::chrono::steady_clock::now();
			std::string display_string;
			display_string.reserve(200);
			while (!render_done) {
				int64_t count = pixel_counter.load(std::memory_order_relaxed);
				float progress = min(1.0f, (float)count / total_pixels);
				int bar_width = 40;
				int pos = (int)(bar_width * progress);
				auto now = std::chrono::steady_clock::now();
				auto elapsed =
					std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
				int eta = (progress > 0.01f) ? (int)(elapsed / progress - elapsed) : 0;
				display_string.clear();
				fmt::format_to(std::back_inserter(display_string), "\r\033[32mRendering\033[0m [");
				for (int i = 0; i < bar_width; ++i) {
					if (i < pos)
						display_string += '=';
					else if (i == pos)
						display_string += '>';
					else
						display_string += ' ';
				}
				fmt::format_to(std::back_inserter(display_string),
							   "] {:3d}% | ETA: {:d}s, Elapsed: {:d}s \033[K",
							   (int)(progress * 100), eta, elapsed);
				fmt::print("{}", display_string);
				std::fflush(stdout);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		});
#pragma omp parallel for collapse(2)
		for (int y = 0; y < image.height; ++y) {
			for (int x = 0; x < image.width; ++x) {
				static thread_local random_generator generator;
				vec3 pixel_color(0.0);
				for (int i = 0; i < sqrt_spp; ++i) {
					for (int j = 0; j < sqrt_spp; ++j) {
						ray3d ray = random_ray_sample_stratified(x, y, i, j, generator);
						pixel_color += ray_color(ray, generator, object);
					}
				}
				image.write_vec3(x, y, pixel_color * pixel_samples_scale);
				pixel_counter.fetch_add(1, std::memory_order_relaxed);
			}
		}
		render_done = true;
		display_thread.join();
		fmt::print("\n\033[32mFinished!\033[0m\n");
	}
};