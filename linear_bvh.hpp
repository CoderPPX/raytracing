#pragma once
#include <algorithm>
#include "aabb.hpp"
#include "hittable.hpp"

struct linear_bvh : public hittable {
public:
	struct linear_bvh_node {
		aabb bbox;
		uint32_t num_childs; // 0: middle, > 0: leaf node
		union {
			uint32_t right_node_idx = 0;   // middle
			uint32_t child_objects_offset; // leaf node
		};
		inline bool is_leaf() const { return num_childs != 0; }
	};

	aabb bbox;
	std::vector<hittable_ptr> objects;
	std::vector<linear_bvh_node> nodes;
	constexpr static uint32_t min_batch_size = 8;

public:
	inline linear_bvh() = default;
	template <typename HittableContainter>
	inline linear_bvh(const HittableContainter &container) : objects(container) {
		build_bvh_sah(objects.begin(), objects.end());
	}
	template <typename Iterator>
		requires(std::random_access_iterator<Iterator> &&
				 std::same_as<hittable_ptr, std::iter_value_t<Iterator>>)
	inline uint32_t build_bvh_sah(Iterator begin, Iterator end) {
		aabb bounds;
		uint32_t current_idx = nodes.size();
		nodes.emplace_back();
		for (auto it = begin; it != end; ++it) {
			bounds = aabb::union_(bounds, (*it)->bbox);
		}
		bbox.pad_to_minimums();
		nodes[current_idx].bbox = bounds;
		uint32_t current_size = end - begin;
		if (current_size <= min_batch_size) {
			nodes[current_idx].num_childs = current_size;
			nodes[current_idx].child_objects_offset = std::distance(objects.begin(), begin);
			return current_idx;
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
			auto axis = bounds.xyz[axis_id];
			float axis_min = axis.min_val, axis_size = axis.size();
			for (auto it = begin; it != end; ++it) {
				float mid = (*it)->bbox.xyz[axis_id].mid();
				int bin_idx = num_bins * (mid - axis.min_val) / axis_size;
				bin_idx = clamp(bin_idx, 0, int(num_bins) - 1);
				++bins[bin_idx].count;
				bins[bin_idx].bbox.union_((*it)->bbox);
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
				right_box = aabb::union_(right_box, bins[i].bbox);
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
		float axis_min = bounds.xyz[best_axis].min_val;
		float axis_size = bounds.xyz[best_axis].size();
		auto mid = std::partition(begin, end, [&](const hittable_ptr &a) {
			float mid = a->bbox.xyz[best_axis].mid();
			int bin_idx = int(num_bins * (mid - axis_min) / axis_size);
			return clamp(bin_idx, 0, int(num_bins) - 1) <= best_split_idx;
		});
		if (mid == begin || mid == end) {
			mid = begin + current_size / 2;
			std::nth_element(begin, mid, end, [&](hittable_ptr &a, hittable_ptr &b) {
				return a->bbox.xyz[best_axis].mid() < b->bbox.xyz[best_axis].mid();
			});
		}
		nodes[current_idx].num_childs = 0;
		build_bvh_sah(begin, mid);
		nodes[current_idx].right_node_idx = build_bvh_sah(mid, end);
		return current_idx;
	}

	inline bool hit(const ray3d &r, interval ray_t, hit_record &rec,
					random_generator &generator) const {
		static thread_local std::array<uint32_t, 32> node_idx_stack;
		if (!nodes[0].bbox.hit(r, ray_t)) {
			return false;
		}
		int32_t stack_ptr = 0;
		node_idx_stack[stack_ptr++] = 0;
		bool hit_anything = false;
		while (stack_ptr != 0) {
			auto current_idx = node_idx_stack[--stack_ptr];
			const auto &current_node = nodes[current_idx];
			if (current_node.is_leaf()) {
				for (uint32_t idx = 0; idx < current_node.num_childs; ++idx) {
					const auto object = objects[idx + current_node.child_objects_offset];
					if (object->hit(r, ray_t, rec, generator)) {
						hit_anything = true;
						ray_t.max_val = rec.t;
					}
				}
				continue;
			}
			float left_t = +1e35, right_t = +1e35;
			uint32_t left_idx = current_idx + 1;
			uint32_t right_idx = current_node.right_node_idx;
			bool left_hit = nodes[left_idx].bbox.hit(r, ray_t, left_t);
			bool right_hit = right_idx != 0 ? nodes[right_idx].bbox.hit(r, ray_t, right_t) : false;
			if (left_t > right_t) {
				if (left_hit) {
					node_idx_stack[stack_ptr++] = left_idx;
				}
				if (right_hit) {
					node_idx_stack[stack_ptr++] = right_idx;
				}
			} else {
				if (right_hit) {
					node_idx_stack[stack_ptr++] = right_idx;
				}
				if (left_hit) {
					node_idx_stack[stack_ptr++] = left_idx;
				}
			}
		}
		return hit_anything;
	}
};