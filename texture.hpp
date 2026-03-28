#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include "perlin.hpp"
#include "stb_image.h"

struct stb_image {
	int width = 0, height = 0;
	std::shared_ptr<uint8_t> pixel_data; // 4 channels

	inline stb_image(const std::string &image_path, bool flip_vertical = false) {
		stbi_set_flip_vertically_on_load(flip_vertical);
		pixel_data = std::shared_ptr<uint8_t>(
			stbi_load(image_path.c_str(), &width, &height, nullptr, 4), &stbi_image_free);
	}

	inline vec<4, uint8_t> color(int i, int j) const {
		auto x = pixel_data.get()[(j * width + i) * 4 + 0];
		auto y = pixel_data.get()[(j * width + i) * 4 + 1];
		auto z = pixel_data.get()[(j * width + i) * 4 + 2];
		auto w = pixel_data.get()[(j * width + i) * 4 + 3];
		return vec<4, uint8_t>(x, y, z, w);
	}
};

struct texture {
	virtual ~texture() = default;
	virtual vec3 color(vec2 uv, vec3 point) const = 0;
};
using texture_ptr = std::shared_ptr<texture>;

struct solid_color_texture : public texture {
	vec3 albedo;
	solid_color_texture(vec3 color) : albedo(color) {}
	inline virtual vec3 color(vec2 uv, vec3 point) const override { return albedo; };
};

struct checker_texture : public texture {
public:
	float inv_scale;
	std::shared_ptr<texture> odd, even;

public:
	checker_texture(float scale, std::shared_ptr<texture> even, std::shared_ptr<texture> odd)
		: inv_scale(1.0 / scale), even(even), odd(odd) {}
	checker_texture(float scale, vec3 c1, vec3 c2)
		: checker_texture(scale, std::make_shared<solid_color_texture>(c1),
						  std::make_shared<solid_color_texture>(c2)) {}
	inline vec3 color(vec2 uv, vec3 p) const override {
		auto xInteger = int(std::floor(inv_scale * p.x));
		auto yInteger = int(std::floor(inv_scale * p.y));
		auto zInteger = int(std::floor(inv_scale * p.z));
		bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;
		return isEven ? even->color(uv, p) : odd->color(uv, p);
	}
};

struct image_texture : public texture {
	stb_image image;
	image_texture(const std::string &image_path) : image(image_path) {}
	inline vec3 color(vec2 uv, vec3 p) const override {
		// If we have no texture data, then return solid cyan as a debugging aid.
		if (image.height <= 0) {
			return vec3(0, 1, 1);
		}
		// Clamp input texture coordinates to [0,1] x [1,0]
		float u = clamp(uv.x, 0.f, 1.f);
		float v = 1.0 - clamp(uv.y, 0.f, 1.f); // Flip V to image coordinates
		auto i = int(u * image.width);
		auto j = int(v * image.height);
		auto pixel = image.color(i, j);
		auto color_scale = 1.0 / 255.0;
		return vec3(color_scale * pixel.x, color_scale * pixel.y, color_scale * pixel.z);
	}
};

struct noise_texture : public texture {
	perlin noise;
	float scale;
	inline noise_texture(float scale_ = 1.f) : scale(scale_) {}
	inline vec3 color(vec2 uv, vec3 p) const override {
		// Notes: 什么原理?
		return vec3(0.5 * (1.0 + sin(scale * p.z + 10.0 * noise.turbulence(p, 7))));
		// return vec3(0.5 * (noise.noise_smooth_random(p * scale) + 1.0));
	}
};