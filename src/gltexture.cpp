/*
	This code was largely taken from nanogui example1.cpp:

    src/example1.cpp -- C++ version of an example application that shows
    how to use the various widget classes. For a Python implementation, see
    '../python/example1.py'.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <iostream>
#include <boost/filesystem.hpp>
#include "gltexture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

GLTexture::GLTexture(const std::string& textureName)
: mTextureName(textureName), mTextureId(0) {}

GLTexture::GLTexture(const std::string& textureName, GLint textureId)
: mTextureName(textureName), mTextureId(textureId) {}

GLTexture::GLTexture(GLTexture&& other) noexcept
: mTextureName(std::move(other.mTextureName)),
mTextureId(other.mTextureId) {
	other.mTextureId = 0;
}
GLTexture& GLTexture::operator=(GLTexture&& other) noexcept {
	mTextureName = std::move(other.mTextureName);
	std::swap(mTextureId, other.mTextureId);
	return *this;
}
GLTexture::~GLTexture() noexcept {
	if (mTextureId)
		glDeleteTextures(1, &mTextureId);
}


GLTexture::handleType GLTexture::load(const std::string& fileName) {
	using namespace boost::filesystem;
	if (mTextureId) {
		//glDeleteTextures(1, &mTextureId);
		mTextureId = 0;
	}
	path filepath(fileName);
	if (!is_regular_file(filepath)) throw std::invalid_argument("No such image file " + fileName);
	int force_channels = 0;
	int w, h, n;
	// Force RGB(A) so row stride is well-defined; keep tight packing for GL.
	// Default GL_UNPACK_ALIGNMENT is 4; RGB widths not divisible by 4 (e.g. 1754)
	// otherwise shear/corrupt the texture (wiring A4 pages often hit this).
	force_channels = 4;
	handleType textureData(stbi_load(fileName.c_str(), &w, &h, &n, force_channels), stbi_image_free);
	if (!textureData)
		throw std::invalid_argument("Could not load texture data from file " + fileName);
	glGenTextures(1, &mTextureId);
	glBindTexture(GL_TEXTURE_2D, mTextureId);
	GLint prev_align = 4;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_align);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData.get());
	glPixelStorei(GL_UNPACK_ALIGNMENT, prev_align);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	return textureData;
}
