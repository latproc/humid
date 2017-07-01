/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
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
    GLTexture(const char *msg);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const GLTexture &other);
    
	using handleType = std::unique_ptr<uint8_t[], void(*)(void*)>;

	GLTexture() = default;
	GLTexture(const std::string& textureName)
	: mTextureName(textureName), mTextureId(0) {}

	GLTexture(const std::string& textureName, GLint textureId)
	: mTextureName(textureName), mTextureId(textureId) {}

	GLTexture(const GLTexture& other) = delete;
	GLTexture(GLTexture&& other) noexcept
	: mTextureName(std::move(other.mTextureName)),
	mTextureId(other.mTextureId) {
		other.mTextureId = 0;
	}
	GLTexture& operator=(const GLTexture& other) = delete;
	GLTexture& operator=(GLTexture&& other) noexcept {
		mTextureName = std::move(other.mTextureName);
		std::swap(mTextureId, other.mTextureId);
		return *this;
	}
	~GLTexture() noexcept {
		if (mTextureId)
			glDeleteTextures(1, &mTextureId);
			}

	GLuint texture() const { return mTextureId; }
	const std::string& textureName() const { return mTextureName; }

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
