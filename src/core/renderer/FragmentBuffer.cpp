#include "core/renderer/FragmentBuffer.h"
#include "core/renderer/Texture.h"

FragmentBuffer::FragmentBuffer(uint32_t width, uint32_t height):
	m_width(0),
	m_height(0) {
	m_albedoTexture = new Texture2D(width, height, TextureFormat::R8_G8_B8_A8_UNORM, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_normalTexture = new Texture2D(width, height, TextureFormat::R16_G16_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_roughnessTexture = new Texture2D(width, height, TextureFormat::R16_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_metalnessTexture = new Texture2D(width, height, TextureFormat::R16_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_ambientOcclusionTexture = new Texture2D(width, height, TextureFormat::R16_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_irradianceTexture = new Texture2D(width, height, TextureFormat::R16_G16_B16_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_reflectionTexture = new Texture2D(width, height, TextureFormat::R16_G16_B16_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_depthTexture = new Texture2D(width, height, TextureFormat::DEPTH32_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);

}

FragmentBuffer::~FragmentBuffer() {
	delete m_albedoTexture;
	delete m_normalTexture;
	delete m_roughnessTexture;
	delete m_metalnessTexture;
	delete m_ambientOcclusionTexture;
	delete m_irradianceTexture;
	delete m_reflectionTexture;
	delete m_depthTexture;
}

void FragmentBuffer::setResolution(uint32_t width, uint32_t height) {
	if (width != m_width || height != m_height) {
		m_width = width;
		m_height = height;

		static_cast<Texture2D*>(m_albedoTexture)->setSize(width, height, GL_RGB);
		static_cast<Texture2D*>(m_normalTexture)->setSize(width, height, GL_RGB);
		static_cast<Texture2D*>(m_roughnessTexture)->setSize(width, height, GL_RGB);
		static_cast<Texture2D*>(m_metalnessTexture)->setSize(width, height, GL_RGB);
		static_cast<Texture2D*>(m_ambientOcclusionTexture)->setSize(width, height, GL_RGB);
		static_cast<Texture2D*>(m_irradianceTexture)->setSize(width, height, GL_RGB);
		static_cast<Texture2D*>(m_reflectionTexture)->setSize(width, height, GL_RGB);
		static_cast<Texture2D*>(m_depthTexture)->setSize(width, height);
	}
}

Texture* FragmentBuffer::getAlbedoTexture() const {
	return m_albedoTexture;
}

Texture* FragmentBuffer::getNormalTexture() const {
	return m_normalTexture;
}

Texture* FragmentBuffer::getRoughnessTexture() const {
	return m_roughnessTexture;
}

Texture* FragmentBuffer::getMetalnessTexture() const {
	return m_metalnessTexture;
}

Texture* FragmentBuffer::getAmbientOcclusionTexture() const {
	return m_ambientOcclusionTexture;
}

Texture* FragmentBuffer::getIrradianceTexture() const {
	return m_irradianceTexture;
}

Texture* FragmentBuffer::getReflectionTexture() const {
	return m_reflectionTexture;
}

Texture* FragmentBuffer::getDepthTexture() const {
	return m_depthTexture;
}

uint32_t FragmentBuffer::getWidth() const {
	return m_width;
}

uint32_t FragmentBuffer::getHeight() const {
	return m_height;
}
