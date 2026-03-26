#pragma once
#include <vector>
#include <algorithm>
#include "hittable.hpp"

class linear_bvh : public hittable {
private:
	struct bvh_node {
		aabb bbox;
		int index; // 分支节点：左子节点索引；叶子节点：objects 里的起始索引
		int count; // count > 0 表示叶子，存物体数量；count == 0 表示分支
		inline bool is_leaf() const { return count > 0; }
	};

	std::vector<bvh_node> nodes;
	std::vector<hittable_ptr> objects; // 存储排序后的物体指针

public:
	linear_bvh() = default;

	// 从 hittable_list 构建
	linear_bvh(const hittable_list &list) : objects(list.objects) {
		if (objects.empty())
			return;
		// 预留空间减少重分配次数 (N 个物体最多 2N-1 个节点)
		nodes.reserve(objects.size() * 2);
		build(0, objects.size());
	}

	virtual bool hit(const ray3d &r, interval t_interval, hit_record &rec) const override {
		if (nodes.empty() || !nodes[0].bbox.hit(r, t_interval))
			return false;
		bool hit_anything = false;
		float closest_so_far = t_interval.max_val;
		int stack[64];
		int stack_ptr = 0;
		stack[stack_ptr++] = 0;
		while (stack_ptr > 0) {
			const bvh_node &node = nodes[stack[--stack_ptr]];
			if (node.is_leaf()) {
				for (int i = 0; i < node.count; ++i) {
					if (objects[node.index + i]->hit(
							r, interval(t_interval.min_val, closest_so_far), rec)) {
						hit_anything = true;
						closest_so_far = rec.t;
					}
				}
			} else {
				int left_idx = node.index;
				int right_idx = node.index + 1;
				bool hit_l =
					nodes[left_idx].bbox.hit(r, interval(t_interval.min_val, closest_so_far));
				bool hit_r =
					nodes[right_idx].bbox.hit(r, interval(t_interval.min_val, closest_so_far));
				if (hit_l && hit_r) {
					// 简单的启发式：根据光线方向决定压栈顺序，优先走近的
					// 这里可以进一步优化，目前先按默认顺序
					stack[stack_ptr++] = right_idx;
					stack[stack_ptr++] = left_idx;
				} else if (hit_l) {
					stack[stack_ptr++] = left_idx;
				} else if (hit_r) {
					stack[stack_ptr++] = right_idx;
				}
			}
		}
		return hit_anything;
	}

	virtual aabb bounding_box() const override { return nodes.empty() ? aabb() : nodes[0].bbox; }

private:
	int build(size_t start, size_t end) {
		int curr_idx = nodes.size();
		nodes.emplace_back(); // 为当前节点占位
		size_t span = end - start;
		// 1. 计算当前范围内物体的整体 AABB
		aabb bbox;
		for (size_t i = start; i < end; ++i) {
			bbox = aabb(bbox, objects[i]->bounding_box());
		}
		if (span <= 2) { // 叶子节点阈值
			nodes[curr_idx] = {bbox, (int)start, (int)span};
		} else {
			// 2. 选择最长轴进行切分
			int axis = bbox.longest_axis();
			auto mid = start + span / 2;
			// 使用 std::nth_element 按照中心点在轴上的位置进行重排
			std::nth_element(objects.begin() + start, objects.begin() + mid, objects.begin() + end,
							 [axis](const hittable_ptr &a, const hittable_ptr &b) {
								 return a->bounding_box().xyz[axis].min_val <
										b->bounding_box().xyz[axis].min_val;
							 });
			// 3. 递归构建
			int left_idx = build(start, mid);
			build(mid, end); // 右孩子紧跟在左孩子后面
			// 4. 回填分支节点信息
			// 这里的 bbox 使用子节点合并的结果更紧凑
			nodes[curr_idx] = {bbox, left_idx, 0};
		}
		return curr_idx;
	}
};