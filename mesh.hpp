#pragma once
#include <algorithm>
#include "aabb.hpp"
#include "hittable.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

struct mesh3d : public hittable {
public:
	std::shared_ptr<material> mat;
	struct triangle {
		aabb bbox;
		vec3 v0, u, v, n, w;
	};

public:
	inline mesh3d(const std::string &model_path, std::shared_ptr<material> mat_) : mat(mat_) {
		load_model(model_path);
	}
	inline void load_model(const std::string &model_path) {
		static Assimp::Importer importer;
		const auto importFlags = aiProcess_Triangulate | aiProcess_PreTransformVertices |
								 aiProcess_JoinIdenticalVertices;
		const aiScene *scene = importer.ReadFile(model_path, importFlags);
		if (!scene || !scene->mRootNode) {
			fmt::print(stderr, "[Error] {}\n", importer.GetErrorString());
			return;
		}
		size_t triangles_count = 0;
		for (size_t i = 0; i < scene->mNumMeshes; i++) {
			triangles_count += scene->mMeshes[i]->mNumFaces;
		}
		std::vector<triangle> triangles;
		triangles.reserve(triangles_count);
		load_mesh(scene, triangles);
		triangle_groups.reserve((triangles_count + 3) / 4);
		build_bvh_sah(&root_node, triangles.begin(), triangles.end());
		bbox = root_node.bbox;
	}

	inline bool hit(const ray3d &r, interval ray_t, hit_record &rec,
					random_generator &generator) const override {
		static thread_local std::array<const bvh_node *, 32> node_stack;
		if (!bbox.hit(r, ray_t)) {
			return false;
		}
		int32_t stack_size = 0;
		node_stack[stack_size++] = &root_node;
		bool hit_anything = false;
		while (stack_size != 0) {
			auto node = node_stack[--stack_size];
			if (node->is_leaf()) {
				const auto &group = triangle_groups[node->group_index];
				vec3 dir = r.direction;
				vec3 orig = r.origin;
				vec4 pvecx = dir.y * group.vz - dir.z * group.vy;
				vec4 pvecy = dir.z * group.vx - dir.x * group.vz;
				vec4 pvecz = dir.x * group.vy - dir.y * group.vx;
				vec4 det = group.ux * pvecx + group.uy * pvecy + group.uz * pvecz;
				vec4 safe_det = {
					std::abs(det[0]) < 1e-10f ? 1.0f : det[0],
					std::abs(det[1]) < 1e-10f ? 1.0f : det[1],
					std::abs(det[2]) < 1e-10f ? 1.0f : det[2],
					std::abs(det[3]) < 1e-10f ? 1.0f : det[3],
				};
				vec4 inv_det = 1.0f / safe_det;
				vec4 tvecx = orig.x - group.v0x;
				vec4 tvecy = orig.y - group.v0y;
				vec4 tvecz = orig.z - group.v0z;
				vec4 u_coord = (tvecx * pvecx + tvecy * pvecy + tvecz * pvecz) * inv_det;
				vec4 qvecx = tvecy * group.uz - tvecz * group.uy;
				vec4 qvecy = tvecz * group.ux - tvecx * group.uz;
				vec4 qvecz = tvecx * group.uy - tvecy * group.ux;
				vec4 v_coord = (dir.x * qvecx + dir.y * qvecy + dir.z * qvecz) * inv_det;
				vec4 t = (group.vx * qvecx + group.vy * qvecy + group.vz * qvecz) * inv_det;
				for (uint32_t i = 0; i < 4; i++) {
					if (std::abs(det[i]) > 1e-10f && u_coord[i] >= 0.0f && v_coord[i] >= 0.0f &&
						u_coord[i] + v_coord[i] <= 1.0f && ray_t.contains(t[i])) {
						hit_anything = true;
						ray_t.max_val = t[i];
						rec.t = t[i];
						rec.point = r.at(t[i]);
						vec3 outward_normal =
							normalize(vec3(group.nx[i], group.ny[i], group.nz[i]));
						rec.set_face_normal(r, outward_normal);
						rec.mat = mat;
					}
				}
				continue;
			}
			float left_t = FLT_MAX, right_t = FLT_MAX;
			bool left_hit = node->left->bbox.hit(r, ray_t, left_t);
			bool right_hit =
				node->right != nullptr ? node->right->bbox.hit(r, ray_t, right_t) : false;
			if (left_t > right_t) {
				if (left_hit) {
					node_stack[stack_size++] = node->left.get();
				}
				if (right_hit) {
					node_stack[stack_size++] = node->right.get();
				}
			} else {
				if (right_hit) {
					node_stack[stack_size++] = node->right.get();
				}
				if (left_hit) {
					node_stack[stack_size++] = node->left.get();
				}
			}
		}
		return hit_anything;
	}

private:
	// group size = 4
	struct alignas(256) triangle_group {
		vec4 v0x, v0y, v0z;
		vec4 ux, uy, uz;
		vec4 vx, vy, vz;
		vec4 nx, ny, nz;
		vec4 wx, wy, wz;
	};
	struct alignas(16) bvh_node {
		std::unique_ptr<bvh_node> left, right;
		aabb bbox;
		size_t group_index = -1; // leaf node
		inline bool is_leaf() const { return group_index != -1; }
	};
	bvh_node root_node;
	std::vector<triangle_group> triangle_groups;

	inline void load_mesh(const aiScene *scene, std::vector<triangle> &triangles) {
		for (size_t i = 0; i < scene->mNumMeshes; i++) {
			aiMesh *mesh = scene->mMeshes[i];
			triangles.reserve(triangles.size() + mesh->mNumFaces);
			for (size_t j = 0; j < mesh->mNumFaces; j++) {
				aiFace &face = mesh->mFaces[j];
				if (face.mNumIndices != 3)
					continue;
				aiVector3D &a = mesh->mVertices[face.mIndices[0]];
				aiVector3D &b = mesh->mVertices[face.mIndices[1]];
				aiVector3D &c = mesh->mVertices[face.mIndices[2]];
				vec3 v0 = {a.x, a.y, a.z};
				vec3 v1 = {b.x, b.y, b.z};
				vec3 v2 = {c.x, c.y, c.z};
				triangle tri;
				tri.v0 = v0;
				tri.u = v1 - v0;
				tri.v = v2 - v0;
				tri.n = glm::cross(tri.u, tri.v);
				vec3 vmin = min(min(v0, v1), v2);
				vec3 vmax = max(max(v0, v1), v2);
				tri.bbox = aabb({vmin.x, vmax.x}, {vmin.y, vmax.y}, {vmin.z, vmax.z});
				float n2 = glm::dot(tri.n, tri.n);
				if (n2 > 1e-8f) {
					tri.w = tri.n / n2;
				} else {
					tri.w = vec3(0.0f);
				}
				triangles.push_back(tri);
			}
		}
	}

	inline void build_bvh_sah(bvh_node *node, std::vector<triangle>::iterator begin,
							  std::vector<triangle>::iterator end) {
		node->bbox = aabb();
		for (auto it = begin; it != end; ++it) {
			node->bbox.union_(it->bbox);
		}
		node->bbox.pad_to_minimums();
		uint32_t current_size = end - begin;
		if (current_size <= 4) {
			triangle_groups.emplace_back();
			auto &group = triangle_groups.back();
			for (uint32_t i = 0; i < current_size; ++i) {
				auto &[_, v0, u, v, n, w] = *(begin + i);
				group.v0x[i] = v0.x;
				group.v0y[i] = v0.y;
				group.v0z[i] = v0.z;
				group.ux[i] = u.x;
				group.uy[i] = u.y;
				group.uz[i] = u.z;
				group.vx[i] = v.x;
				group.vy[i] = v.y;
				group.vz[i] = v.z;
				group.nx[i] = n.x;
				group.ny[i] = n.y;
				group.nz[i] = n.z;
				group.wx[i] = w.x;
				group.wy[i] = w.y;
				group.wz[i] = w.z;
			}
			// 剩余槽位默认是 glm::vec4(0.0f)，相当于 dummy 三角形
			node->group_index = triangle_groups.size() - 1;
			return;
		}
		constexpr uint32_t num_bins = 12;
		struct bin {
			aabb bbox;
			uint32_t count = 0;
		};
		int left_counts[num_bins - 1], right_counts[num_bins - 1];
		float left_areas[num_bins - 1], right_areas[num_bins - 1];
		uint32_t best_split_idx = 0, best_axis = 0;
		float min_cost = std::numeric_limits<float>::max();
		for (uint32_t axis_id = 0; axis_id < 3; ++axis_id) {
			bin bins[num_bins] = {};
			auto axis = node->bbox.xyz[axis_id];
			float axis_min = axis.min_val, axis_size = axis.size();
			for (auto it = begin; it != end; ++it) {
				float mid = it->bbox.xyz[axis_id].mid();
				int bin_idx = num_bins * (mid - axis.min_val) / axis_size;
				bin_idx = clamp(bin_idx, 0, int(num_bins) - 1);
				++bins[bin_idx].count;
				bins[bin_idx].bbox.union_(it->bbox);
			}
			aabb left_box, right_box;
			int left_sum = 0, right_sum = 0;
			for (int i = 0; i < num_bins - 1; ++i) {
				left_box.union_(bins[i].bbox);
				left_sum += bins[i].count;
				left_areas[i] = left_box.surface_area();
				left_counts[i] = left_sum;
			}
			for (int i = num_bins - 1; i > 0; i--) {
				right_box.union_(bins[i].bbox);
				right_sum += bins[i].count;
				right_areas[i - 1] = right_box.surface_area();
				right_counts[i - 1] = right_sum;
			}
			for (int i = 0; i < num_bins - 1; i++) {
				// SAH formula：Cost = 1 + (Area_L * Count_L + Area_R * Count_R) / Total_Area
				float cost = left_areas[i] * left_counts[i] + right_areas[i] * right_counts[i];
				if (cost < min_cost) {
					min_cost = cost;
					best_axis = axis_id;
					best_split_idx = i;
				}
			}
		}
		float axis_min = node->bbox.xyz[best_axis].min_val;
		float axis_size = node->bbox.xyz[best_axis].size();
		auto mid = std::partition(begin, end, [&](const triangle &tri) {
			float mid = tri.bbox.xyz[best_axis].mid();
			int bin_idx = int(num_bins * (mid - axis_min) / axis_size);
			return clamp(bin_idx, 0, int(num_bins) - 1) <= best_split_idx;
		});
		if (mid == begin || mid == end) {
			mid = begin + current_size / 2;
			std::nth_element(begin, mid, end, [&](triangle &a, triangle &b) {
				return a.bbox.xyz[best_axis].mid() < b.bbox.xyz[best_axis].mid();
			});
		}
		node->left = std::make_unique<bvh_node>();
		node->right = std::make_unique<bvh_node>();
		build_bvh_sah(node->left.get(), begin, mid);
		build_bvh_sah(node->right.get(), mid, end);
	}
};

struct mesh3d_naive : public hittable {
public:
	std::shared_ptr<material> mat;

	struct triangle {
		aabb bbox;
		vec3 v0, u, v, n, w;
	};

	std::vector<triangle> triangles;
	aabb bbox;

public:
	inline mesh3d_naive(const std::string &model_path, std::shared_ptr<material> mat_) : mat(mat_) {
		load_model(model_path);
	}

	inline void load_model(const std::string &model_path) {
		static Assimp::Importer importer;
		const auto importFlags = aiProcess_Triangulate | aiProcess_PreTransformVertices |
								 aiProcess_JoinIdenticalVertices;
		const aiScene *scene = importer.ReadFile(model_path, importFlags);
		if (!scene || !scene->mRootNode) {
			fmt::print(stderr, "[Error] {}\n", importer.GetErrorString());
			return;
		}
		size_t triangles_count = 0;
		for (size_t i = 0; i < scene->mNumMeshes; i++) {
			triangles_count += scene->mMeshes[i]->mNumFaces;
		}
		triangles.reserve(triangles_count);
		load_mesh(scene, triangles);

		// 计算整体 bbox
		bbox = aabb();
		for (auto &tri : triangles) {
			bbox.union_(tri.bbox);
		}
		bbox.pad_to_minimums();
	}

	inline bool hit(const ray3d &r, interval ray_t, hit_record &rec,
					random_generator &generator) const override {
		if (!bbox.hit(r, ray_t)) {
			return false;
		}
		bool hit_anything = false;
		float closest_so_far = ray_t.max_val;
		hit_record temp_rec;

		for (const auto &tr : triangles) {
			// --- Möller–Trumbore 算法 ---
			vec3 pvec = cross(r.direction, tr.v);
			float det = dot(tr.u, pvec);
			if (std::abs(det) < 1e-10f)
				continue;
			float inv_det = 1.0f / det;

			vec3 tvec = r.origin - tr.v0;
			float u_coord = dot(tvec, pvec) * inv_det;
			if (u_coord < 0.0f || u_coord > 1.0f)
				continue;

			vec3 qvec = cross(tvec, tr.u);
			float v_coord = dot(r.direction, qvec) * inv_det;
			if (v_coord < 0.0f || u_coord + v_coord > 1.0f)
				continue;

			float t = dot(tr.v, qvec) * inv_det;
			if (t < ray_t.min_val || t > closest_so_far)
				continue;

			// 更新最近交点
			hit_anything = true;
			closest_so_far = t;
			temp_rec.t = t;
			temp_rec.point = r.at(t);
			vec3 outward_normal = normalize(tr.n);
			temp_rec.set_face_normal(r, outward_normal);
			temp_rec.mat = mat;
		}

		if (hit_anything) {
			rec = temp_rec;
		}
		return hit_anything;
	}

private:
	inline void load_mesh(const aiScene *scene, std::vector<triangle> &triangles) {
		for (size_t i = 0; i < scene->mNumMeshes; i++) {
			aiMesh *mesh = scene->mMeshes[i];
			triangles.reserve(triangles.size() + mesh->mNumFaces);
			for (size_t j = 0; j < mesh->mNumFaces; j++) {
				aiFace &face = mesh->mFaces[j];
				if (face.mNumIndices != 3)
					continue;

				aiVector3D &a = mesh->mVertices[face.mIndices[0]];
				aiVector3D &b = mesh->mVertices[face.mIndices[1]];
				aiVector3D &c = mesh->mVertices[face.mIndices[2]];
				vec3 v0 = {a.x, a.y, a.z};
				vec3 v1 = {b.x, b.y, b.z};
				vec3 v2 = {c.x, c.y, c.z};

				triangle tri;
				tri.v0 = v0;
				tri.u = v1 - v0;
				tri.v = v2 - v0;
				tri.n = glm::cross(tri.u, tri.v);

				vec3 vmin = min(min(v0, v1), v2);
				vec3 vmax = max(max(v0, v1), v2);
				tri.bbox = aabb({vmin.x, vmax.x}, {vmin.y, vmax.y}, {vmin.z, vmax.z});

				float n2 = glm::dot(tri.n, tri.n);
				tri.w = (n2 > 1e-8f) ? tri.n / n2 : vec3(0.0f);

				triangles.push_back(tri);
			}
		}
	}
};