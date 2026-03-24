#include "ppmshow.hpp"
#include "raytracing.hpp"

int main() {
	// Image
	image2d image(256, 256);
	// Render
	for (int y = 0; y < image.height; ++y) {
		for (int x = 0; x < image.width; x++) {
			auto r = double(x) / (image.width - 1);
			auto g = double(y) / (image.height - 1);
			auto b = 0.0;
			int ir = int(255.999 * r);
			int ig = int(255.999 * g);
			int ib = int(255.999 * b);
			image.at(x, y) = {ir, ig, ib};
		}
	}
	return ppmshow(image);
}