#pragma once

#include "core/pch.h"

// TODO: R12G12B12, R5G6B5, etc
enum class TextureFormat {
	// 32-BIT COMPONENTS
	R32_G32_B32_A32_FLOAT,// GL_RGBA32F
	R32_G32_B32_A32_UINT,// GL_RGBA32UI
	R32_G32_B32_A32_SINT,// GL_RGBA32I

	R32_G32_B32_FLOAT,// GL_RGB32F
	R32_G32_B32_UINT,// GL_RGB32UI
	R32_G32_B32_SINT,// GL_RGB32I

	R32_G32_FLOAT,// GL_RG32F
	R32_G32_UINT,// GL_RG32UI
	R32_G32_SINT,// GL_RG32I

	R32_FLOAT,// GL_R32F
	R32_UINT,// GL_R32UI
	R32_SINT,// GL_R32I
	DEPTH32_FLOAT, // GL_DEPTH_COMPONENT32F
	DEPTH32_SNORM, // GL_DEPTH_COMPONENT32

	DEPTH24_SNORM, // GL_DEPTH_COMPONENT24

	// 16-BIT COMPONENTS
	R16_G16_B16_A16_FLOAT,// GL_RGBA16F
	R16_G16_B16_A16_UINT,// GL_RGBA16UI
	R16_G16_B16_A16_SINT,// GL_RGBA16I
	R16_G16_B16_A16_UNORM,// GL_RGBA16
	R16_G16_B16_A16_SNORM,// GL_RGBA16_SNORM

	R16_G16_B16_FLOAT,// GL_RGB16F
	R16_G16_B16_UINT,// GL_RGB16UI
	R16_G16_B16_SINT,// GL_RGB16I
	R16_G16_B16_UNORM,// GL_RGB16
	R16_G16_B16_SNORM,// GL_RGB16_SNORM

	R16_G16_FLOAT,// GL_RG16F
	R16_G16_UINT,// GL_RG16UI
	R16_G16_SINT,// GL_RG16I
	R16_G16_UNORM,// GL_RG16
	R16_G16_SNORM,// GL_RG16_SNORM

	R16_FLOAT,// GL_R16F
	R16_UINT,// GL_R16UI
	R16_SINT,// GL_R16I
	R16_UNORM,// GL_R16
	R16_SNORM,// GL_R16_SNORM
	DEPTH16_SNORM, // GL_DEPTH_COMPONENT16

	// 8-BIT COMPONENTS
	R8_G8_B8_A8_UINT,// GL_RGBA8UI
	R8_G8_B8_A8_SINT,// GL_RGBA8I
	R8_G8_B8_A8_UNORM,// GL_RGBA8
	R8_G8_B8_A8_SNORM,// GL_RGBA8_SNORM

	R8_G8_B8_UINT,// GL_RGB8UI
	R8_G8_B8_SINT,// GL_RGB8I
	R8_G8_B8_UNORM,// GL_RGB8
	R8_G8_B8_SNORM,// GL_RGB8_SNORM

	R8_G8_UINT,// GL_RG8UI
	R8_G8_SINT,// GL_RG8I
	R8_G8_UNORM,// GL_RG8
	R8_G8_SNORM,// GL_RG8_SNORM

	R8_UINT,// GL_R8UI
	R8_SINT,// GL_R8I
	R8_UNORM,// GL_R8
	R8_SNORM,// GL_R8_SNORM

	DEFAULT_RGBA = R8_G8_B8_A8_UNORM,
	DEFAULT_RGB = R8_G8_B8_UNORM,
	DEFAULT_RG = R8_G8_UNORM,
	DEFAULT_R = R8_UNORM,
	DEFAULT_GRAYSCALE = R8_UNORM,
	DEFAULT = DEFAULT_RGBA,
} ;

enum class TextureWrap {
	CLAMP_TO_EDGE, // Coordinates outside the range [0,1] are clamped to this range, causing the last pixel to be repeated.
	MIRRORED_REPEAT, // Coordinates outside the range [0,1] are repeated, but also flipped when the integer component is odd.
	REPEAT, // Coordinates outside the range [0,1] are repeated. The integer part of the coordinate is simply ignored.

	DEFAULT = REPEAT
};


enum class TextureFilter {
	NEAREST_PIXEL, // Uses the colour of the nearest pixel center to the sampled coordinate.
	LINEAR_PIXEL, // Linearly interpolates between the four pixels surrounding the sampled coordinate.
	NEAREST_MIPMAP_NEAREST_PIXEL, // Chooses the nearest mipmap to the size of the sampled pixel, and uses NEAREST filtering at the sample coordinate.
	NEAREST_MIPMAP_LINEAR_PIXEL, // Chooses the nearest mipmap to the size of the sampled pixel, and uses LINEAR filtering at the sample coordinate.
	LINEAR_MIPMAP_NEAREST_PIXEL, // Chooses the two nearest mipmaps to the size of the sampled pixel, samples both using NEAREST filtering, and linearly interpolates between the sample result for both mipmaps.
	LINEAR_MIPMAP_LINEAR_PIXEL, // Chooses the two nearest mipmaps to the size of the sampled pixel, samples both using LINEAR filtering, and linearly interpolates between the sample result for both mipmaps.

	DEFAULT = LINEAR_PIXEL
};

enum class TextureTarget {
	TEXTURE_1D,
	TEXTURE_1D_ARRAY,
	TEXTURE_2D,
	TEXTURE_2D_ARRAY,
	TEXTURE_3D,
	TEXTURE_CUBEMAP,
	TEXTURE_CUBEMAP_POSITIVE_X,
	TEXTURE_CUBEMAP_NEGATIVE_X,
	TEXTURE_CUBEMAP_POSITIVE_Y,
	TEXTURE_CUBEMAP_NEGATIVE_Y,
	TEXTURE_CUBEMAP_POSITIVE_Z,
	TEXTURE_CUBEMAP_NEGATIVE_Z,

	DEFAULT = TEXTURE_2D
};

struct OpenGLTextureFormat {
	uint32_t internalFormat;
	uint32_t externalFormat;
	uint32_t type;

	OpenGLTextureFormat(uint32_t internalFormat = GL_NONE, uint32_t externalFormat = GL_NONE, uint32_t type = GL_NONE) :
		internalFormat(internalFormat),
		externalFormat(externalFormat),
		type(type) {
	}
};

struct OpenGLTextureFilter {
	uint32_t minFilter;
	uint32_t magFilter;
	bool mipmap;

	OpenGLTextureFilter(uint32_t minFilter = GL_NONE, uint32_t magFilter = GL_NONE, bool mipmap = false) :
		minFilter(minFilter),
		magFilter(magFilter),
		mipmap(mipmap) {
	}
};

struct OpenGLTextureTarget {
	uint32_t target;

	OpenGLTextureTarget(uint32_t target = GL_NONE):
		target(target) {
	}
};

struct OpenGLTextureWrap {
	uint32_t u;
	uint32_t v;
	uint32_t w;

	OpenGLTextureWrap(uint32_t u = GL_NONE, uint32_t v = GL_NONE, uint32_t w = GL_NONE):
		u(u),
		v(v),
		w(w) {
	}
};

// TODO: cache bound textures and ignore repeated bind calls.

class Texture {
public:
	Texture(TextureTarget target = TextureTarget::DEFAULT, TextureFormat format = TextureFormat::DEFAULT, TextureFilter minFilter = TextureFilter::DEFAULT, TextureFilter magFilter = TextureFilter::DEFAULT);

	~Texture();
	
	//virtual void upload(void* data, uint32_t width, uint32_t height, uint32_t left, uint32_t top) = 0;

	virtual void bind(uint32_t texture = 0) = 0;

	virtual void unbind() = 0;

	virtual uint64_t getMemorySize() const = 0;

	virtual bool setFilterMode(TextureFilter minFilter, TextureFilter magFilter, float anisotropy = 1.0F);

	void generateMipmap(int32_t levels = -1);

	TextureTarget getTarget() const;

	TextureFormat getFormat() const;

	TextureFilter getMinificationFilterMode() const;

	TextureFilter getMagnificationFilterMode() const;

	uint32_t getHandle() const;

	bool isMipmapEnabled() const;

	bool isAnisotropicFilteringEnabled() const;

	float getAnisotropy() const;

	static OpenGLTextureFormat getOpenGLTextureFormat(TextureFormat textureFormat, uint32_t externalFormat = GL_NONE);

	static OpenGLTextureFilter getOpenGLTextureFilter(TextureFilter minFilter, TextureFilter magFilter);

	static OpenGLTextureTarget getOpenGLTextureTarget(TextureTarget textureTarget);

	static OpenGLTextureWrap getOpenGLTextureWrap(TextureWrap u, TextureWrap v = TextureWrap::DEFAULT, TextureWrap w = TextureWrap::DEFAULT);

	static uint64_t getPixelSize(TextureFormat textureFormat);

protected:
	Texture(uint32_t handle, TextureTarget target = TextureTarget::DEFAULT, TextureFormat format = TextureFormat::DEFAULT, TextureFilter minFilter = TextureFilter::DEFAULT, TextureFilter magFilter = TextureFilter::DEFAULT);

	TextureTarget m_target;
	TextureFormat m_format;
	TextureFilter m_minFilter;
	TextureFilter m_magFilter;
	uint32_t m_handle;
	bool m_mipmapEnabled;
	float m_anisotropy;
};

class Texture2D : public Texture {
public:
	Texture2D(uint32_t width, uint32_t height, TextureFormat format = TextureFormat::DEFAULT, TextureFilter minFilter = TextureFilter::DEFAULT, TextureFilter magFilter = TextureFilter::DEFAULT, TextureWrap uWrap = TextureWrap::DEFAULT, TextureWrap vWrap = TextureWrap::DEFAULT);

	~Texture2D();

	static bool load(std::string filePath, Texture2D** dstTexturePtr, bool verticalFlip = true);

	virtual void upload(void* data, uint32_t width = 0, uint32_t height = 0, uint32_t left = 0, uint32_t top = 0);

	virtual void bind(uint32_t textureUnit = 0) override;

	virtual void unbind() override;

	virtual uint64_t getMemorySize() const;

	void setSize(uint32_t width, uint32_t height, uint32_t externalFormat = GL_NONE);

	//void resize(uint32_t width, uint32_t height, uint32_t left = 0, uint32_t top = 0);

	bool setWrapMode(TextureWrap u, TextureWrap v);

	TextureWrap getUWrapFilterMode() const;

	TextureWrap getVWrapFilterMode() const;

	uint32_t getWidth() const;

	uint32_t getHeight() const;

protected:
	Texture2D(uint32_t handle, uint32_t width, uint32_t height, TextureTarget target = TextureTarget::TEXTURE_2D, TextureFormat format = TextureFormat::DEFAULT, TextureFilter minFilter = TextureFilter::DEFAULT, TextureFilter magFilter = TextureFilter::DEFAULT, TextureWrap uWrap = TextureWrap::DEFAULT, TextureWrap vWrap = TextureWrap::DEFAULT);

	TextureWrap m_uWrap;
	TextureWrap m_vWrap;
	uint32_t m_width;
	uint32_t m_height;
};

class Texture2DArray : public Texture {
public:
	Texture2DArray(uint32_t width, uint32_t height, uint32_t layers, TextureFormat format = TextureFormat::DEFAULT, TextureFilter minFilter = TextureFilter::DEFAULT, TextureFilter magFilter = TextureFilter::DEFAULT, TextureWrap uWrap = TextureWrap::DEFAULT, TextureWrap vWrap = TextureWrap::DEFAULT);

	~Texture2DArray();

	virtual void upload(void* data, uint32_t depth, uint32_t width, uint32_t height, uint32_t layer, uint32_t left = 0, uint32_t top = 0);
	
	virtual void bind(uint32_t textureUnit = 0) override;
	
	virtual void unbind() override;

	virtual uint64_t getMemorySize() const;

	void setSize(uint32_t width, uint32_t height, uint32_t layers, uint32_t externalFormat = GL_NONE);

	bool setWrapMode(TextureWrap u, TextureWrap v);

	TextureWrap getUWrapFilterMode() const;

	TextureWrap getVWrapFilterMode() const;

	uint32_t getWidth() const;

	uint32_t getHeight() const;

	uint32_t getLayers() const;

protected:
	TextureWrap m_uWrap;
	TextureWrap m_vWrap;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_layers;
};

