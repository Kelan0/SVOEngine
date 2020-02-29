#pragma once

#include "core/pch.h"
#include "core/renderer/Texture.h"

class ShaderProgram;

enum class CubeMapFace {
	POSITIVE_X = 0,
	NEGATIVE_X = 1,
	POSITIVE_Y = 2,
	NEGATIVE_Y = 3,
	POSITIVE_Z = 4,
	NEGATIVE_Z = 5,

	LEFT = NEGATIVE_X,
	RIGHT = POSITIVE_X,
	TOP = POSITIVE_Y,
	BOTTOM = NEGATIVE_Y,
	FRONT = NEGATIVE_Z,
	BACK = POSITIVE_Z,
};

class CubeMap;

struct CubemapConfiguration {
	// equirectangular image takes priority. If left empty, face files are attempted
	std::string rightFilePath = "";
	std::string leftFilePath = "";
	std::string topFilePath = "";
	std::string bottomFilePath = "";
	std::string backFilePath = "";
	std::string frontFilePath = "";
	std::string equirectangularFilePath = "";

	bool floatingPoint = false;
	bool verticallyFlip = false;
};

class CubeMap : public Texture {
	friend class CubeMapArray;
public:
	CubeMap(uint32_t width, uint32_t height, TextureFormat format = TextureFormat::DEFAULT, TextureFilter minFilter = TextureFilter::DEFAULT, TextureFilter magFilter = TextureFilter::DEFAULT, TextureWrap uWrap = TextureWrap::MIRRORED_REPEAT, TextureWrap vWrap = TextureWrap::MIRRORED_REPEAT, TextureWrap wWrap = TextureWrap::MIRRORED_REPEAT);

	~CubeMap();

	static bool load(CubemapConfiguration config, CubeMap** dstCubemapPtr);

	static bool loadEquirectangular(CubemapConfiguration config, CubeMap** dstCubemapPtr);

	static bool loadFaces(CubemapConfiguration config, CubeMap** dstCubemapPtr);

	// Upload equirectangular data
	virtual void upload(void* data, uint32_t width = 0, uint32_t height = 0);

	// Upload individual data for individual faces. NULL data will be ignored.
	virtual void upload(void* data[6], uint32_t width = 0, uint32_t height = 0);

	virtual void bind(uint32_t textureUnit = 0) override;

	virtual void unbind() override;

	virtual uint64_t getMemorySize() const;

	void setSize(uint32_t width, uint32_t height, uint32_t externalFormat = GL_NONE);

	bool setWrapMode(TextureWrap u, TextureWrap v, TextureWrap w);

	TextureWrap getUWrapFilterMode() const;

	TextureWrap getVWrapFilterMode() const;

	TextureWrap getWWrapFilterMode() const;

	uint32_t getWidth() const;

	uint32_t getHeight() const;

	static TextureTarget getFaceTarget(CubeMapFace index);

	static OpenGLTextureTarget getOpenGLFaceTarget(CubeMapFace index);
private:
	static void uploadEquirectangular(Texture* texture, void* data, uint32_t width = 0, uint32_t height = 0);

	static void uploadCubeFaces(Texture* texture, void* data[6], uint32_t width = 0, uint32_t height = 0);

	uint32_t m_width;
	uint32_t m_height;
	TextureWrap m_uWrap;
	TextureWrap m_vWrap;
	TextureWrap m_wWrap;

	static ShaderProgram* s_equirectangularShader;
};

class CubeMapArray : public Texture {
public:
	CubeMapArray(uint32_t width, uint32_t height, uint32_t layers, TextureFormat format = TextureFormat::DEFAULT, TextureFilter minFilter = TextureFilter::DEFAULT, TextureFilter magFilter = TextureFilter::DEFAULT, TextureWrap uWrap = TextureWrap::MIRRORED_REPEAT, TextureWrap vWrap = TextureWrap::MIRRORED_REPEAT, TextureWrap wWrap = TextureWrap::MIRRORED_REPEAT);

	~CubeMapArray();

	// Upload equirectangular data for each layer.
	virtual void upload(std::vector<void*> data, uint32_t width = 0, uint32_t height = 0);

	// Upload equirectangular data to layer
	virtual void upload(void* data, uint32_t layer, uint32_t width = 0, uint32_t height = 0);

	// Upload individual data for individual faces for each layer.
	virtual void upload(std::vector<void*> data[6], uint32_t width = 0, uint32_t height = 0);

	// Upload individual data for individual faces to layer.
	virtual void upload(void* data[6], uint32_t layer, uint32_t width = 0, uint32_t height = 0);

	virtual void bind(uint32_t textureUnit = 0) override;

	virtual void unbind() override;

	virtual uint64_t getMemorySize() const;

	void setSize(uint32_t width, uint32_t height, uint32_t layers, uint32_t externalFormat = GL_NONE);

	bool setWrapMode(TextureWrap u, TextureWrap v, TextureWrap w);

	TextureWrap getUWrapFilterMode() const;

	TextureWrap getVWrapFilterMode() const;

	TextureWrap getWWrapFilterMode() const;

	uint32_t getWidth() const;

	uint32_t getHeight() const;

	uint32_t getLayers() const;
private:
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_layers;
	TextureWrap m_uWrap;
	TextureWrap m_vWrap;
	TextureWrap m_wWrap;

};