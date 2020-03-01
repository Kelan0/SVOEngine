#include "ShadowMapRenderer.h"
#include "core/Engine.h"
#include "core/scene/Scene.h"
#include "core/renderer/Lights.h"
#include "core/renderer/Texture.h"
#include "core/renderer/CubeMap.h"
#include "core/renderer/ShaderProgram.h"
#include "core/renderer/Framebuffer.h"
#include "core/renderer/ScreenRenderer.h"
#include "core/scene/Camera.h"

ShaderProgram* ShadowMap::s_shadowMapShader = NULL;

ShadowMap::ShadowMap(uint32_t width, uint32_t height):
	m_width(0), 
	m_height(0) {
	this->setSize(width, height);
	m_shadowCamera = new CubemapCamera();
}

ShadowMap::~ShadowMap() {
	delete m_shadowTexture;
}

void ShadowMap::setSize(uint32_t width, uint32_t height) {
	if (width != m_width || height != m_height) {
		m_width = width;
		m_height = height;

		if (m_shadowTexture == NULL) {
			m_shadowTexture = new CubeMap(width, height, TextureFormat::DEPTH32_FLOAT, TextureFilter::LINEAR_PIXEL, TextureFilter::LINEAR_PIXEL);
		}

		m_shadowTexture->setSize(width, height, GL_DEPTH_COMPONENT);

		delete m_shadowFramebuffer;
		m_shadowFramebuffer = new Framebuffer();
		m_shadowFramebuffer->bind(width, height);
		//m_shadowFramebuffer->setDrawBuffer(GL_NONE);
		//m_shadowFramebuffer->setReadBuffer(GL_NONE);

		//m_shadowFramebuffer->createColourTextureAttachment(0, m_shadowTexture->getTextureName(), 0);
		//m_shadowFramebuffer->createDepthBufferAttachment(width, height, m_shadowFramebuffer->genRenderBuffers());
		m_shadowFramebuffer->createDepthTextureAttachment(m_shadowTexture->getTextureName());

		m_shadowFramebuffer->checkStatus(true);
		m_shadowFramebuffer->unbind();
	}
}

void ShadowMap::renderShadow(PointLight* light, double dt, double partialTicks) {
	//info("Rendering omnidirectional shadow at [%f %f %f]\n", light->getPosition().x, light->getPosition().y, light->getPosition().z);
	Camera* camera = Engine::scene()->getCamera();
	m_shadowCamera->transform().setTranslation(light->getPosition());
	m_shadowCamera->render(dt, partialTicks);
	
	m_shadowFramebuffer->bind(m_width, m_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Engine::scene()->setCamera(m_shadowCamera);
	Engine::scene()->setSimpleRenderEnabled(true);
	Engine::scene()->renderDirect(ShadowMap::shadowMapShader(), dt, partialTicks);
	Engine::scene()->setSimpleRenderEnabled(false);
	Engine::scene()->setCamera(camera);
	Engine::screenRenderer()->bindFramebuffer();
}

bool ShadowMap::bindShadowTexture(uint32_t unit) const {
	if (m_shadowTexture != NULL) {
		m_shadowTexture->bind(unit);
		return true;
	}
	return false;
}

uint32_t ShadowMap::getWidth() const {
	return m_width;
}

uint32_t ShadowMap::getHeight() const {
	return m_height;
}

ShaderProgram* ShadowMap::shadowMapShader() {
	if (s_shadowMapShader == NULL) {
		s_shadowMapShader = new ShaderProgram();
		s_shadowMapShader->addShader(GL_VERTEX_SHADER, "shaders/raster/shadow/vert.glsl");
		s_shadowMapShader->addShader(GL_GEOMETRY_SHADER, "shaders/raster/shadow/geom.glsl");
		s_shadowMapShader->addShader(GL_FRAGMENT_SHADER, "shaders/raster/shadow/frag.glsl");
		s_shadowMapShader->addAttribute(0, "vs_vertexPosition");
		s_shadowMapShader->completeProgram();
	}

	return s_shadowMapShader;
}