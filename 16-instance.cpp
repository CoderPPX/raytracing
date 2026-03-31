#include "camera.hpp"
#include "ppmshow.hpp"
#include "hittable.hpp"
#include "raytracing.hpp"

int main() {
	// 材质定义
	auto red = std::make_shared<lambertian>(vec3(.65, .05, .05));
	auto white = std::make_shared<lambertian>(vec3(.73, .73, .73));
	auto green = std::make_shared<lambertian>(vec3(.12, .45, .15));
	auto light = std::make_shared<diffuse_light>(vec3(15, 15, 15));
	// --- 墙壁与光源 ---
	// 左墙 (绿)
	auto world = std::make_shared<hittable_list>();
	world->add(
		std::make_shared<quadrilateral>(vec3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	// 右墙 (红)
	world->add(
		std::make_shared<quadrilateral>(vec3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	// 顶部光源
	world->add(std::make_shared<quadrilateral>(vec3(213, 554, 227), vec3(130, 0, 0),
											   vec3(0, 0, 105), light));
	// 地面
	world->add(
		std::make_shared<quadrilateral>(vec3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	// 顶面
	world->add(std::make_shared<quadrilateral>(vec3(555, 555, 555), vec3(-555, 0, 0),
											   vec3(0, 0, -555), white));
	// 后墙
	world->add(
		std::make_shared<quadrilateral>(vec3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));
	// --- 实例化测试：旋转的盒子 ---
	// 1. 高盒子：旋转 15 度，平移到 (265, 0, 295)
	// 先创建一个在原点的盒子
	auto box1 = std::make_shared<box3d>(vec3(0, 0, 0), vec3(165, 330, 165), white);
	mat4 trans1(1.0);
	trans1 = translate(trans1, vec3(265, 0, 295));
	trans1 = rotate(trans1, radians(15.f), vec3(0, 1, 0));
	world->add(std::make_shared<instance>(box1, std::vector<mat4>{trans1}));

	// 2. 矮盒子：旋转 -18 度，平移到 (130, 0, 65)
	auto box2 = std::make_shared<box3d>(vec3(0, 0, 0), vec3(165, 165, 165), white);
	mat4 trans2(1.0);
	trans2 = translate(trans2, vec3(130, 0, 65));
	trans2 = rotate(trans2, radians(-18.f), vec3(0, 1, 0));
	world->add(std::make_shared<instance>(box2, std::vector<mat4>{trans2}));

	// --- 渲染设置 ---
	image2d image(800, 800);
	camera3d camera(image);
	camera.fov = radians(40.0);
	camera.samples_per_pixel = 200; // 实例较多时增加采样减少噪点
	camera.max_depth = 50;
	camera.look_from = vec3(278, 278, -800);
	camera.look_at = vec3(278, 278, 0);
	camera.background_color = vec3(0, 0, 0);

	// 建议：在 render 之前将 world 转化为 linear_bvh 提升速度
	// auto bvh_world = std::make_shared<linear_bvh<hittable_ptr>>(world);

	camera.update();
	camera.render(world);

	return ppmshow(image);
}