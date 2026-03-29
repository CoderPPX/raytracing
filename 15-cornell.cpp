#include "camera.hpp"
#include "ppmshow.hpp"
#include "hittable.hpp"
#include "raytracing.hpp"

int main() {
	// Materials
	auto red = std::make_shared<lambertian>(vec3(.65, .05, .05));
	auto white = std::make_shared<lambertian>(vec3(.73, .73, .73));
	auto green = std::make_shared<lambertian>(vec3(.12, .45, .15));
	auto light = std::make_shared<diffuse_light>(vec3(15, 15, 15));
	// Scene
	auto world = std::make_shared<hittable_list>();
	world->add(
		std::make_shared<quadrilateral>(vec3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	world->add(
		std::make_shared<quadrilateral>(vec3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world->add(std::make_shared<quadrilateral>(vec3(343, 554, 332), vec3(-130, 0, 0),
											   vec3(0, 0, -105), light));
	world->add(
		std::make_shared<quadrilateral>(vec3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world->add(std::make_shared<quadrilateral>(vec3(555, 555, 555), vec3(-555, 0, 0),
											   vec3(0, 0, -555), white));
	world->add(
		std::make_shared<quadrilateral>(vec3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));
	world->add(std::make_shared<box3d>(vec3(130, 0, 65), vec3(295, 165, 230), white));
	world->add(std::make_shared<box3d>(vec3(265, 0, 295), vec3(430, 330, 460), white));
	// Render
	image2d image(800, 800);
	camera3d camera(image);
	camera.fov = radians(40.0);
	camera.samples_per_pixel = 128;
	camera.look_at = vec3(278, 278, 0);
	camera.look_from = vec3(278, 278, -800);
	camera.defocus_angle = 0;
	camera.background_color = vec3(0.0);
	camera.update();
	camera.render(world);
	return ppmshow(image);
}