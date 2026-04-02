#pragma once
#include <algorithm>
#include "aabb.hpp"
#include "hittable.hpp"

struct bvh_leaf_node;
struct bvh_node : public hittable {
public:
	std::unique_ptr<bvh_node> left, right;
	constexpr static uint32_t min_batch_size = 8;

public:
	inline bvh_node() = default;
	template <typename HittableContainter> inline bvh_node(const HittableContainter &container) {
		std::vector<hittable_ptr> container_(container);
		build_bvh_sah(std::begin(container_), std::end(container_));
	}
	inline bool is_leaf() const { return left == nullptr && right == nullptr; }
	inline bool hit(const ray3d &r, interval ray_t, hit_record &rec,
					random_generator &generator) const override;
	inline bool hit_recursive(const ray3d &r, interval ray_t, hit_record &rec,
							  random_generator &generator) const;
	template <typename Iterator>
		requires(std::random_access_iterator<Iterator> &&
				 std::same_as<hittable_ptr, std::iter_value_t<Iterator>>)
	inline void build_bvh(Iterator begin, Iterator end);
	template <typename Iterator>
		requires(std::random_access_iterator<Iterator> &&
				 std::same_as<hittable_ptr, std::iter_value_t<Iterator>>)
	inline void build_bvh_sah(Iterator begin, Iterator end);
	// Notes: Compiler通过vtable找到bvh_leaf_node的destructor防止memory leak
	virtual ~bvh_node() = default;
};

struct bvh_leaf_node : public bvh_node {
public:
	std::array<hittable_ptr, min_batch_size> objects;
};

template <typename Iterator>
	requires(std::random_access_iterator<Iterator> &&
			 std::same_as<hittable_ptr, std::iter_value_t<Iterator>>)
inline void bvh_node::build_bvh(Iterator begin, Iterator end) {
	for (auto it = begin; it != end; ++it) {
		bbox = aabb::union_(bbox, (*it)->bbox);
	}
	bbox.pad_to_minimums();
	if (end - begin <= min_batch_size) {
		auto temp_left = new bvh_leaf_node();
		temp_left->bbox = bbox;
		for (uint32_t i = 0; begin + i != end; ++i) {
			temp_left->objects[i] = *(begin + i);
		}
		left.reset(temp_left);
		return;
	}
	auto longest_axis_id = bbox.longest_axis();
	std::vector<hittable_ptr> temp(begin, end);
	std::sort(temp.begin(), temp.end(), [&](hittable_ptr &a, hittable_ptr &b) {
		return a->bbox.xyz[longest_axis_id].mid() < b->bbox.xyz[longest_axis_id].mid();
	});
	auto it = temp.begin();
	float mid = bbox.xyz[longest_axis_id].mid();
	for (; it != temp.end() && (*it)->bbox.xyz[longest_axis_id].mid() < mid; ++it) {
	}
	if (it == begin || it == end) {
		it = temp.begin() + temp.size() / 2;
	}
	left = std::make_unique<bvh_node>();
	right = std::make_unique<bvh_node>();
	left->build_bvh(temp.begin(), it);
	right->build_bvh(it, temp.end());
}

template <typename Iterator>
	requires(std::random_access_iterator<Iterator> &&
			 std::same_as<hittable_ptr, std::iter_value_t<Iterator>>)
inline void bvh_node::build_bvh_sah(Iterator begin, Iterator end) {
	bbox = aabb();
	for (auto it = begin; it != end; ++it) {
		bbox.union_((*it)->bbox);
	}
	bbox.pad_to_minimums();
	uint32_t current_size = end - begin;
	if (current_size <= min_batch_size) {
		auto leaf = new bvh_leaf_node();
		leaf->bbox = bbox;
		for (uint32_t i = 0; i < current_size; ++i) {
			leaf->objects[i] = *(begin + i);
		}
		left.reset(leaf);
		right.reset(nullptr);
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
		auto axis = bbox.xyz[axis_id];
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
	float axis_min = bbox.xyz[best_axis].min_val;
	float axis_size = bbox.xyz[best_axis].size();
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
	left = std::make_unique<bvh_node>();
	right = std::make_unique<bvh_node>();
	left->build_bvh_sah(begin, mid);
	right->build_bvh_sah(mid, end);
}

inline bool bvh_node::hit_recursive(const ray3d &r, interval ray_t, hit_record &rec,
									random_generator &generator) const {
	if (!bbox.hit(r, ray_t)) {
		return false;
	}
	bool hit_anything = false;
	if (is_leaf()) {
		hit_record temp_rec;
		auto leaf = reinterpret_cast<const bvh_leaf_node *>(this);
		for (auto &object : leaf->objects) {
			if (object != nullptr && object->hit(r, ray_t, temp_rec, generator)) {
				hit_anything = true;
				ray_t.max_val = temp_rec.t;
				rec = temp_rec;
			}
		}
		return hit_anything;
	}
	if (left != nullptr) {
		hit_anything |= left->hit(r, ray_t, rec, generator);
		if (hit_anything) {
			ray_t.max_val = rec.t;
		}
	}
	if (right != nullptr) {
		hit_anything |= right->hit(r, ray_t, rec, generator);
	}
	return hit_anything;
}

inline bool bvh_node::hit(const ray3d &r, interval ray_t, hit_record &rec,
						  random_generator &generator) const {
	static thread_local std::array<const bvh_node *, 32> node_stack;
	int32_t stack_ptr = 0;
	node_stack[stack_ptr++] = this;
	bool hit_anything = false;
	while (stack_ptr != 0) {
		auto node_ptr = node_stack[--stack_ptr];
		if (!node_ptr->bbox.hit(r, ray_t)) {
			continue;
		}
		if (node_ptr->is_leaf()) {
			auto leaf = reinterpret_cast<const bvh_leaf_node *>(node_ptr);
			for (auto &object : leaf->objects) {
				if (object != nullptr && object->hit(r, ray_t, rec, generator)) {
					hit_anything = true;
					ray_t.max_val = rec.t;
				}
			}
			continue;
		}
		float left_t, right_t;
		bool left_hit =
			node_ptr->left != nullptr ? node_ptr->left->bbox.hit(r, ray_t, left_t) : false;
		bool right_hit =
			node_ptr->right != nullptr ? node_ptr->right->bbox.hit(r, ray_t, right_t) : false;
		if (left_t > right_t) {
			if (left_hit) {
				node_stack[stack_ptr++] = node_ptr->left.get();
			}
			if (right_hit) {
				node_stack[stack_ptr++] = node_ptr->right.get();
			}
		} else {
			if (right_hit) {
				node_stack[stack_ptr++] = node_ptr->right.get();
			}
			if (left_hit) {
				node_stack[stack_ptr++] = node_ptr->left.get();
			}
		}
	}
	return hit_anything;
}