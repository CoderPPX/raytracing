#pragma once
#include <algorithm>
#include "aabb.hpp"
#include "hittable.hpp"

struct linear_bvh_leaf_node;
struct linear_bvh_node : public hittable {
public:
	std::unique_ptr<linear_bvh_node> left, right;
	constexpr static uint32_t min_batch_size = 8;

public:
	inline linear_bvh_node() = default;
	template <typename HittableContainter>
	inline linear_bvh_node(const HittableContainter &container) {
		build_linear_bvh(std::begin(container), std::end(container));
	}
	inline bool is_leaf() const { return left == nullptr && right == nullptr; }
	inline bool hit(const ray3d &r, interval ray_t, hit_record &rec,
					random_generator &generator) const override;
	template <typename Iterator>
		requires(std::random_access_iterator<Iterator> &&
				 std::same_as<hittable_ptr, std::iter_value_t<Iterator>>)
	inline void build_linear_bvh(Iterator begin, Iterator end);
	// Notes: Compiler通过vtable找到linear_bvh_leaf_node的destructor防止memory leak
	virtual ~linear_bvh_node() = default;
};

struct linear_bvh_leaf_node : public linear_bvh_node {
public:
	std::array<hittable_ptr, min_batch_size> objects;
};

template <typename Iterator>
	requires(std::random_access_iterator<Iterator> &&
			 std::same_as<hittable_ptr, std::iter_value_t<Iterator>>)
inline void linear_bvh_node::build_linear_bvh(Iterator begin, Iterator end) {
	for (auto it = begin; it != end; ++it) {
		bbox = aabb::union_(bbox, (*it)->bbox);
	}
	bbox.pad_to_minimums();
	if (end - begin <= min_batch_size) {
		auto temp_left = new linear_bvh_leaf_node();
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
	left = std::make_unique<linear_bvh_node>();
	right = std::make_unique<linear_bvh_node>();
	left->build_linear_bvh(temp.begin(), it);
	right->build_linear_bvh(it, temp.end());
}

inline bool linear_bvh_node::hit(const ray3d &r, interval ray_t, hit_record &rec,
								 random_generator &generator) const {
	if (!bbox.hit(r, ray_t)) {
		return false;
	}
	bool hit_anything = false;
	if (is_leaf()) {
		hit_record temp_rec;
		auto leaf = reinterpret_cast<const linear_bvh_leaf_node *>(this);
		for (auto &object : leaf->objects) {
			if (object != nullptr && object->hit(r, ray_t, temp_rec, generator)) {
				hit_anything = true;
				ray_t.max_val = temp_rec.t;
				rec = temp_rec;
			}
		}
		return hit_anything;
	}
	if (left != nullptr && left->bbox.hit(r, ray_t)) {
		hit_anything |= left->hit(r, ray_t, rec, generator);
		if (hit_anything) {
			ray_t.max_val = rec.t;
		}
	}
	if (right != nullptr && right->bbox.hit(r, ray_t)) {
		hit_anything |= right->hit(r, ray_t, rec, generator);
	}
	return hit_anything;
}