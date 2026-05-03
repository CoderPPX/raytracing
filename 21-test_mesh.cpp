#include "ppmshow.hpp"
#include "yaml_loader.hpp"

int main() {
	try {
		auto scene = yaml_loader::load_from_file("scene.yaml");
		scene.camera->render(scene.world);
		return ppmshow_and_write(*scene.image, "image/scene.png");
	} catch (const std::exception &e) {
		fmt::print(stderr, "Error: {}\n", e.what());
		return -1;
	}
}
