#include "camera.hpp"
#include "ppmshow.hpp"
#include "hittable.hpp"
#include "raytracing.hpp"

int main() {
	auto world = std::make_shared<hittable_list>();
	auto pertext = std::make_shared<noise_texture>(4.0);
	auto mat = std::make_shared<lambertian>(pertext);
	world->add(std::make_shared<sphere3d>(vec3(0, 2, 0), 2, mat));
	world->add(std::make_shared<sphere3d>(vec3(0, -1000, 0), 1000, mat));
	image2d image(1280, 720);
	camera3d camera(image);
	camera.fov = radians(90.0);
	camera.samples_per_pixel = 4;
	camera.look_from = vec3(13, 2, 3);
	camera.look_at = vec3(0, 0, 0);
	camera.defocus_angle = radians(0.6);
	camera.focus_dist = 10.0;
	camera.update();
	camera.render(world);
	return ppmshow(image);
}