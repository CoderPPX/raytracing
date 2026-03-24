#include "camera.hpp"
#include "ppmshow.hpp"
#include "hittable.hpp"
#include "raytracing.hpp"

int main() {
	image2d image(640, 360);
	camera3d camera(image);
	auto material_ground = std::make_shared<lambertian>(vec3(0.8, 0.8, 0.0));
	auto material_center = std::make_shared<lambertian>(vec3(0.1, 0.2, 0.5));
	sphere3d sphere1({0.0, 0.0, -2.0}, 0.5, material_center);
	sphere3d sphere2({0.0, -20.6, -2.0}, 20.0, material_ground);
	camera.render(sphere1, sphere2);
	return ppmshow(image);
}