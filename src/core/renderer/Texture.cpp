#include "Texture.h"
#include "core/Engine.h"
#include "core/ResourceHandler.h"
#include "core/util/FileUtils.h"

Texture::Texture(TextureTarget target, TextureFormat format, TextureFilter minFilter, TextureFilter magFilter) :
	m_target(target),
	m_format(format),
	m_minFilter(minFilter),
	m_magFilter(magFilter),
	m_textureName(0),
	m_textureHandle(-1),
	m_mipmapEnabled(false),
	m_anisotropy(0.0) {
	glGenTextures(1, &m_textureName);
}

Texture::Texture(uint32_t handle, TextureTarget target, TextureFormat format, TextureFilter minFilter, TextureFilter magFilter) :
	m_target(target),
	m_format(format),
	m_minFilter(minFilter),
	m_magFilter(magFilter),
	m_textureName(handle),
	m_textureHandle(-1),
	m_mipmapEnabled(false),
	m_anisotropy(0.0) {
}

Texture::~Texture() {
	this->makeResident(false);
	glDeleteTextures(1, &m_textureName);
	m_textureName = 0;
}

void Texture::bind(uint32_t textureUnit) {
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(target.target, m_textureName);
}

void Texture::unbind() {
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	glBindTexture(target.target, 0);
}

bool Texture::setFilterMode(TextureFilter minFilter, TextureFilter magFilter, float anisotropy) {
	if (minFilter == m_minFilter && magFilter == m_magFilter && epsilonEqual(anisotropy, m_anisotropy, 0.01F)) {
		return true;
	}

	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	OpenGLTextureFilter filter = Texture::getOpenGLTextureFilter(minFilter, magFilter);

	if (target.target == GL_NONE || filter.minFilter == GL_NONE || filter.magFilter == GL_NONE) {
		return false;
	}

	m_minFilter = minFilter;
	m_magFilter = magFilter;

	this->bind();
	glTexParameteri(target.target, GL_TEXTURE_MIN_FILTER, filter.minFilter);
	glTexParameteri(target.target, GL_TEXTURE_MAG_FILTER, filter.magFilter);
	glTexParameterf(target.target, GL_TEXTURE_LOD_BIAS, -2.0);
	if (filter.mipmap) {
		this->generateMipmap();
	} else {
		m_mipmapEnabled = false;
	}

	anisotropy = clamp(anisotropy, 1.0F, 16.0F);
	if (epsilonNotEqual(m_anisotropy, anisotropy, 0.01F)) {
		m_anisotropy = anisotropy;
		glTexParameterf(target.target, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);
	}

	this->unbind();
	return true;
}

void Texture::generateMipmap(int32_t levels) {
	this->m_mipmapEnabled = true;
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	this->bind();
	if (levels > 0) {
		glTexParameteri(target.target, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(target.target, GL_TEXTURE_MAX_LEVEL, levels);
	}
	glGenerateMipmap(target.target);
	this->unbind();
}

TextureTarget Texture::getTarget() const {
	return m_target;
}

TextureFormat Texture::getFormat() const {
	return m_format;
}

TextureFilter Texture::getMinificationFilterMode() const {
	return m_minFilter;
}

TextureFilter Texture::getMagnificationFilterMode() const {
	return m_magFilter;
}

uint32_t Texture::getTextureName() const {
	return m_textureName;
}

uint64_t Texture::getTextureHandle() {
	if (m_textureHandle == uint64_t(-1)) {
		m_textureHandle = glGetTextureHandleARB(m_textureName);
	}
	return m_textureHandle;
}

uvec2 Texture::getPackedTextureHandle() {
	union {
		uvec2 u2x32;
		uint64_t u1x64;
	} v;
	// TODO: ensure little endian
	v.u1x64 = this->getTextureHandle();
	return v.u2x32;
}

void Texture::makeResident(bool resident) {
	if (resident != m_resident) {
		m_resident = resident;

		if (resident) {
			glMakeTextureHandleResidentARB(this->getTextureHandle());
		} else {
			glMakeTextureHandleNonResidentARB(this->getTextureHandle());
		}
	}
}

bool Texture::isMipmapEnabled() const {
	return m_mipmapEnabled;
}

bool Texture::isAnisotropicFilteringEnabled() const {
	return m_anisotropy > 1.0;
}

float Texture::getAnisotropy() const {
	return m_anisotropy;
}

OpenGLTextureFormat Texture::getOpenGLTextureFormat(TextureFormat textureFormat, uint32_t externalFormat) {
	bool b = externalFormat == GL_NONE;
	switch (textureFormat) {
		case TextureFormat::R32_G32_B32_A32_FLOAT: return OpenGLTextureFormat(GL_RGBA32F, b?GL_RGBA:externalFormat, GL_FLOAT);
		case TextureFormat::R32_G32_B32_A32_UINT: return OpenGLTextureFormat(GL_RGBA32UI, b?GL_RGBA:externalFormat, GL_UNSIGNED_INT);
		case TextureFormat::R32_G32_B32_A32_SINT: return OpenGLTextureFormat(GL_RGBA32I, b?GL_RGBA:externalFormat, GL_INT);

		case TextureFormat::R32_G32_B32_FLOAT: return OpenGLTextureFormat(GL_RGB32F, b?GL_RGB:externalFormat, GL_FLOAT);
		case TextureFormat::R32_G32_B32_UINT: return OpenGLTextureFormat(GL_RGB32UI, b?GL_RGB:externalFormat, GL_UNSIGNED_INT);
		case TextureFormat::R32_G32_B32_SINT: return OpenGLTextureFormat(GL_RGB32I, b?GL_RGB:externalFormat, GL_INT);

		case TextureFormat::R32_G32_FLOAT: return OpenGLTextureFormat(GL_RG32F, b?GL_RG:externalFormat, GL_FLOAT);
		case TextureFormat::R32_G32_UINT: return OpenGLTextureFormat(GL_RG32UI, b?GL_RG:externalFormat, GL_UNSIGNED_INT);
		case TextureFormat::R32_G32_SINT: return OpenGLTextureFormat(GL_RG32I, b?GL_RG:externalFormat, GL_INT);

		case TextureFormat::R32_FLOAT: return OpenGLTextureFormat(GL_R32F, b?GL_RED:externalFormat, GL_FLOAT);
		case TextureFormat::R32_UINT: return OpenGLTextureFormat(GL_R32UI, b?GL_RED_INTEGER:externalFormat, GL_UNSIGNED_INT);
		case TextureFormat::R32_SINT: return OpenGLTextureFormat(GL_R32I, b?GL_RED_INTEGER:externalFormat, GL_INT);
		case TextureFormat::DEPTH32_FLOAT: return OpenGLTextureFormat(GL_DEPTH_COMPONENT32F, b?GL_DEPTH_COMPONENT:externalFormat, GL_FLOAT);
		case TextureFormat::DEPTH32_SNORM: return OpenGLTextureFormat(GL_DEPTH_COMPONENT32, b?GL_DEPTH_COMPONENT:externalFormat, GL_UNSIGNED_INT);

		case TextureFormat::DEPTH24_SNORM: return OpenGLTextureFormat(GL_DEPTH_COMPONENT24, b?GL_DEPTH_COMPONENT:externalFormat, GL_UNSIGNED_INT);

		case TextureFormat::R16_G16_B16_A16_FLOAT: return OpenGLTextureFormat(GL_RGBA16F, b?GL_RGBA:externalFormat, GL_FLOAT); // GL_HALF_FLOAT ?
		case TextureFormat::R16_G16_B16_A16_UINT: return OpenGLTextureFormat(GL_RGBA16UI, b?GL_RGBA:externalFormat, GL_UNSIGNED_SHORT);
		case TextureFormat::R16_G16_B16_A16_SINT: return OpenGLTextureFormat(GL_RGBA16I, b?GL_RGBA:externalFormat, GL_SHORT);
		case TextureFormat::R16_G16_B16_A16_UNORM: return OpenGLTextureFormat(GL_RGBA16, b?GL_RGBA:externalFormat, GL_UNSIGNED_SHORT);
		case TextureFormat::R16_G16_B16_A16_SNORM: return OpenGLTextureFormat(GL_RGBA16_SNORM, b?GL_RGBA:externalFormat, GL_SHORT);

		case TextureFormat::R16_G16_B16_FLOAT: return OpenGLTextureFormat(GL_RGB16F, b?GL_RGB:externalFormat, GL_FLOAT); // GL_HALF_FLOAT ?
		case TextureFormat::R16_G16_B16_UINT: return OpenGLTextureFormat(GL_RGB16UI, b?GL_RGB:externalFormat, GL_UNSIGNED_SHORT);
		case TextureFormat::R16_G16_B16_SINT: return OpenGLTextureFormat(GL_RGB16I, b?GL_RGB:externalFormat, GL_SHORT);
		case TextureFormat::R16_G16_B16_UNORM: return OpenGLTextureFormat(GL_RGB16, b?GL_RGB:externalFormat, GL_UNSIGNED_SHORT);
		case TextureFormat::R16_G16_B16_SNORM: return OpenGLTextureFormat(GL_RGB16_SNORM, b?GL_RGB:externalFormat, GL_SHORT);

		case TextureFormat::R16_G16_FLOAT: return OpenGLTextureFormat(GL_RG16F, b?GL_RG:externalFormat, GL_FLOAT); // GL_HALF_FLOAT ?
		case TextureFormat::R16_G16_UINT: return OpenGLTextureFormat(GL_RG16UI, b?GL_RG:externalFormat, GL_UNSIGNED_SHORT);
		case TextureFormat::R16_G16_SINT: return OpenGLTextureFormat(GL_RG16I, b?GL_RG:externalFormat, GL_SHORT);
		case TextureFormat::R16_G16_UNORM: return OpenGLTextureFormat(GL_RG16, b?GL_RG:externalFormat, GL_UNSIGNED_SHORT);
		case TextureFormat::R16_G16_SNORM: return OpenGLTextureFormat(GL_RG16_SNORM, b?GL_RG:externalFormat, GL_SHORT);

		case TextureFormat::R16_FLOAT: return OpenGLTextureFormat(GL_R16F, b?GL_RED:externalFormat, GL_FLOAT); // GL_HALF_FLOAT ?
		case TextureFormat::R16_UINT: return OpenGLTextureFormat(GL_R16UI, b?GL_RED_INTEGER:externalFormat, GL_UNSIGNED_SHORT);
		case TextureFormat::R16_SINT: return OpenGLTextureFormat(GL_R16I, b?GL_RED_INTEGER:externalFormat, GL_SHORT);
		case TextureFormat::R16_UNORM: return OpenGLTextureFormat(GL_R16, b?GL_RED:externalFormat, GL_UNSIGNED_SHORT);
		case TextureFormat::R16_SNORM: return OpenGLTextureFormat(GL_R16_SNORM, b?GL_RED:externalFormat, GL_SHORT);
		case TextureFormat::DEPTH16_SNORM: return OpenGLTextureFormat(GL_DEPTH_COMPONENT16, b?GL_DEPTH_COMPONENT:externalFormat, GL_UNSIGNED_SHORT);

		case TextureFormat::R8_G8_B8_A8_UINT: return OpenGLTextureFormat(GL_RGBA8UI, b?GL_RGBA:externalFormat, GL_UNSIGNED_BYTE);
		case TextureFormat::R8_G8_B8_A8_SINT: return OpenGLTextureFormat(GL_RGBA8I, b?GL_RGBA:externalFormat, GL_BYTE);
		case TextureFormat::R8_G8_B8_A8_UNORM: return OpenGLTextureFormat(GL_RGBA8, b?GL_RGBA:externalFormat, GL_UNSIGNED_BYTE);
		case TextureFormat::R8_G8_B8_A8_SNORM: return OpenGLTextureFormat(GL_RGBA8_SNORM, b?GL_RGBA:externalFormat, GL_BYTE);

		case TextureFormat::R8_G8_B8_UINT: return OpenGLTextureFormat(GL_RGB8UI, b?GL_RGB:externalFormat, GL_UNSIGNED_BYTE);
		case TextureFormat::R8_G8_B8_SINT: return OpenGLTextureFormat(GL_RGB8I, b?GL_RGB:externalFormat, GL_BYTE);
		case TextureFormat::R8_G8_B8_UNORM: return OpenGLTextureFormat(GL_RGB8, b?GL_RGB:externalFormat, GL_UNSIGNED_BYTE);
		case TextureFormat::R8_G8_B8_SNORM: return OpenGLTextureFormat(GL_RGB8_SNORM, b?GL_RGB:externalFormat, GL_BYTE);

		case TextureFormat::R8_G8_UINT: return OpenGLTextureFormat(GL_RG8UI, b?GL_RG:externalFormat, GL_UNSIGNED_BYTE);
		case TextureFormat::R8_G8_SINT: return OpenGLTextureFormat(GL_RG8I, b?GL_RG:externalFormat, GL_BYTE);
		case TextureFormat::R8_G8_UNORM: return OpenGLTextureFormat(GL_RG8, b?GL_RG:externalFormat, GL_UNSIGNED_BYTE);
		case TextureFormat::R8_G8_SNORM: return OpenGLTextureFormat(GL_RG8_SNORM, b?GL_RG:externalFormat, GL_BYTE);

		case TextureFormat::R8_UINT: return OpenGLTextureFormat(GL_R8UI, b?GL_RED_INTEGER:externalFormat, GL_UNSIGNED_BYTE);
		case TextureFormat::R8_SINT: return OpenGLTextureFormat(GL_R8I, b?GL_RED_INTEGER:externalFormat, GL_BYTE);
		case TextureFormat::R8_UNORM: return OpenGLTextureFormat(GL_R8, b?GL_RED:externalFormat, GL_UNSIGNED_BYTE);
		case TextureFormat::R8_SNORM: return OpenGLTextureFormat(GL_R8_SNORM, b?GL_RED:externalFormat, GL_BYTE);

		default: return OpenGLTextureFormat();
	}
}

OpenGLTextureFilter Texture::getOpenGLTextureFilter(TextureFilter minFilter, TextureFilter magFilter) {
	OpenGLTextureFilter ret;
	switch (minFilter) {
		case TextureFilter::NEAREST_PIXEL: ret.minFilter = GL_NEAREST; break;
		case TextureFilter::LINEAR_PIXEL: ret.minFilter = GL_LINEAR; break;
		case TextureFilter::NEAREST_MIPMAP_NEAREST_PIXEL: ret.minFilter = GL_NEAREST_MIPMAP_NEAREST, ret.mipmap = true; break;
		case TextureFilter::NEAREST_MIPMAP_LINEAR_PIXEL: ret.minFilter = GL_LINEAR_MIPMAP_NEAREST, ret.mipmap = true; break;
		case TextureFilter::LINEAR_MIPMAP_NEAREST_PIXEL: ret.minFilter = GL_NEAREST_MIPMAP_LINEAR, ret.mipmap = true; break;
		case TextureFilter::LINEAR_MIPMAP_LINEAR_PIXEL: ret.minFilter = GL_LINEAR_MIPMAP_LINEAR, ret.mipmap = true; break;
		default: return OpenGLTextureFilter(); // INVALID
	}
	switch (magFilter) {
		case TextureFilter::NEAREST_PIXEL: ret.magFilter = GL_NEAREST; break;
		case TextureFilter::LINEAR_PIXEL: ret.magFilter = GL_LINEAR; break;
		default: return OpenGLTextureFilter(); // INVALID
	}
	return ret;
}

OpenGLTextureTarget Texture::getOpenGLTextureTarget(TextureTarget textureTarget) {
	switch (textureTarget) {
		case TextureTarget::TEXTURE_1D: return OpenGLTextureTarget(GL_TEXTURE_1D);
		case TextureTarget::TEXTURE_1D_ARRAY: return OpenGLTextureTarget(GL_TEXTURE_1D_ARRAY);
		case TextureTarget::TEXTURE_2D: return OpenGLTextureTarget(GL_TEXTURE_2D);
		case TextureTarget::TEXTURE_2D_ARRAY: return OpenGLTextureTarget(GL_TEXTURE_2D_ARRAY);
		case TextureTarget::TEXTURE_3D: return OpenGLTextureTarget(GL_TEXTURE_3D);
		case TextureTarget::TEXTURE_CUBEMAP: return OpenGLTextureTarget(GL_TEXTURE_CUBE_MAP);
		case TextureTarget::TEXTURE_CUBEMAP_POSITIVE_X: return OpenGLTextureTarget(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
		case TextureTarget::TEXTURE_CUBEMAP_NEGATIVE_X: return OpenGLTextureTarget(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
		case TextureTarget::TEXTURE_CUBEMAP_POSITIVE_Y: return OpenGLTextureTarget(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
		case TextureTarget::TEXTURE_CUBEMAP_NEGATIVE_Y: return OpenGLTextureTarget(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
		case TextureTarget::TEXTURE_CUBEMAP_POSITIVE_Z: return OpenGLTextureTarget(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
		case TextureTarget::TEXTURE_CUBEMAP_NEGATIVE_Z: return OpenGLTextureTarget(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
		default: return OpenGLTextureTarget();
	}
}

OpenGLTextureWrap Texture::getOpenGLTextureWrap(TextureWrap u, TextureWrap v, TextureWrap w) {
	OpenGLTextureWrap ret;
	switch (u) {
		case TextureWrap::CLAMP_TO_EDGE: ret.u = GL_CLAMP_TO_EDGE; break;
		case TextureWrap::MIRRORED_REPEAT: ret.u = GL_MIRRORED_REPEAT; break;
		case TextureWrap::REPEAT: ret.u = GL_REPEAT; break;
		default: return OpenGLTextureWrap();
	}
	switch (v) {
		case TextureWrap::CLAMP_TO_EDGE: ret.v = GL_CLAMP_TO_EDGE; break;
		case TextureWrap::MIRRORED_REPEAT: ret.v = GL_MIRRORED_REPEAT; break;
		case TextureWrap::REPEAT: ret.v = GL_REPEAT; break;
		default: return OpenGLTextureWrap();
	}
	switch (w) {
		case TextureWrap::CLAMP_TO_EDGE: ret.w = GL_CLAMP_TO_EDGE; break;
		case TextureWrap::MIRRORED_REPEAT: ret.w = GL_MIRRORED_REPEAT; break;
		case TextureWrap::REPEAT: ret.w = GL_REPEAT; break;
		default: return OpenGLTextureWrap();
	}
	return ret;
}

uint64_t Texture::getPixelSize(TextureFormat textureFormat) {
	switch (textureFormat) {
		case TextureFormat::R32_G32_B32_A32_FLOAT:
		case TextureFormat::R32_G32_B32_A32_UINT:
		case TextureFormat::R32_G32_B32_A32_SINT:
			return 16;

		case TextureFormat::R32_G32_B32_FLOAT:
		case TextureFormat::R32_G32_B32_UINT:
		case TextureFormat::R32_G32_B32_SINT:
			return 12;

		case TextureFormat::R32_G32_FLOAT:
		case TextureFormat::R32_G32_UINT:
		case TextureFormat::R32_G32_SINT:
			return 8;

		case TextureFormat::R32_FLOAT:
		case TextureFormat::R32_UINT:
		case TextureFormat::R32_SINT:
		case TextureFormat::DEPTH32_FLOAT:
		case TextureFormat::DEPTH32_SNORM:
			return 4;

		case TextureFormat::DEPTH24_SNORM:
			return 3;

		case TextureFormat::R16_G16_B16_A16_FLOAT:
		case TextureFormat::R16_G16_B16_A16_UINT:
		case TextureFormat::R16_G16_B16_A16_SINT:
		case TextureFormat::R16_G16_B16_A16_UNORM:
		case TextureFormat::R16_G16_B16_A16_SNORM:
			return 8;

		case TextureFormat::R16_G16_B16_FLOAT:
		case TextureFormat::R16_G16_B16_UINT:
		case TextureFormat::R16_G16_B16_SINT:
		case TextureFormat::R16_G16_B16_UNORM:
		case TextureFormat::R16_G16_B16_SNORM:
			return 6;

		case TextureFormat::R16_G16_FLOAT:
		case TextureFormat::R16_G16_UINT:
		case TextureFormat::R16_G16_SINT:
		case TextureFormat::R16_G16_UNORM:
		case TextureFormat::R16_G16_SNORM:
			return 4;

		case TextureFormat::R16_FLOAT:
		case TextureFormat::R16_UINT:
		case TextureFormat::R16_SINT:
		case TextureFormat::R16_UNORM:
		case TextureFormat::R16_SNORM:
		case TextureFormat::DEPTH16_SNORM:
			return 2;

		case TextureFormat::R8_G8_B8_A8_UINT:
		case TextureFormat::R8_G8_B8_A8_SINT:
		case TextureFormat::R8_G8_B8_A8_UNORM:
		case TextureFormat::R8_G8_B8_A8_SNORM:
			return 4;

		case TextureFormat::R8_G8_B8_UINT:
		case TextureFormat::R8_G8_B8_SINT:
		case TextureFormat::R8_G8_B8_UNORM:
		case TextureFormat::R8_G8_B8_SNORM:
			return 3;

		case TextureFormat::R8_G8_UINT:
		case TextureFormat::R8_G8_SINT:
		case TextureFormat::R8_G8_UNORM:
		case TextureFormat::R8_G8_SNORM:
			return 2;

		case TextureFormat::R8_UINT:
		case TextureFormat::R8_SINT:
		case TextureFormat::R8_UNORM:
		case TextureFormat::R8_SNORM:
			return 1;

		default: return 0; // error
	}
}

Texture2D::Texture2D(uint32_t width, uint32_t height, TextureFormat format, TextureFilter minFilter, TextureFilter magFilter, TextureWrap uWrap, TextureWrap vWrap):
	Texture(TextureTarget::TEXTURE_2D, format, minFilter, magFilter),
	m_width(width), 
	m_height(height) {
	this->setSize(width, height);
	this->setWrapMode(uWrap, vWrap);
	this->setFilterMode(minFilter, magFilter);
}

Texture2D::Texture2D(uint32_t handle, uint32_t width, uint32_t height, TextureTarget target, TextureFormat format, TextureFilter minFilter, TextureFilter magFilter, TextureWrap uWrap, TextureWrap vWrap):
	Texture(handle, target, format, minFilter, magFilter),
	m_width(width),
	m_height(height) {
	this->setSize(width, height);
	this->setWrapMode(uWrap, vWrap);
	this->setFilterMode(minFilter, magFilter);
}

Texture2D::~Texture2D() {
	Texture::~Texture();
}

bool Texture2D::load(std::string filePath, Texture2D** dstTexturePtr, bool verticalFlip) {
	if (filePath.empty()) {
		return false;
	}

	if (dstTexturePtr == NULL) {
		return false;
	}

	Image image;
	if (!FileUtils::loadImage(filePath, image, verticalFlip)) {
		info("Failed to load image file \"%s\"\n", filePath.c_str());
		return false;
	}

	TextureFormat format = TextureFormat::DEFAULT_RGB;
	switch (image.channels) {
		case 1: format = TextureFormat::DEFAULT_GRAYSCALE; break;
		case 2: format = TextureFormat::DEFAULT_RG; break;
		case 3: format = TextureFormat::DEFAULT_RGB; break;
		case 4: format = TextureFormat::DEFAULT_RGBA; break;
		default:
			error("Loaded image with unsupported channel configuration\n");
			return false;
	}

	Texture2D* dstTexture = *dstTexturePtr;

	if (dstTexture == NULL) {
		*dstTexturePtr = new Texture2D(image.width, image.height, format);
		dstTexture = *dstTexturePtr;
	}

	if (dstTexture->getWidth() < image.width || dstTexture->getHeight() < image.height) {
		dstTexture->setSize(image.width, image.height);
	}
	dstTexture->upload(image.data, image.width, image.height);
	return true;
}

void Texture2D::upload(void* data, uint32_t width, uint32_t height, uint32_t left, uint32_t top) {
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	OpenGLTextureFormat format = Texture::getOpenGLTextureFormat(m_format);
	if (width == 0) width = m_width;
	if (height == 0) height = m_height;

	this->bind();
	glTexSubImage2D(target.target, 0, left, top, width, height, format.externalFormat, format.type, data);
	this->unbind();
}

//void Texture2D::bind(uint32_t textureUnit) {
//	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
//	glActiveTexture(GL_TEXTURE0 + textureUnit);
//	glBindTexture(target.target, m_textureName);
//}
//
//void Texture2D::unbind() {
//	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
//	glBindTexture(target.target, 0);
//}

uint64_t Texture2D::getMemorySize() const {
	return Texture::getPixelSize(m_format) * m_width * m_height;
}

void Texture2D::setSize(uint32_t width, uint32_t height, uint32_t externalFormat) {
	m_width = width;
	m_height = height;
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	OpenGLTextureFormat format = Texture::getOpenGLTextureFormat(m_format, externalFormat);
	this->bind();
	glTexImage2D(target.target, 0, format.internalFormat, width, height, 0, format.externalFormat, format.type, NULL);
	this->unbind();
}

//void Texture2D::resize(uint32_t width, uint32_t height, uint32_t left, uint32_t top) {
//	// TODO:
//	// Copy data out of texture in region of interest
//	// reallocate texture and upload copied data
//	assert(false);
//}

bool Texture2D::setWrapMode(TextureWrap u, TextureWrap v) {
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	OpenGLTextureWrap wrap = Texture::getOpenGLTextureWrap(u, v);
	if (wrap.u == GL_NONE || wrap.v == GL_NONE) {
		return false;
	}

	m_uWrap = u;
	m_vWrap = v;

	this->bind();
	glTexParameteri(target.target, GL_TEXTURE_WRAP_S, wrap.u);
	glTexParameteri(target.target, GL_TEXTURE_WRAP_T, wrap.v);
	this->unbind();

	return true;
}

TextureWrap Texture2D::getUWrapFilterMode() const {
	return m_uWrap;
}

TextureWrap Texture2D::getVWrapFilterMode() const {
	return m_vWrap;
}

uint32_t Texture2D::getWidth() const {
	return m_width;
}

uint32_t Texture2D::getHeight() const {
	return m_height;
}

Texture2DArray::Texture2DArray(uint32_t width, uint32_t height, uint32_t layers, TextureFormat format, TextureFilter minFilter, TextureFilter magFilter, TextureWrap uWrap, TextureWrap vWrap):
	Texture(TextureTarget::TEXTURE_2D_ARRAY, format, minFilter, magFilter),
	m_width(width),
	m_height(height),
	m_layers(layers) {
	this->setSize(width, height, layers);
	this->setWrapMode(uWrap, vWrap);
	this->setFilterMode(minFilter, magFilter);
}

Texture2DArray::~Texture2DArray() {
	Texture::~Texture();
}

void Texture2DArray::upload(void* data, uint32_t depth, uint32_t width, uint32_t height, uint32_t layer, uint32_t left, uint32_t top) {
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	OpenGLTextureFormat format = Texture::getOpenGLTextureFormat(m_format);
	if (width == 0) width = m_width;
	if (height == 0) height = m_height;

	this->bind();
	glTexSubImage3D(target.target, 0, left, top, layer, width, height, depth, format.externalFormat, format.type, data);
	this->unbind();
}

//void Texture2DArray::bind(uint32_t textureUnit) {
//	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
//	glActiveTexture(GL_TEXTURE0 + textureUnit);
//	glBindTexture(target.target, m_textureName);
//}
//
//void Texture2DArray::unbind() {
//	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
//	glBindTexture(target.target, 0);
//}

uint64_t Texture2DArray::getMemorySize() const {
	return Texture::getPixelSize(m_format) * m_width * m_height * m_layers;
}

void Texture2DArray::setSize(uint32_t width, uint32_t height, uint32_t layers, uint32_t externalFormat) {
	m_width = width;
	m_height = height;
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	OpenGLTextureFormat format = Texture::getOpenGLTextureFormat(m_format, externalFormat);
	this->bind();
	glTexImage3D(target.target, 0, format.internalFormat, width, height, layers, 0, format.externalFormat, format.type, NULL);
	this->unbind();
}

bool Texture2DArray::setWrapMode(TextureWrap u, TextureWrap v) {
	OpenGLTextureTarget target = Texture::getOpenGLTextureTarget(m_target);
	OpenGLTextureWrap wrap = Texture::getOpenGLTextureWrap(u, v);
	if (wrap.u == GL_NONE || wrap.v == GL_NONE) {
		return false;
	}

	m_uWrap = u;
	m_vWrap = v;

	this->bind();
	glTexParameteri(target.target, GL_TEXTURE_WRAP_S, wrap.u);
	glTexParameteri(target.target, GL_TEXTURE_WRAP_T, wrap.v);
	this->unbind();

	return true;
}

TextureWrap Texture2DArray::getUWrapFilterMode() const {
	return TextureWrap();
}

TextureWrap Texture2DArray::getVWrapFilterMode() const {
	return TextureWrap();
}

uint32_t Texture2DArray::getWidth() const {
	return m_width;
}

uint32_t Texture2DArray::getHeight() const {
	return m_height;
}

uint32_t Texture2DArray::getLayers() const {
	return m_layers;
}
