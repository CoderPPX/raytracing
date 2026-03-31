#include "camera.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "ppmshow.hpp"

int main() {
	auto world = std::make_shared<hittable_list>();

	// --- 材质定义 ---
	auto mat_ground = std::make_shared<lambertian>(vec3(0.5, 0.5, 0.5));
	auto mat_center = std::make_shared<lambertian>(vec3(0.1, 0.2, 0.5));
	auto mat_left = std::make_shared<dielectric>(1.5);					// 玻璃
	auto mat_right = std::make_shared<metal>(vec3(0.8, 0.6, 0.2), 0.0); // 抛光金

	// --- 物体放置 ---

	// 地面大球
	world->add(std::make_shared<sphere3d>(vec3(0.0, -1000.0, 0.0), 1000.0, mat_ground));

	// 中心蓝球
	world->add(std::make_shared<sphere3d>(vec3(0.0, 1.0, 0.0), 1.0, mat_center));

	// 左侧玻璃球
	world->add(std::make_shared<sphere3d>(vec3(-4.0, 1.0, 0.0), 1.0, mat_left));

	// 右侧金属球
	world->add(std::make_shared<sphere3d>(vec3(4.0, 1.0, 0.0), 1.0, mat_right));

	// 可选：加一个稀疏的烟雾球在空中（测试介质，计算量适中）
	auto smoke_boundary = std::make_shared<sphere3d>(vec3(0, 4, -2), 2.0, mat_ground);
	world->add(std::make_shared<volume_smoke>(smoke_boundary, 0.5f, vec3(1, 1, 1)));

	// --- 相机设置 ---
	image2d image(800, 450); // 16:9 比例
	camera3d camera(image);

	camera.samples_per_pixel = 16; // 计算量极小，16次采样即可有不错效果

	// 背景色设置为天空渐变（在你的 ray_color 中实现逻辑）
	camera.background_color = vec3(0.70, 0.80, 1.00);

	camera.look_from = vec3(5, 5, 5);
	camera.look_at = vec3(0, 0, 0);
	camera.fov = radians(90.0);

	// 景深控制（可选，增加视觉高级感）
	camera.defocus_angle = 0.0;
	camera.focus_dist = 9.0;

	camera.update();
	camera.render(world);

	return ppmshow(image);
}