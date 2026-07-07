/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include "screencapture.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include <nanogui/opengl.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../lib/nanogui/ext/nanovg/example/stb_image_write.h"

static bool writePixelsToPng(const std::string &filename, int width, int height, const unsigned char *data) {
	const int row_stride = width * 4;
	std::vector<unsigned char> flipped(width * height * 4);
	for (int y = 0; y < height; ++y) {
		const int src_y = height - 1 - y;
		std::copy(
			data + src_y * row_stride,
			data + (src_y + 1) * row_stride,
			flipped.begin() + y * row_stride
		);
	}

	if (!stbi_write_png(filename.c_str(), width, height, 4, flipped.data(), row_stride)) {
		std::cerr << "Capture failed: could not write PNG " << filename << "\n";
		return false;
	}

	std::cout << "Wrote capture to " << filename << "\n";
	return true;
}

bool writeFramebufferToPng(const std::string &filename, int width, int height) {
	if (width <= 0 || height <= 0) {
		std::cerr << "Capture failed: invalid framebuffer size " << width << "x" << height << "\n";
		return false;
	}

	std::vector<unsigned char> pixels(width * height * 4);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

	if (glGetError() != GL_NO_ERROR) {
		std::cerr << "Capture failed: glReadPixels returned an OpenGL error\n";
		return false;
	}

	return writePixelsToPng(filename, width, height, pixels.data());
}

bool writeFramebufferRegionToPng(const std::string &filename, int x, int y, int width, int height) {
	if (x < 0 || y < 0 || width <= 0 || height <= 0) {
		std::cerr << "Capture failed: invalid framebuffer region " << x << "," << y << " " << width << "x" << height << "\n";
		return false;
	}

	std::vector<unsigned char> pixels(width * height * 4);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer(GL_BACK);
	glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

	if (glGetError() != GL_NO_ERROR) {
		std::cerr << "Capture failed: glReadPixels returned an OpenGL error\n";
		return false;
	}

	return writePixelsToPng(filename, width, height, pixels.data());
}
