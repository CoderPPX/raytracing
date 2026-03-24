#include "camera.hpp"
#include "ppmshow.hpp"
#include "hittable.hpp"
#include "raytracing.hpp"

int main() {
	image2d image(640, 360);
	camera3d camera(image);
	plane3d plane({0.0, -1.0, 0.0});
	sphere3d sphere({0.0, 0.0, -2.0}, 0.5);
	camera.render(plane, sphere);
	return ppmshow(image);
}