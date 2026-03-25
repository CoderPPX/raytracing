#include "camera.hpp"
#include "ppmshow.hpp"
#include "hittable.hpp"
#include "raytracing.hpp"

int main() {
	image2d image(1280, 720);
	camera3d camera(image, vec3(0.0, 0.0, 2.0));
	auto material_ground = std::make_shared<lambertian>(vec3(0.8, 0.8, 0.0));
	auto material_center = std::make_shared<lambertian>(vec3(0.1, 0.2, 0.5));
	auto material_left = std::make_shared<dielectric>(1.0 / 1.33);
	auto material_right = std::make_shared<metal>(vec3(0.8, 0.6, 0.2), 0.1);
	sphere3d sphere1({0.0, 0.0, -2.0}, 0.5, material_center);
	sphere3d sphere2({0.0, -20.6, -2.0}, 20.0, material_ground);
	sphere3d sphere3(vec3(-1.0, 0.0, -1.0), 0.5, material_left);
	sphere3d sphere4(vec3(1.0, 0.0, -1.0), 0.5, material_right);
	camera.render(sphere1, sphere2, sphere3, sphere4);
	return ppmshow(image);
}