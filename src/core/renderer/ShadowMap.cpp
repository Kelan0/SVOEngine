#include "ShadowMap.h"
#include "core/Engine.h"
#include "core/scene/Scene.h"
#include "core/renderer/Lights.h"
#include "core/renderer/Texture.h"
#include "core/renderer/CubeMap.h"
#include "core/renderer/ShaderProgram.h"

ShaderProgram* ShadowMapRenderer::s_shadowMapShader = NULL;

ShadowMapRenderer::ShadowMapRenderer(uint32_t width, uint32_t height, uint32_t maxLights):
	m_width(0), 
	m_height(0), 
	m_maxLights(0) {
	this->setSize(width, height, maxLights);
}

ShadowMapRenderer::~ShadowMapRenderer() {
	delete m_directionalShadowTextures;
	delete m_omnidirectionalShadowTextures;
}

void ShadowMapRenderer::setSize(uint32_t width, uint32_t height, uint32_t maxLights) {
	if (width != m_width || height != m_height || maxLights != m_maxLights) {
		m_width = width;
		m_height = height;
		m_maxLights = maxLights;

		if (m_directionalShadowTextures == NULL) {
			m_directionalShadowTextures = new Texture2DArray(width, height, maxLights, TextureFormat::DEPTH32_FLOAT, TextureFilter::LINEAR_PIXEL, TextureFilter::LINEAR_PIXEL);
		} else {
			m_directionalShadowTextures->setSize(width, height, maxLights);
		}

		if (m_omnidirectionalShadowTextures == NULL) {
			m_omnidirectionalShadowTextures = new CubeMapArray(width, height, maxLights, TextureFormat::DEPTH32_FLOAT, TextureFilter::LINEAR_PIXEL, TextureFilter::LINEAR_PIXEL);
		} else {
			m_omnidirectionalShadowTextures->setSize(width, height, maxLights);
		}
	}
}

void ShadowMapRenderer::preRender(double dt, double partialTicks) {
	if (m_lights.size() > 1) {
		this->setSize(m_width, m_height, 1);
	}

	ShaderProgram* shadowMapShader = ShadowMapRenderer::shadowMapShader();
	ShaderProgram::use(shadowMapShader);
	Engine::scene()->applyUniforms(shadowMapShader);
	Engine::scene()->renderDirect(shadowMapShader, dt, partialTicks);
	ShaderProgram::use(NULL);
}

void ShadowMapRenderer::postRender() {
	m_lights.clear();
}

void ShadowMapRenderer::addLight(Light* light) {
	m_lights.push_back(light);
}

bool ShadowMapRenderer::bindDirectionalShadowTextures(uint32_t unit) const {
	if (m_directionalShadowTextures != NULL) {
		m_directionalShadowTextures->bind(unit);
		return true;
	}
	return false;
}

bool ShadowMapRenderer::bindOmnidirectionalShadowTextures(uint32_t unit) const {
	if (m_omnidirectionalShadowTextures != NULL) {
		m_omnidirectionalShadowTextures->bind(unit);
		return true;
	}
	return false;
}

uint32_t ShadowMapRenderer::getWidth() const {
	return m_width;
}

uint32_t ShadowMapRenderer::getHeight() const {
	return m_height;
}

uint32_t ShadowMapRenderer::getMaxLights() const {
	return m_maxLights;
}

ShaderProgram* ShadowMapRenderer::shadowMapShader() {
	if (s_shadowMapShader == NULL) {
		s_shadowMapShader = new ShaderProgram();
		s_shadowMapShader->addShader(GL_VERTEX_SHADER, "shaders/raster/shadow/vert.glsl");
		s_shadowMapShader->addShader(GL_GEOMETRY_SHADER, "shaders/raster/shadow/geom.glsl");
		s_shadowMapShader->addShader(GL_FRAGMENT_SHADER, "shaders/raster/shadow/frag.glsl");
		s_shadowMapShader->completeProgram();
	}

	return s_shadowMapShader;
}
