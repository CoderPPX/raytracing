#pragma once
#include "camera.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "hittable.hpp"
#include <yaml-cpp/yaml.h>

// 辅助函数：将 YAML 节点转换为 vec3
namespace YAML {
    template<>
    struct convert<vec3> {
        static Node encode(const vec3& rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            return node;
        }
        static bool decode(const Node& node, vec3& rhs) {
            if (!node.IsSequence() || node.size() != 3) return false;
            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };
}

class yaml_loader {
public:
    // 返回包含初始化好的相机和世界的结构体
    struct scene_data {
        std::unique_ptr<camera3d> camera;
        std::shared_ptr<hittable_list> world;
        std::unique_ptr<image2d> image;
    };

    static scene_data load_from_file(const std::string& filename) {
        YAML::Node config = YAML::LoadFile(filename);
        scene_data data;
        // 1. 创建图像基础缓冲区
        auto cam_node = config["camera"];
        int width = cam_node["resolution"][0].as<int>();
        int height = cam_node["resolution"][1].as<int>();
        data.image = std::make_unique<image2d>(width, height);
        // 2. 初始化相机
        data.camera = std::make_unique<camera3d>(*data.image);
        data.camera->samples_per_pixel = cam_node["samples_per_pixel"].as<int>(16);
        data.camera->background_color = cam_node["background_color"].as<vec3>();
        data.camera->look_from = cam_node["look_from"].as<vec3>();
        data.camera->look_at = cam_node["look_at"].as<vec3>();
        data.camera->fov = radians(cam_node["fov"].as<float>());
        data.camera->defocus_angle = radians(cam_node["defocus_angle"].as<float>(0.0));
        data.camera->focus_dist = cam_node["focus_dist"].as<float>(10.0);
        data.camera->update(); // 必须调用以更新内部矩阵
        // 3. 构建材质库
        std::unordered_map<std::string, std::shared_ptr<material>> mat_library;
        auto mats_node = config["materials"];
        for (auto it = mats_node.begin(); it != mats_node.end(); ++it) {
            std::string name = it->first.as<std::string>();
            auto m = it->second;
            std::string type = m["type"].as<std::string>();
            std::shared_ptr<material> mat_ptr;
            if (type == "lambertian") {
                mat_ptr = std::make_shared<lambertian>(m["color"].as<vec3>());
            } else if (type == "metal") {
                mat_ptr = std::make_shared<metal>(m["color"].as<vec3>(), m["fuzz"].as<float>(0.0));
            } else if (type == "dielectric") {
                mat_ptr = std::make_shared<dielectric>(m["ior"].as<float>());
            } else if (type == "diffuse_light") {
                mat_ptr = std::make_shared<diffuse_light>(m["color"].as<vec3>());
            }
            mat_library[name] = mat_ptr;
        }
        // 4. 构建世界物体
        data.world = std::make_shared<hittable_list>();
        auto world_node = config["world"];
        for (const auto& obj : world_node) {
            std::string type = obj["type"].as<std::string>();
            auto mat = mat_library[obj["material"].as<std::string>()];
            if (type == "sphere") {
                vec3 center = obj["center"].as<vec3>();
                float radius = obj["radius"].as<float>();
                data.world->add(std::make_shared<sphere3d>(center, radius, mat));
            } else if (type == "mesh") {
                std::string path = obj["path"].as<std::string>();
                // 使用带 SAH BVH 加速的 mesh3d
                data.world->add(std::make_shared<mesh3d>(path, mat));
            }
        }
        return data;
    }
};