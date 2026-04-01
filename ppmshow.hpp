#pragma once
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include "raytracing.hpp"

// RAII 自动释放 SDL 资源
struct SDL_Deleter {
	void operator()(SDL_Window *w) const {
		if (w)
			SDL_DestroyWindow(w);
	}
	void operator()(SDL_Renderer *r) const {
		if (r)
			SDL_DestroyRenderer(r);
	}
	void operator()(SDL_Texture *t) const {
		if (t)
			SDL_DestroyTexture(t);
	}
};

/**
 * ppm_show: 接收 PPM 格式的字符串并显示
 * 返回 0 表示正常退出
 */

inline int ppmshow(const image2d &img) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return -1;

	std::unique_ptr<SDL_Window, SDL_Deleter> window(
		SDL_CreateWindow("Raytracing Preview", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
						 img.width, img.height, SDL_WINDOW_SHOWN));

	std::unique_ptr<SDL_Renderer, SDL_Deleter> renderer(
		SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED));

	// 直接创建 RGB24 纹理 (对应 vec<3, unsigned char>)
	std::unique_ptr<SDL_Texture, SDL_Deleter> texture(SDL_CreateTexture(
		renderer.get(), SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, img.width, img.height));

	// 关键步骤：直接将 data.data() 内存拷贝到纹理
	// Pitch (字节跨度) = 宽度 * 每个像素的字节数 (3)
	SDL_UpdateTexture(texture.get(), nullptr, img.data.data(), img.width * 3);

	bool running = true;
	SDL_Event e;
	while (running) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				running = false;
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
				running = false;
		}
		SDL_RenderClear(renderer.get());
		SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
		SDL_RenderPresent(renderer.get());
		SDL_Delay(16);
	}

	SDL_Quit();
	return 0;
}

inline int ppmshow_and_write(const image2d &img, const std::string &file_name) {
	stbi_write_png(file_name.c_str(), img.width, img.height, 3, img.data.data(), 3 * img.width);
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return -1;

	std::unique_ptr<SDL_Window, SDL_Deleter> window(
		SDL_CreateWindow("Raytracing Preview", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
						 img.width, img.height, SDL_WINDOW_SHOWN));

	std::unique_ptr<SDL_Renderer, SDL_Deleter> renderer(
		SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED));

	// 直接创建 RGB24 纹理 (对应 vec<3, unsigned char>)
	std::unique_ptr<SDL_Texture, SDL_Deleter> texture(SDL_CreateTexture(
		renderer.get(), SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, img.width, img.height));

	// 关键步骤：直接将 data.data() 内存拷贝到纹理
	// Pitch (字节跨度) = 宽度 * 每个像素的字节数 (3)
	SDL_UpdateTexture(texture.get(), nullptr, img.data.data(), img.width * 3);

	bool running = true;
	SDL_Event e;
	while (running) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				running = false;
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
				running = false;
		}
		SDL_RenderClear(renderer.get());
		SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
		SDL_RenderPresent(renderer.get());
		SDL_Delay(16);
	}
	SDL_Quit();
	return 0;
}

inline int ppmshow(const std::string &ppm_source_string) {
	std::stringstream ss(ppm_source_string);
	std::string magic_number;
	int width, height, max_color;

	// 1. 解析 PPM Header (仅支持 P3 ASCII 格式，方便调试)
	ss >> magic_number >> width >> height >> max_color;
	if (magic_number != "P3") {
		std::cerr << "Error: Only P3 PPM string is supported in this wrapper." << std::endl;
		return -1;
	}

	// 2. 将字符串数据转换为像素数组
	std::vector<uint8_t> pixels;
	pixels.reserve(width * height * 3);
	int r, g, b;
	while (ss >> r >> g >> b) {
		pixels.push_back(static_cast<uint8_t>(r));
		pixels.push_back(static_cast<uint8_t>(g));
		pixels.push_back(static_cast<uint8_t>(b));
	}

	// 3. 初始化 SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return -1;

	// 使用智能指针确保函数返回时自动销毁窗口和渲染器
	std::unique_ptr<SDL_Window, SDL_Deleter> window(
		SDL_CreateWindow("Raytracing Preview", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
						 width, height, SDL_WINDOW_SHOWN));

	std::unique_ptr<SDL_Renderer, SDL_Deleter> renderer(
		SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED));

	// 4. 创建纹理并上传数据
	std::unique_ptr<SDL_Texture, SDL_Deleter> texture(SDL_CreateTexture(
		renderer.get(), SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, width, height));

	SDL_UpdateTexture(texture.get(), nullptr, pixels.data(), width * 3);

	// 5. 事件循环：阻塞直到 ESC 或关闭窗口
	bool running = true;
	SDL_Event e;
	while (running) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				running = false;
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
				running = false;
		}

		SDL_RenderClear(renderer.get());
		SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
		SDL_RenderPresent(renderer.get());

		SDL_Delay(16); // 限制帧率，减少 CPU 占用
	}

	SDL_Quit();
	return 0;
}