#pragma once
#include <vector>
#include <type_traits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fmt/format.h>
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include "stb_image_write.h"
using namespace glm;

using color_type = vec<3, uint8_t>;
struct image2d {
	int width, height;
	std::vector<color_type> data;
	inline image2d(int w = 256, int h = 256, color_type fill = {0xFF, 0xFF, 0xFF})
		: width(w), height(h), data(w * h, fill) {}
	inline color_type &at(int x, int y) { return data[y * width + x]; }
	inline const color_type &at(int x, int y) const { return data[y * width + x]; }
	inline const color_type &at(ivec2 pos) const { return data[pos.y * width + pos.x]; }
	// With gamma correction
	inline void write_vec3(int x, int y, vec3 color) {
		// if (std::isnan(color.x) || std::isnan(color.y) || std::isnan(color.z))
		// 	color = vec3(0);
		// if (std::isinf(color.x) || std::isinf(color.y) || std::isinf(color.z))
		// 	color = vec3(1);
		color = sqrt(max(color, vec3(0.0)));
		this->at(x, y) = clamp(
			{uint8_t(255.999 * color.r), uint8_t(255.999 * color.g), uint8_t(255.999 * color.b)},
			color_type(0), color_type(0xFF));
	}
	inline float aspect_ratio() const { return (float)width / height; }
	inline void save_to(const std::string &filename) {
		stbi_write_png(filename.c_str(), width, height, 3, data.data(), 3 * width);
	}
};

template <length_t L, typename T, qualifier Q> struct fmt::formatter<vec<L, T, Q>> {
	std::string format_str = "{}";
	constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
		auto it = ctx.begin(), end = ctx.end();
		if (it != end && *it != '}') {
			format_str = "{:";
			while (it != end && *it != '}') {
				if constexpr (std::is_integral_v<T>) {
					if (*it == '.' || *it == 'f' || *it == 'e' || *it == 'g') {
						throw format_error("precision format is not allowed for integer-type vec");
					}
				}
				format_str += *it++;
			}
			format_str += "}";
		}
		return it;
	}
	template <typename FormatContext>
	auto format(const vec<L, T, Q> &v, FormatContext &ctx) const -> decltype(ctx.out()) {
		auto out = fmt::format_to(ctx.out(), "(");
		for (length_t i = 0; i < L; ++i) {
			if constexpr (std::is_same_v<T, unsigned char>) {
				out = fmt::format_to(out, format_str, static_cast<int>(v[i]));
			} else {
				out = fmt::format_to(out, format_str, v[i]);
			}
			if (i < L - 1)
				out = fmt::format_to(out, ", ");
		}
		return fmt::format_to(out, ")");
	}
};

template <typename glm_vec> inline auto length_square(const glm_vec &v) { return dot(v, v); }
template <typename glm_vec> inline bool near_zero(const glm_vec &v) {
	return length_square(v) < 1e-10;
}

struct ray3d {
	vec3 origin, direction;
	inline ray3d() = default;
	inline ray3d(vec3 orig, vec3 dir) : origin(orig), direction(dir) {}
	inline vec3 at(float t) const { return origin + t * direction; }
};

struct interval {
	// Notes: min和max的作用有哪些
	float min_val, max_val;
	// Default interval is empty
	inline interval(float min_val = +1e36, float max_val = -1e36)
		: min_val(min_val), max_val(max_val) {}
	// union of a, b
	inline static interval union_(interval a, interval b) {
		return interval(min(a.min_val, b.min_val), max(a.max_val, b.max_val));
	}
	inline static interval intersection(interval a, interval b) {
		return interval(max(a.min_val, b.min_val), min(a.max_val, b.max_val));
	}
	inline float size() const { return max_val - min_val; }
	inline bool contains(float x) const { return min_val <= x && x <= max_val; }
	inline bool surrounds(float x) const { return min_val < x && x < max_val; }
	inline float clamp(float x) const {
		return x > max_val ? max_val : (x < min_val ? min_val : x);
	}
	inline interval expand(float delta) {
		float padding = delta / 2.0;
		return interval(min_val - padding, max_val + padding);
	}
	inline void extend_with(float x) {
		if (min_val > x) {
			min_val = x;
		}
		if (max_val < x) {
			max_val = x;
		}
	}
	inline float mid() const { return (min_val + max_val) / 2.0; }
};