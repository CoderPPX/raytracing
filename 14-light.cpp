#include "camera.hpp"
#include "ppmshow.hpp"
#include "hittable.hpp"
#include "raytracing.hpp"

int main() {
	auto world = std::make_shared<hittable_list>();

	auto pertext = std::make_shared<noise_texture>(4);
	world->add(
		std::make_shared<sphere3d>(vec3(0, -1000, 0), 1000, std::make_shared<lambertian>(pertext)));
	world->add(std::make_shared<sphere3d>(vec3(0, 2, 0), 2, std::make_shared<lambertian>(pertext)));
	auto difflight = std::make_shared<diffuse_light>(vec3(1, 1, 1));
	world->add(make_shared<quadrilateral>(vec3(3, 1, -2), vec3(2, 0, 0), vec3(0, 2, 0),
										  difflight)); // Scene
	image2d image(1280, 720);
	camera3d camera(image);
	camera.fov = radians(90.0);
	camera.samples_per_pixel = 128;
	camera.look_at = vec3(0, 2, 0);
	camera.look_from = vec3(16, 3, 6);
	camera.defocus_angle = 0;
	camera.background_color = vec3(0.0);
	camera.update();
	camera.render(world);
	return ppmshow(image);
}