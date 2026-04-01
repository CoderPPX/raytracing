#include "camera.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "ppmshow.hpp"

int main() {
	auto world = std::make_shared<hittable_list>();

	// --- 材质 ---
	auto ground_mat = std::make_shared<lambertian>(vec3(0.48, 0.83, 0.53));
	auto white_mat = std::make_shared<lambertian>(vec3(0.73, 0.73, 0.73));
	auto light_mat = std::make_shared<diffuse_light>(vec3(7, 7, 7));
	auto glass_mat = std::make_shared<dielectric>(1.5);
	auto metal_mat = std::make_shared<metal>(vec3(0.8, 0.8, 0.9), 1.0);
	auto orange_mat = std::make_shared<lambertian>(vec3(0.7, 0.3, 0.1));

	// 1. 地面：用一个巨大的四边形代替原来的方块阵列（极大减少计算量）
	// 范围从 -1000 到 1000
	world->add(std::make_shared<quadrilateral>(vec3(-1000, 0, -1000), vec3(2000, 0, 0),
											   vec3(0, 0, 2000), ground_mat));

	// 2. 顶部光源
	world->add(std::make_shared<quadrilateral>(vec3(123, 554, 147), vec3(300, 0, 0),
											   vec3(0, 0, 265), light_mat));

	// 3. 基础几何体 (全部使用球体，因为你已经实现了它的 AABB 和 Hit)
	// 漫反射球
	world->add(std::make_shared<sphere3d>(vec3(400, 50, 200), 50, orange_mat));
	// 玻璃球
	world->add(std::make_shared<sphere3d>(vec3(260, 50, 45), 50, glass_mat));
	// 金属球
	world->add(std::make_shared<sphere3d>(vec3(0, 50, 145), 50, metal_mat));

	// 4. 随机散落的小球 (代替原来的 Instance 阵列，只放 10 个以保证速度)
	random_generator gen;
	for (int i = 0; i < 10; i++) {
		vec3 random_pos(gen.random_float(100, 500), 10, gen.random_float(100, 500));
		world->add(std::make_shared<sphere3d>(random_pos, 10, white_mat));
	}

	// --- 相机设置 ---
	image2d image(1280, 720);
	camera3d camera(image);

	camera.samples_per_pixel = 16;
	camera.background_color = vec3(0, 0, 0);
	camera.fov = radians(40.0);

	// 调整视角以观察这个简化场景
	camera.look_from = vec3(478, 278, -600);
	camera.look_at = vec3(278, 50, 0);
	camera.defocus_angle = 0;

	camera.update();

	// 直接传 world，不使用 linear_bvh
	camera.render(world);
	return ppmshow_and_write(image, "image/scene.png");
}