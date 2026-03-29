#include "camera.hpp"
#include "ppmshow.hpp"
#include "hittable.hpp"
#include "raytracing.hpp"

int main() {
	auto world = std::make_shared<hittable_list>();
	// Materials
	auto left_red = std::make_shared<lambertian>(vec3(1.0, 0.2, 0.2));
	auto back_green = std::make_shared<lambertian>(vec3(0.2, 1.0, 0.2));
	auto right_blue = std::make_shared<lambertian>(vec3(0.2, 0.2, 1.0));
	auto upper_orange = std::make_shared<lambertian>(vec3(1.0, 0.5, 0.0));
	auto lower_teal = std::make_shared<lambertian>(vec3(0.2, 0.8, 0.8));
	// Quads
	world->add(
		std::make_shared<quadrilateral>(vec3(-3, -2, 5), vec3(0, 0, -4), vec3(0, 4, 0), left_red));
	world->add(
		std::make_shared<quadrilateral>(vec3(-2, -2, 0), vec3(4, 0, 0), vec3(0, 4, 0), back_green));
	world->add(
		std::make_shared<quadrilateral>(vec3(3, -2, 1), vec3(0, 0, 4), vec3(0, 4, 0), right_blue));
	world->add(std::make_shared<quadrilateral>(vec3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4),
											   upper_orange));
	world->add(std::make_shared<quadrilateral>(vec3(-2, -3, 5), vec3(4, 0, 0), vec3(0, 0, -4),
											   lower_teal));
	// Scene
	image2d image(1280, 720);
	camera3d camera(image);
	camera.fov = radians(90.0);
	camera.samples_per_pixel = 128;
	camera.look_at = vec3(0, 0, 0);
	camera.look_from = vec3(0, 0, 9);
	camera.defocus_angle = 0;
	camera.update();
	camera.render(world);
	return ppmshow(image);
}