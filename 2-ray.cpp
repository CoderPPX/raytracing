#include "ppmshow.hpp"
#include "raytracing.hpp"

vec3 ray_color(const ray3d &r) {
	vec3 unit_direction = normalize(r.direction);
	float a = 0.5 * (unit_direction.y + 1.0);
	return mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), a);
}

int main() {
	// Image
	image2d image(256, 256);
	// Render
	const vec3 camera_center(0.0);
	const float fov = radians(90.0), focal_length = 0.1;
	const float viewport_width = 2.0 * focal_length * tan(fov / 2.0),
				viewport_height = viewport_width / image.aspect_ratio();
	const vec3 viewport_u(viewport_width, 0, 0);
	const vec3 viewport_v(0, -viewport_height, 0);
	const vec3 pixel_du = viewport_u / (float)image.width;
	const vec3 pixel_dv = viewport_v / (float)image.height;
	const vec3 viewport_upper_left =
		camera_center - vec3(0, 0, focal_length) - viewport_u / 2.f - viewport_v / 2.f;
	const vec3 pixel00_loc = viewport_upper_left + 0.5f * (pixel_du + pixel_dv);

	for (int y = 0; y < image.height; ++y) {
		for (int x = 0; x < image.width; x++) {
			auto pixel_center = pixel00_loc + (float)x * pixel_du + (float)y * pixel_dv;
			auto ray_direction = pixel_center - camera_center;
			ray3d r(camera_center, ray_direction);
			auto pixel_color = ray_color(r);
			image.write_vec3(x, y, pixel_color);
		}
	}
	return ppmshow(image);
}