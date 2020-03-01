#include "CubeMap.h"
#include "core/Engine.h"
#include "core/renderer/ShaderProgram.h"
#include "core/ResourceHandler.h"
#include "core/util/FileUtils.h"

ShaderProgram* CubeMap::s_equirectangularShader = NULL;

CubeMap::CubeMap(uint32_t width, uint32_t height, TextureFormat format, TextureFilter minFilter, TextureFilter magFilter, TextureWrap uWrap, TextureWrap vWrap, TextureWrap wWrap):
	Texture(TextureTarget::TEXTURE_CUBEMAP, format, minFilter, magFilter),
	m_width(width),
	m_height(height) {

	this->setSize(width, height);
	this->setWrapMode(uWrap, vWrap, wWrap);
	this->setFilterMode(minFilter, magFilter);
}

CubeMap::~CubeMap() {
	Texture::~Texture();
}

bool CubeMap::load(CubemapConfiguration config, CubeMap** dstCubemapPtr) {
	if (!config.equirectangularFilePath.empty()) {
		return CubeMap::loadEquirectangular(config, dstCubemapPtr);
	} else {
		return CubeMap::loadFaces(config, dstCubemapPtr);
	}
}

bool CubeMap::loadEquirectangular(CubemapConfiguration config, CubeMap** dstCubemapPtr) {
	if (config.equirectangularFilePath.empty()) {
		return false;
	}

	if (dstCubemapPtr == NULL) {
		return false;
	}

	Image image;
	if (!FileUtils::loadImage(config.equirectangularFilePath, image, config.verticallyFlip, config.floatingPoint)) {
		info("Failed to load image file \"%s\"\n", config.equirectangularFilePath.c_str());
		return false;
	}

	//std::vector<float> floats(data);
	TextureFormat format = config.floatingPoint ? TextureFormat::R32_G32_B32_FLOAT : TextureFormat::DEFAULT_RGB;
	switch (image.channels) {
		case 1: format = config.floatingPoint ? TextureFormat::R32_FLOAT : TextureFormat::DEFAULT_GRAYSCALE; break;
		case 2: format = config.floatingPoint ? TextureFormat::R32_G32_FLOAT : TextureFormat::DEFAULT_RG; break;
		case 3: format = config.floatingPoint ? TextureFormat::R32_G32_B32_FLOAT : TextureFormat::DEFAULT_RGB; break;
		case 4: format = config.floatingPoint ? TextureFormat::R32_G32_B32_A32_FLOAT : TextureFormat::DEFAULT_RGBA; break;
		default:
			error("Loaded image with unsupported channel configuration\n");
			return false;
	}

	CubeMap* dstCubemap = *dstCubemapPtr;
	uint32_t faceWidth = image.width / 4;
	uint32_t faceHeight = image.height / 2;

	if (dstCubemap == NULL) {
		*dstCubemapPtr = new CubeMap(faceWidth, faceHeight, format);
		dstCubemap = *dstCubemapPtr;
	}

	//if (dstCubemap->getWidth() < faceWidth || dstCubemap->getHeight() < faceHeight) {
	dstCubemap->setSize(faceWidth, faceHeight, GL_RGBA);
	//}

	dstCubemap->upload(image.data, image.width, image.height);
	return true;
}

bool CubeMap::loadFaces(CubemapConfiguration config, CubeMap** dstCubemapPtr) {
	std::string filePaths[6];
	filePaths[(int)CubeMapFace::RIGHT] = config.rightFilePath;
	filePaths[(int)CubeMapFace::LEFT] = config.leftFilePath;
	filePaths[(int)CubeMapFace::TOP] = config.topFilePath;
	filePaths[(int)CubeMapFace::BOTTOM] = config.bottomFilePath;
	filePaths[(int)CubeMapFace::BACK] = config.backFilePath;
	filePaths[(int)CubeMapFace::FRONT] = config.frontFilePath;

	if (dstCubemapPtr == NULL) {
		return false;
	}

	uint32_t faceWidth;
	uint32_t faceHeight;
	uint32_t channels;
	Image* faceImages[6];
	void* data[6];

	for (int i = 0; i < 6; i++) {
		faceImages[i] = new Image();

		if (filePaths[i].empty()) {
			info("Could not load cubemap - face %d was not specified\n", i);
			for (int j = 0; j <= i; j++) delete faceImages[j];
			return false;
		}

		if (!FileUtils::loadImage(filePaths[i], *faceImages[i], config.verticallyFlip, config.floatingPoint)) {
			info("Failed to load image file \"%s\"\n", filePaths[i].c_str());
			info("Could not load cubemap - face %d was not found\n", i);
			for (int j = 0; j <= i; j++) delete faceImages[j];
			return false;
		}

		data[i] = faceImages[i]->data;

		if (i == 0) {
			faceWidth = faceImages[i]->width;
			faceHeight = faceImages[i]->height;
			channels = faceImages[i]->channels;
		} else {
			if (faceImages[i]->width != faceWidth || faceImages[i]->height != faceHeight || faceImages[i]->channels != channels) {
				info("Could not load cubemap - size of face %d does not match previous faces\n", i);
				for (int j = 0; j <= i; j++) delete faceImages[j];
				return false;
			}
		}
	}

	TextureFormat format = TextureFormat::DEFAULT_RGB;
	switch (channels) {
		case 1: format = TextureFormat::DEFAULT_GRAYSCALE; break;
		case 2: format = TextureFormat::DEFAULT_RG; break;
		case 3: format = TextureFormat::DEFAULT_RGB; break;
		case 4: format = TextureFormat::DEFAULT_RGBA; break;
		default:
			error("Could not load cubemap - unsupported channel configuration\n");
			return false;
	}

	CubeMap* dstCubemap = *dstCubemapPtr;

	if (dstCubemap == NULL) {
		*dstCubemapPtr = new CubeMap(faceWidth, faceHeight, format);
		dstCubemap = *dstCubemapPtr;
	}

	if (dstCubemap->getWidth() < faceWidth || dstCubemap->getHeight() < faceHeight) {
		dstCubemap->setSize(faceWidth, faceHeight);
	}

	dstCubemap->upload(data, faceWidth, faceHeight);

	for (int j = 0; j < 6; j++) delete faceImages[j];
}

void CubeMap::upload(void* data, uint32_t width, uint32_t height) {
	CubeMap::uploadEquirectangular(this, data, width, height);
}

void CubeMap::upload(void* data[6], uint32_t width, uint32_t height) {
	CubeMap::uploadCubeFaces(this, data, width, height);
}

void CubeMap::bind(uint32_t textureUnit) {
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(target.target, m_textureName);
}

void CubeMap::unbind() {
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	glBindTexture(target.target, 0);
}

uint64_t CubeMap::getMemorySize() const {
	return Texture::getPixelSize(m_format) * m_width * m_height * 6;
}

void CubeMap::setSize(uint32_t width, uint32_t height, uint32_t externalFormat) {
	if (width != m_width || height != m_height || externalFormat != GL_NONE) {
		m_width = width;
		m_height = height;
		OpenGLTextureFormat format = Texture::getOpenGLTextureFormat(m_format, externalFormat);

		this->bind();
		for (int i = 0; i < 6; i++) {
			OpenGLTextureTarget target = CubeMap::getOpenGLFaceTarget((CubeMapFace)i);
			glTexImage2D(target.target, 0, format.internalFormat, width, height, 0, format.externalFormat, format.type, NULL);
		}
		this->unbind();
	}
}

bool CubeMap::setWrapMode(TextureWrap u, TextureWrap v, TextureWrap w) {
	if (u != m_uWrap || v != m_vWrap || w != m_wWrap) {
		OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
		OpenGLTextureWrap wrap = Texture::getOpenGLTextureWrap(u, v, w);
		if (wrap.u == GL_NONE || wrap.v == GL_NONE || wrap.w == GL_NONE) {
			return false;
		}

		m_uWrap = u;
		m_vWrap = v;
		m_wWrap = w;

		this->bind();
		glTexParameteri(target.target, GL_TEXTURE_WRAP_S, wrap.u);
		glTexParameteri(target.target, GL_TEXTURE_WRAP_T, wrap.v);
		glTexParameteri(target.target, GL_TEXTURE_WRAP_R, wrap.w);
		this->unbind();
	}

	return true;
}

//CubemapFaceTexture* CubeMap::getFaceTexture(CubeMapFace face) const {
//	return m_faces[(int) face];
//}

TextureWrap CubeMap::getUWrapFilterMode() const {
	return m_uWrap;
}

TextureWrap CubeMap::getVWrapFilterMode() const {
	return m_vWrap;
}

TextureWrap CubeMap::getWWrapFilterMode() const {
	return m_wWrap;
}

uint32_t CubeMap::getWidth() const {
	return m_width;
}

uint32_t CubeMap::getHeight() const {
	return m_height;
}

TextureTarget CubeMap::getFaceTarget(CubeMapFace index) {
	switch (index) {
		case CubeMapFace::POSITIVE_X: return TextureTarget::TEXTURE_CUBEMAP_POSITIVE_X;
		case CubeMapFace::NEGATIVE_X: return TextureTarget::TEXTURE_CUBEMAP_NEGATIVE_X;
		case CubeMapFace::POSITIVE_Y: return TextureTarget::TEXTURE_CUBEMAP_POSITIVE_Y;
		case CubeMapFace::NEGATIVE_Y: return TextureTarget::TEXTURE_CUBEMAP_NEGATIVE_Y;
		case CubeMapFace::POSITIVE_Z: return TextureTarget::TEXTURE_CUBEMAP_POSITIVE_Z;
		case CubeMapFace::NEGATIVE_Z: return TextureTarget::TEXTURE_CUBEMAP_NEGATIVE_Z;
		default: assert(false); // shouldnt be possible
	}
}

OpenGLTextureTarget CubeMap::getOpenGLFaceTarget(CubeMapFace index) {
	return Texture::getOpenGLTextureTarget(CubeMap::getFaceTarget(index));
}

void CubeMap::uploadEquirectangular(Texture* texture, void* data, uint32_t width, uint32_t height) {
	if (s_equirectangularShader == NULL) {
		s_equirectangularShader = new ShaderProgram();
		s_equirectangularShader->addShader(GL_COMPUTE_SHADER, "shaders/compute/equirectangular/comp.glsl");
		s_equirectangularShader->completeProgram();
	}

	OpenGLTextureFormat format = Texture::getOpenGLTextureFormat(texture->getFormat());

	uint32_t tempTextureHandle = 0; // Build temp texture storing the equirectangular data as RGBA32F
	glGenTextures(1, &tempTextureHandle);
	glBindTexture(GL_TEXTURE_2D, tempTextureHandle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, format.externalFormat, format.type, data);

	int faceWidth = width / 4;
	int faceHeight = height / 2;
	ShaderProgram::use(s_equirectangularShader);
	s_equirectangularShader->setUniform("faceSize", faceWidth, faceHeight);
	glBindImageTexture(0, tempTextureHandle, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, texture->getTextureName(), 0, GL_TRUE, 0, GL_WRITE_ONLY, format.internalFormat);
	glDispatchCompute((int)ceil(faceWidth / 16), (int)ceil(faceHeight / 16), 1);
	ShaderProgram::use(NULL);

	glDeleteTextures(1, &tempTextureHandle);
}

void CubeMap::uploadCubeFaces(Texture* texture, void* data[6], uint32_t width, uint32_t height) {
	OpenGLTextureFormat format = Texture::getOpenGLTextureFormat(texture->getFormat());

	texture->bind();
	for (int i = 0; i < 6; i++) {
		OpenGLTextureTarget target = CubeMap::getOpenGLFaceTarget((CubeMapFace)i);
		glTexImage2D(target.target, 0, format.internalFormat, width, height, 0, format.externalFormat, format.type, data[i]);
	}
	texture->unbind();
}




CubeMapArray::CubeMapArray(uint32_t width, uint32_t height, uint32_t layers, TextureFormat format, TextureFilter minFilter, TextureFilter magFilter, TextureWrap uWrap, TextureWrap vWrap, TextureWrap wWrap):
	Texture(TextureTarget::TEXTURE_CUBEMAP, format, minFilter, magFilter),
	m_width(width),
	m_height(height) {
	this->setSize(width, height, layers);
	this->setWrapMode(uWrap, vWrap, wWrap);
	this->setFilterMode(minFilter, magFilter);
}

CubeMapArray::~CubeMapArray() {
	Texture::~Texture();
}

void CubeMapArray::upload(std::vector<void*> data, uint32_t width, uint32_t height) {
	for (int i = 0; i < data.size(); i++) {
		this->upload(data[i], width, height);
	}
}

void CubeMapArray::upload(void* data, uint32_t layer, uint32_t width, uint32_t height) {
	assert(false); // unsupported
	//CubeMap::uploadEquirectangular(this, data, width, height);
}

void CubeMapArray::upload(std::vector<void*> data[6], uint32_t width, uint32_t height) {
	uint32_t size = data[0].size();

	//for (int i = 0; i < 6; i++) {
	//	assert(data[i].size() == size);
	//}

	for (int i = 0; i < size; i++) {
		void* layerData[6] { data[0][i], data[1][i], data[2][i], data[3][i], data[4][i], data[5][i] };
		this->upload(layerData, width, height);
	}
}

void CubeMapArray::upload(void* data[6], uint32_t layer, uint32_t width, uint32_t height) {
	assert(false); // unsupported
	//CubeMap::uploadCubeFaces(this, data, width, height);

	// texSubImage3D into the layer-face
}

void CubeMapArray::bind(uint32_t textureUnit) {
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(target.target, m_textureName);
}

void CubeMapArray::unbind() {
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	glBindTexture(target.target, 0);
}

uint64_t CubeMapArray::getMemorySize() const {
	return Texture::getPixelSize(m_format) * m_width * m_height * m_layers * 6;
}

void CubeMapArray::setSize(uint32_t width, uint32_t height, uint32_t layers, uint32_t externalFormat) {
	if (width != m_width || height != m_height || layers != m_layers || externalFormat != GL_NONE) {
		m_width = width;
		m_height = height;
		m_layers = layers;
		OpenGLTextureFormat format = Texture::getOpenGLTextureFormat(m_format, externalFormat);

		this->bind();
		OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
		glTexImage3D(target.target, 0, format.internalFormat, width, height, layers * 6, 0, format.externalFormat, format.type, NULL);
		this->unbind();
	}
}

bool CubeMapArray::setWrapMode(TextureWrap u, TextureWrap v, TextureWrap w) {
	if (u != m_uWrap || v != m_vWrap || w != m_wWrap) {
		OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
		OpenGLTextureWrap wrap = Texture::getOpenGLTextureWrap(u, v, w);
		if (wrap.u == GL_NONE || wrap.v == GL_NONE || wrap.w == GL_NONE) {
			return false;
		}

		m_uWrap = u;
		m_vWrap = v;
		m_wWrap = w;

		this->bind();
		glTexParameteri(target.target, GL_TEXTURE_WRAP_S, wrap.u);
		glTexParameteri(target.target, GL_TEXTURE_WRAP_T, wrap.v);
		glTexParameteri(target.target, GL_TEXTURE_WRAP_R, wrap.w);
		this->unbind();
	}

	return true;
}

TextureWrap CubeMapArray::getUWrapFilterMode() const {
	return m_uWrap;
}

TextureWrap CubeMapArray::getVWrapFilterMode() const {
	return m_vWrap;
}

TextureWrap CubeMapArray::getWWrapFilterMode() const {
	return m_wWrap;
}

uint32_t CubeMapArray::getWidth() const {
	return m_width;
}

uint32_t CubeMapArray::getHeight() const {
	return m_height;
}

uint32_t CubeMapArray::getLayers() const {
	return m_layers;
}
