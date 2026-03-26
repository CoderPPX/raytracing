#include "camera.hpp"
#include "ppmshow.hpp"
#include "hittable.hpp"
#include "raytracing.hpp"

int main() {
	image2d image(1280, 720);
	random_generator generator;
	hittable_list<sphere3d> world;
	auto ground_material = std::make_shared<lambertian>(vec3(0.5, 0.5, 0.5));
	world.add(sphere3d(vec3(0, -1000, 0), 1000, ground_material));
	for (int a = -5; a < 5; a++) {
		for (int b = -5; b < 5; b++) {
			auto choose_mat = generator.random_float();
			vec3 center(a + 0.9 * generator.random_float(), 0.2,
						b + 0.9 * generator.random_float());
			if ((center - vec3(4, 0.2, 0)).length() > 0.9) {
				std::shared_ptr<material> sphere_material;
				if (choose_mat < 0.8) {
					// diffuse
					auto albedo = generator.random_color();
					auto center2 = center + vec3(0, generator.random_float(0, 0.5), 0);
					sphere_material = std::make_shared<lambertian>(albedo);
					world.add(sphere3d(center, center2, 0.2, sphere_material));
				} else if (choose_mat < 0.95) {
					// metal
					auto albedo = generator.random_color();
					float fuzz = generator.random_float(0, 0.5);
					sphere_material = std::make_shared<metal>(albedo, fuzz);
					world.add(sphere3d(center, 0.2, sphere_material));
				} else {
					// glass
					sphere_material = std::make_shared<dielectric>(1.5);
					world.add(sphere3d(center, 0.2, sphere_material));
				}
			}
		}
	}
	auto material1 = std::make_shared<dielectric>(1.5);
	auto material3 = std::make_shared<metal>(vec3(0.7, 0.6, 0.5), 0.0);
	auto material2 = std::make_shared<lambertian>(vec3(0.4, 0.2, 0.1));
	world.add(sphere3d(vec3(0, 1, 0), 1.0, material1));
	world.add(sphere3d(vec3(-4, 1, 0), 1.0, material2));
	world.add(sphere3d(vec3(4, 1, 0), 1.0, material3));
	camera3d camera(image);
	camera.samples_per_pixel = 32;
	camera.fov = radians(90.0);
	camera.look_from = vec3(13, 2, 3);
	camera.look_at = vec3(0, 0, 0);
	camera.defocus_angle = radians(0.6);
	camera.focus_dist = 10.0;
	camera.update();
	camera.render(world);
	return ppmshow(image);
}