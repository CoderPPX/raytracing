#include "camera.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "ppmshow.hpp"
#include "bvh.hpp"
#include "linear_bvh.hpp"

int main() {
	auto world = std::make_shared<hittable_list>();
	random_generator gen;
	// --- 材质定义 ---
	auto mat_ground = std::make_shared<lambertian>(vec3(0.5, 0.5, 0.5));
	// 1. 添加地面
	world->add(std::make_shared<sphere3d>(vec3(0.0, -1000.0, 0.0), 1000.0, mat_ground));
	// 2. 生成球体阵列 (10x10)
	int grid_size = 10;
	for (int i = -grid_size / 2; i < grid_size / 2; i++) {
		for (int j = -grid_size / 2; j < grid_size / 2; j++) {
			// 随机选择材质类型
			auto choose_mat = gen.random_float();
			vec3 center(i * 2.2 + gen.random_float(0, 0.5), 0.2,
						j * 2.2 + gen.random_float(0, 0.5));
			if (choose_mat < 0.7) {
				// 漫反射球 (随机颜色)
				auto albedo = gen.random_vec3() * gen.random_vec3();
				world->add(
					std::make_shared<sphere3d>(center, 0.2, std::make_shared<lambertian>(albedo)));
			} else if (choose_mat < 0.9) {
				// 金属球
				auto albedo = gen.random_vec3(0.5, 1);
				auto fuzz = gen.random_float(0, 0.5);
				world->add(
					std::make_shared<sphere3d>(center, 0.2, std::make_shared<metal>(albedo, fuzz)));
			} else {
				// 玻璃球
				world->add(
					std::make_shared<sphere3d>(center, 0.2, std::make_shared<dielectric>(1.5)));
			}
		}
	}
	// 传入 objects 内部的 std::vector<hittable_ptr>
	// 这会触发你的 bvh_node::build_bvh 逻辑
	auto world_bvh = std::make_shared<linear_bvh>(world->objects);
	// --- 相机设置 ---
	image2d image(1280, 720);
	camera3d camera(image);
	camera.samples_per_pixel = 128;				   // 预览建议 16-32，最终建议 500+
	camera.background_color = vec3(0.7, 0.8, 1.0); // 天蓝色背景
	camera.look_from = vec3(8);
	camera.look_at = vec3(0, 0, 0);
	camera.fov = radians(90.0);
	camera.defocus_angle = 0.0;
	camera.focus_dist = 13.0;
	camera.update();
	// fmt::print("[Non-BVH]\n");
	// camera.render(world);
	// image.save_to("image/open_scene.png");
	// fmt::print("[BVH]\n");
	camera.render(world_bvh);
	return ppmshow_and_write(image, "image/bvh_open_scene.png");
}