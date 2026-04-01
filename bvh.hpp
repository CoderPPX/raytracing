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
		build_bvh(std::begin(container), std::end(container));
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
	static thread_local std::array<int8_t, 64> rip_stack;
	static thread_local std::array<const bvh_node *, 64> node_stack;
	rip_stack.fill(0);
	node_stack[0] = this;
	int32_t stack_size = 1;
	bool hit_anything = false;
	while (stack_size != 0) {
		auto rip = rip_stack[stack_size - 1];
		auto node_ptr = node_stack[stack_size - 1];
		if (!node_ptr->bbox.hit(r, ray_t)) {
			--stack_size;
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
			--stack_size;
			continue;
		}
		if ((node_ptr->left != nullptr) && rip == 0) {
			rip_stack[stack_size - 1] = 1;
			node_stack[stack_size++] = node_ptr->left.get();
			continue;
		}
		if ((node_ptr->right != nullptr) && rip <= 1) {
			rip_stack[stack_size - 1] = 2;
			node_stack[stack_size++] = node_ptr->right.get();
			continue;
		}
		rip_stack[--stack_size] = 0;
	}
	return hit_anything;
}