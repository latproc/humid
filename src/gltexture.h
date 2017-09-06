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

#ifndef latprocc_GLTexture_h
#define latprocc_GLTexture_h

#include <ostream>
#include <string>
#include <memory>
#include <nanogui/opengl.h>
#include <nanogui/glutil.h>

class GLTexture {
public:
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const GLTexture &other);
    
	using handleType = std::unique_ptr<uint8_t[], void(*)(void*)>;

	GLTexture() = default;
	GLTexture(const std::string& textureName);

	GLTexture(const std::string& textureName, GLint textureId);

	GLTexture(const GLTexture& other) = delete;
	GLTexture(GLTexture&& other) noexcept;
	GLTexture& operator=(const GLTexture& other) = delete;
	GLTexture& operator=(GLTexture&& other) noexcept;
	~GLTexture() noexcept;

	GLuint texture() const { return mTextureId; }
	const std::string& textureName() const { return mTextureName; }

	/* detach this object from the GL texture so that this data can be removed without 
		calling glDeleteTextures()
	 */
	void detach() { mTextureId = 0; }

	/**
	 *  Load a file in memory and create an OpenGL texture.
	 *  Returns a handle type (an std::unique_ptr) to the loaded pixels.
	 */
	handleType load(const std::string& fileName);
private:
	std::string mTextureName;
	GLuint mTextureId;
};

std::ostream &operator<<(std::ostream &out, const GLTexture &m);

#endif
