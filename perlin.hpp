#pragma once
#include <array>
#include "random.hpp"

// Notes: 原理?
struct perlin {
	static constexpr int point_count = 256;
	random_generator generator;
	std::array<vec3, point_count> randvecs;
	std::array<float, point_count> randfloats;
	std::array<int, point_count> perm_x, perm_y, perm_z;

	inline perlin() {
		for (int i = 0; i < point_count; ++i) {
			randfloats[i] = generator.random_float();
			randvecs[i] = generator.random_unit_vec3();
		}
		for (int i = 0; i < point_count; ++i) {
			perm_x[i] = perm_y[i] = perm_z[i] = i;
		}
		for (int i = point_count - 1; i > 0; --i) {
			std::swap(perm_x[i], perm_x[generator.random_int(0, i)]);
			std::swap(perm_y[i], perm_y[generator.random_int(0, i)]);
			std::swap(perm_z[i], perm_z[generator.random_int(0, i)]);
		}
	}
	inline float turbulence(vec3 p, int depth) const {
        // Notes: 不同频率噪声的混合
		float accum = 0.0, weight = 1.0;
		for (int i = 0; i < depth; ++i) {
			accum += weight * noise_smooth_random(p);
			weight *= 0.5, p *= 2.f;
		}
		return abs(accum);
	}
	inline float noise_smooth_random(vec3 p) const {
		int i = floor(p.x), j = floor(p.y), k = floor(p.z);
		float u = p.x - i, v = p.y - j, w = p.z - k, accum = 0.0;
		float uu = u * u * (3 - 2 * u);
		float vv = v * v * (3 - 2 * v);
		float ww = w * w * (3 - 2 * w);
		// Notes: 没有完全理解原理
		for (int di = 0; di < 2; di++) {
			for (int dj = 0; dj < 2; dj++) {
				for (int dk = 0; dk < 2; dk++) {
					vec3 dir = randvecs[perm_x[(i + di) & 255] ^ perm_y[(j + dj) & 255] ^
										perm_z[(k + dk) & 255]];
					accum += (di * uu + (1 - di) * (1 - uu)) * (dj * vv + (1 - dj) * (1 - vv)) *
							 (dk * ww + (1 - dk) * (1 - ww)) *
							 dot(dir, vec3(u - di, v - dj, w - dk));
				}
			}
		}
		return accum;
	}
	inline float noise_smooth(vec3 p) const {
		int i = floor(p.x), j = floor(p.y), k = floor(p.z);
		float u = p.x - i, v = p.y - j, w = p.z - k, accum = 0.0;
		u = u * u * (3 - 2 * u);
		v = v * v * (3 - 2 * v);
		w = w * w * (3 - 2 * w);
		// Notes: 没有完全理解原理
		for (int di = 0; di < 2; di++) {
			for (int dj = 0; dj < 2; dj++) {
				for (int dk = 0; dk < 2; dk++) {
					accum += (di * u + (1 - di) * (1 - u)) * (dj * v + (1 - dj) * (1 - v)) *
							 (dk * w + (1 - dk) * (1 - w)) *
							 randfloats[perm_x[(i + di) & 255] ^ perm_y[(j + dj) & 255] ^
										perm_z[(k + dk) & 255]];
				}
			}
		}
		return accum;
	}
	inline float noise_simple(vec3 p) const {
		// Notes: 周期性的噪声
		int i = int(4 * p.x) & 255;
		int j = int(4 * p.y) & 255;
		int k = int(4 * p.z) & 255;
		return randfloats[perm_x[i] ^ perm_y[j] ^ perm_z[k]];
	}
};