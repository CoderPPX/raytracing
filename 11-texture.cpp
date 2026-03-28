#include "camera.hpp"
#include "ppmshow.hpp"
#include "hittable.hpp"
#include "raytracing.hpp"

int main() {
	auto earth_texture = std::make_shared<image_texture>("image/earthmap.jpg");
	auto earth_surface = std::make_shared<lambertian>(earth_texture);
	auto globe = make_shared<sphere3d>(vec3(0, 0, 0), 2, earth_surface);
	image2d image(1280, 720);
	camera3d camera(image);
	camera.samples_per_pixel = 32;
	camera.fov = radians(90.0);
	camera.look_from = vec3(0, 0, 6);
	camera.look_at = vec3(0, 0, 0);
	camera.update();
	camera.render(globe);
	return ppmshow(image);
}