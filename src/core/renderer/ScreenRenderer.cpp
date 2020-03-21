#include "core/renderer/geometry/GeometryBuffer.h"
#include "core/renderer/ScreenRenderer.h"
#include "core/renderer/ShaderProgram.h"
#include "core/renderer/Framebuffer.h"
#include "core/renderer/MaterialManager.h"
#include "core/renderer/Texture.h"
#include "core/renderer/CubeMap.h"
#include "core/renderer/ShadowMapRenderer.h"
#include "core/renderer/EnvironmentMap.h"
#include "core/renderer/LayeredDepthBuffer.h"
#include "core/renderer/RaytraceRenderer.h"
#include "core/renderer/Voxelizer.h"
#include "core/scene/Scene.h"
#include "core/scene/BVH.h"
#include "core/scene/Camera.h"
#include "core/scene/Scene.h"
#include "core/ResourceHandler.h"
#include "core/Engine.h"

ScreenRenderer::ScreenRenderer(uint32_t width, uint32_t height):
	m_width(0),
	m_height(0) {

	m_normalTexture = new Texture2D(width, height, TextureFormat::R16_G16_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_tangentTexture = new Texture2D(width, height, TextureFormat::R16_G16_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_velocityTexture = new Texture2D(width, height, TextureFormat::R32_G32_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_textureCoordTexture = new Texture2D(width, height, TextureFormat::R32_G32_FLOAT, TextureFilter::LINEAR_PIXEL, TextureFilter::LINEAR_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_materialIndexTexture = new Texture2D(width, height, TextureFormat::R32_SINT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_linearDepthTexture = new Texture2D(width, height, TextureFormat::R32_FLOAT, TextureFilter::LINEAR_PIXEL, TextureFilter::LINEAR_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_depthTexture = new Texture2D(width, height, TextureFormat::DEPTH32_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_reprojectionHistoryTexture = new Texture2D(width, height, TextureFormat::R16_UINT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_prevReprojectionHistoryTexture = new Texture2D(width, height, TextureFormat::R16_UINT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	
	m_prevTextureCoordTexture = new Texture2D(width, height, TextureFormat::R32_G32_FLOAT, TextureFilter::LINEAR_PIXEL, TextureFilter::LINEAR_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_prevMaterialIndexTexture = new Texture2D(width, height, TextureFormat::R32_SINT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_prevLinearDepthTexture = new Texture2D(width, height, TextureFormat::R32_FLOAT, TextureFilter::LINEAR_PIXEL, TextureFilter::LINEAR_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_prevDepthTexture = new Texture2D(width, height, TextureFormat::DEPTH32_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);

	glGenTextures(1, &m_pixelNodeHeadTexture);
	glGenBuffers(1, &m_pixelNodeBuffer);
	glGenBuffers(1, &m_pixelNodeCounter);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_pixelNodeCounter);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(uint32_t), NULL, GL_DYNAMIC_COPY);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	m_screenShader = new ShaderProgram();
	m_screenShader->addShader(GL_VERTEX_SHADER, "shaders/raster/screen/vert.glsl");
	m_screenShader->addShader(GL_FRAGMENT_SHADER, "shaders/raster/screen/frag.glsl");
	m_screenShader->completeProgram();

	glGenVertexArrays(1, &m_fullscreenQuadVAO);

	this->setResolution(width, height);

	m_pointLightIcon = NULL;
	Texture2D* pointLightIcon = NULL;
	Texture2D::load(RESOURCE_PATH("textures/pointlight_icon.png"), &pointLightIcon, false);
	m_pointLightIcon = pointLightIcon;

	m_frameCount = 0;
}

ScreenRenderer::~ScreenRenderer() {
	glDeleteVertexArrays(1, &m_fullscreenQuadVAO);
	delete m_screenShader;
	delete m_normalTexture;
	delete m_tangentTexture;
	delete m_velocityTexture;
	delete m_textureCoordTexture;
	delete m_materialIndexTexture;
	delete m_linearDepthTexture;
	delete m_depthTexture;
	delete m_prevTextureCoordTexture;
	delete m_prevMaterialIndexTexture;
	delete m_prevLinearDepthTexture;
	delete m_prevDepthTexture;

	delete m_reprojectionHistoryTexture;
	delete m_prevReprojectionHistoryTexture;
	delete m_framebuffer;
	delete m_pointLightIcon;
}

void ScreenRenderer::drawFullscreenQuad() {
	// Binds a dummy empty VAO and tells the GPU to draw 3 vertices.
	// The vertex shader generates a triangle that covers the whole screen.
	glBindVertexArray(m_fullscreenQuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

CubemapCamera* depthCamera = NULL;
ShaderProgram* shadowShader = NULL;

void ScreenRenderer::render(double dt, double partialTicks) {
	this->unbindFramebuffer();

	//if (depthCamera == NULL) {
	//	depthCamera = new CubemapCamera();
	//}
	//
	//if (shadowShader == NULL) {
	//	shadowShader = new ShaderProgram();
	//	shadowShader->addShader(GL_VERTEX_SHADER, "shaders/raster/shadow/vert.glsl");
	//	shadowShader->addShader(GL_GEOMETRY_SHADER, "shaders/raster/shadow/geom.glsl");
	//	shadowShader->addShader(GL_FRAGMENT_SHADER, "shaders/raster/shadow/frag.glsl");
	//	shadowShader->addAttribute(0, "vs_vertexPosition");
	//	shadowShader->completeProgram();
	//}
	//
	//Camera* camera = Engine::scene()->getCamera();
	//depthCamera->transform().setTranslation(camera->transform().getTranslation());
	//depthCamera->transform().setOrientation(camera->transform().getOrientation());
	//depthCamera->transform().setScale(camera->transform().getScale());
	//depthCamera->render(dt, partialTicks);
	//
	//t_cubeDepthFramebuffer->bind(128, 128);
	//glClearColor(0.25F, 0.5F, 0.75F, 1.0F);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//Engine::scene()->setCamera(depthCamera);
	//Engine::scene()->setSimpleRenderEnabled(true);
	////Engine::instance()->setDebugRenderLightingEnabled(false); // TODO: flag to ignore material separation, render whole mesh in one call
	//Engine::scene()->renderDirect(shadowShader, dt, partialTicks);
	////Engine::instance()->setDebugRenderLightingEnabled(true);
	//Engine::scene()->setSimpleRenderEnabled(false);
	//Engine::scene()->setCamera(camera);
	//t_cubeDepthFramebuffer->unbind();

	glViewport(0, 0, m_width, m_height);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_3D);
	glEnable(GL_TEXTURE_CUBE_MAP);

	LightProbe* skybox = Engine::scene()->getGlobalEnvironmentMap();

	ShaderProgram::use(m_screenShader);
	Engine::instance()->scene()->applyUniforms(m_screenShader);
	m_screenShader->setUniform("renderGBufferMode", Engine::instance()->getDebugRenderGBufferMode());
	m_screenShader->setUniform("screenResolution", fvec2(m_width, m_height));

	int32_t index = 0;
	m_normalTexture->makeResident(true);
	m_screenShader->setUniform("normalTexture", m_normalTexture->getPackedTextureHandle());

	m_tangentTexture->makeResident(true);
	m_screenShader->setUniform("tangentTexture", m_tangentTexture->getPackedTextureHandle());

	m_velocityTexture->makeResident(true);
	m_screenShader->setUniform("velocityTexture", m_velocityTexture->getPackedTextureHandle());

	m_textureCoordTexture->makeResident(true);
	m_screenShader->setUniform("textureCoordTexture", m_textureCoordTexture->getPackedTextureHandle());

	m_materialIndexTexture->makeResident(true);
	m_screenShader->setUniform("materialIndexTexture", m_materialIndexTexture->getPackedTextureHandle());

	m_linearDepthTexture->makeResident(true);
	m_screenShader->setUniform("linearDepthTexture", m_linearDepthTexture->getPackedTextureHandle());

	m_depthTexture->makeResident(true);
	m_screenShader->setUniform("depthTexture", m_depthTexture->getPackedTextureHandle());

	m_prevTextureCoordTexture->makeResident(true);
	m_screenShader->setUniform("prevTextureCoordTexture", m_prevTextureCoordTexture->getPackedTextureHandle());

	m_prevMaterialIndexTexture->makeResident(true);
	m_screenShader->setUniform("prevMaterialIndexTexture", m_prevMaterialIndexTexture->getPackedTextureHandle());

	m_prevLinearDepthTexture->makeResident(true);
	m_screenShader->setUniform("prevLinearDepthTexture", m_prevLinearDepthTexture->getPackedTextureHandle());

	m_prevDepthTexture->makeResident(true);
	m_screenShader->setUniform("prevDepthTexture", m_prevDepthTexture->getPackedTextureHandle());

	//m_albedoTexture->makeResident(true);
	//m_screenShader->setUniform("albedoTexture", m_albedoTexture->getPackedTextureHandle());
	//
	//m_emissionTexture->makeResident(true);
	//m_screenShader->setUniform("emissionTexture", m_emissionTexture->getPackedTextureHandle());
	//
	//m_normalTexture->makeResident(true);
	//m_screenShader->setUniform("normalTexture", m_normalTexture->getPackedTextureHandle());
	//
	//m_roughnessTexture->makeResident(true);
	//m_screenShader->setUniform("roughnessTexture", m_roughnessTexture->getPackedTextureHandle());
	//
	//m_metalnessTexture->makeResident(true);
	//m_screenShader->setUniform("metalnessTexture", m_metalnessTexture->getPackedTextureHandle());
	//
	//m_ambientOcclusionTexture->makeResident(true);
	//m_screenShader->setUniform("ambientOcclusionTexture", m_ambientOcclusionTexture->getPackedTextureHandle());
	//
	//m_irradianceTexture->makeResident(true);
	//m_screenShader->setUniform("irradianceTexture", m_irradianceTexture->getPackedTextureHandle());
	//
	//m_reflectionTexture->makeResident(true);
	//m_screenShader->setUniform("reflectionTexture", m_reflectionTexture->getPackedTextureHandle());
	//
	//m_transmissionTexture->makeResident(true);
	//m_screenShader->setUniform("transmissionTexture", m_transmissionTexture->getPackedTextureHandle());
	//
	//m_depthTexture->makeResident(true);
	//m_screenShader->setUniform("depthTexture", m_depthTexture->getPackedTextureHandle());
	//
	//m_prevDepthTexture->makeResident(true);
	//m_screenShader->setUniform("prevDepthTexture", m_prevDepthTexture->getPackedTextureHandle());

	if (skybox != NULL) {
		skybox->getEnvironmentMap()->makeResident(true);
		m_screenShader->setUniform("skyboxEnvironmentTexture", skybox->getEnvironmentMap()->getPackedTextureHandle());
	}

	glBindImageTexture(0, m_pixelNodeHeadTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_pixelNodeBuffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, m_pixelNodeCounter);

	glBindImageTexture(3, m_prevReprojectionHistoryTexture->getTextureName(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R16UI);
	glBindImageTexture(4, m_reprojectionHistoryTexture->getTextureName(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R16UI);

	Engine::scene()->getMaterialManager()->bindMaterialBuffer(4);

	Engine::raytraceRenderer()->getFrameTexture()->makeResident(true);
	m_screenShader->setUniform("raytracedFrame", Engine::raytraceRenderer()->getFrameTexture()->getPackedTextureHandle());
	//Engine::raytraceRenderer()->bindFillTexture(6);

	//Engine::scene()->getStaticGeometryBuffer()->bindVertexBuffer(3);
	//Engine::scene()->getStaticGeometryBuffer()->bindTriangleBuffer(4);
	//Engine::scene()->getStaticGeometryBuffer()->bindBVHNodeBuffer(5);
	//Engine::scene()->getStaticGeometryBuffer()->bindBVHReferenceBuffer(6);

	LightProbe::getBRDFIntegrationMap()->makeResident(true);
	m_screenShader->setUniform("BRDFIntegrationMap", LightProbe::getBRDFIntegrationMap()->getPackedTextureHandle());

	m_pointLightIcon->makeResident(true);
	m_screenShader->setUniform("pointLightIcon", m_pointLightIcon->getPackedTextureHandle());

	this->drawFullscreenQuad();
	ShaderProgram::use(NULL);

	uint32_t zero = 0;
	uint32_t invalidIndex = 0xFFFFFFFF;
	glClearTexImage(m_pixelNodeHeadTexture, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &invalidIndex);

	uint32_t nodeCount;
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_pixelNodeCounter);
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(uint32_t), &nodeCount);
	glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	
	if (nodeCount > m_allocatedNodes) {
		// deallocate nodes after prolonged time of too many being allocated
		nodeCount = (uint32_t)(nodeCount * 1.1);
		this->setAllocatedNodes(nodeCount);
	}

	Texture* temp;
	// Swap depth textures for next frame.
	temp = m_textureCoordTexture, m_textureCoordTexture = m_prevTextureCoordTexture, m_prevTextureCoordTexture = temp;
	temp = m_materialIndexTexture, m_materialIndexTexture = m_prevMaterialIndexTexture, m_prevMaterialIndexTexture = temp;
	temp = m_depthTexture, m_depthTexture = m_prevDepthTexture, m_prevDepthTexture = temp;
	temp = m_linearDepthTexture, m_linearDepthTexture = m_prevLinearDepthTexture, m_prevLinearDepthTexture = temp;

	m_framebuffer->bind(m_width, m_height);
	m_framebuffer->createColourTextureAttachment(3, m_textureCoordTexture->getTextureName());
	m_framebuffer->createColourTextureAttachment(4, m_materialIndexTexture->getTextureName());
	m_framebuffer->createColourTextureAttachment(5, m_linearDepthTexture->getTextureName());
	m_framebuffer->createDepthTextureAttachment(m_depthTexture->getTextureName());

	// Swap reprojection history textures for next frame.

	Texture* tempReprojectionHistoryTexture = m_reprojectionHistoryTexture;
	m_reprojectionHistoryTexture = m_prevReprojectionHistoryTexture;
	m_prevReprojectionHistoryTexture = tempReprojectionHistoryTexture;

	m_frameCount++;
}

void ScreenRenderer::applyUniforms(ShaderProgram* shaderProgram) {
	glBindImageTexture(0, m_pixelNodeHeadTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_pixelNodeBuffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, m_pixelNodeCounter);
	shaderProgram->setUniform("allocatedFragmentNodes", (int)(m_allocatedNodes));

}

void ScreenRenderer::setResolution(uint32_t width, uint32_t height) {
	if (width != m_width || height != m_height) {
		info("Initializing deferred screen framebuffer\n");
		m_width = width;
		m_height = height;

		uint32_t drawBuffers[6] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5 };

		delete m_framebuffer;
		m_framebuffer = new Framebuffer();
		m_framebuffer->bind(width, height);
		m_framebuffer->setDrawBuffers(6, drawBuffers);

		static_cast<Texture2D*>(m_normalTexture)->setSize(width, height);
		static_cast<Texture2D*>(m_tangentTexture)->setSize(width, height);
		static_cast<Texture2D*>(m_velocityTexture)->setSize(width, height);
		static_cast<Texture2D*>(m_textureCoordTexture)->setSize(width, height);
		static_cast<Texture2D*>(m_materialIndexTexture)->setSize(width, height);
		static_cast<Texture2D*>(m_depthTexture)->setSize(width, height);
		static_cast<Texture2D*>(m_linearDepthTexture)->setSize(width, height);
		static_cast<Texture2D*>(m_prevTextureCoordTexture)->setSize(width, height);
		static_cast<Texture2D*>(m_prevMaterialIndexTexture)->setSize(width, height);
		static_cast<Texture2D*>(m_prevLinearDepthTexture)->setSize(width, height);
		static_cast<Texture2D*>(m_prevDepthTexture)->setSize(width, height);
		static_cast<Texture2D*>(m_reprojectionHistoryTexture)->setSize(width, height, GL_RED_INTEGER);
		static_cast<Texture2D*>(m_prevReprojectionHistoryTexture)->setSize(width, height, GL_RED_INTEGER);

		m_framebuffer->createColourTextureAttachment(0, m_normalTexture->getTextureName());
		m_framebuffer->createColourTextureAttachment(1, m_tangentTexture->getTextureName());
		m_framebuffer->createColourTextureAttachment(2, m_velocityTexture->getTextureName());
		m_framebuffer->createColourTextureAttachment(3, m_textureCoordTexture->getTextureName());
		m_framebuffer->createColourTextureAttachment(4, m_materialIndexTexture->getTextureName());
		m_framebuffer->createColourTextureAttachment(5, m_linearDepthTexture->getTextureName());
		m_framebuffer->createDepthTextureAttachment(m_depthTexture->getTextureName());

		//static_cast<Texture2D*>(m_albedoTexture)->setSize(width, height, GL_RGB);
		//static_cast<Texture2D*>(m_emissionTexture)->setSize(width, height, GL_RGB);
		//static_cast<Texture2D*>(m_normalTexture)->setSize(width, height, GL_RGB);
		//static_cast<Texture2D*>(m_roughnessTexture)->setSize(width, height, GL_RGB);
		//static_cast<Texture2D*>(m_metalnessTexture)->setSize(width, height, GL_RGB);
		//static_cast<Texture2D*>(m_ambientOcclusionTexture)->setSize(width, height, GL_RGB);
		//static_cast<Texture2D*>(m_irradianceTexture)->setSize(width, height, GL_RGB);
		//static_cast<Texture2D*>(m_reflectionTexture)->setSize(width, height, GL_RGB);
		//static_cast<Texture2D*>(m_transmissionTexture)->setSize(width, height, GL_RGB);
		//static_cast<Texture2D*>(m_depthTexture)->setSize(width, height);
		//static_cast<Texture2D*>(m_prevDepthTexture)->setSize(width, height);
		//static_cast<Texture2D*>(m_reprojectionHistoryTexture)->setSize(width, height, GL_RED_INTEGER);
		
		//m_framebuffer->createColourTextureAttachment(0, m_albedoTexture->getTextureName());
		//m_framebuffer->createColourTextureAttachment(1, m_emissionTexture->getTextureName());
		//m_framebuffer->createColourTextureAttachment(2, m_normalTexture->getTextureName());
		//m_framebuffer->createColourTextureAttachment(3, m_roughnessTexture->getTextureName());
		//m_framebuffer->createColourTextureAttachment(4, m_metalnessTexture->getTextureName());
		//m_framebuffer->createColourTextureAttachment(5, m_ambientOcclusionTexture->getTextureName());
		//m_framebuffer->createColourTextureAttachment(6, m_irradianceTexture->getTextureName());
		//m_framebuffer->createColourTextureAttachment(7, m_reflectionTexture->getTextureName());
		//m_framebuffer->createColourTextureAttachment(8, m_transmissionTexture->getTextureName());
		//m_framebuffer->createDepthTextureAttachment(m_depthTexture->getTextureName());

		m_framebuffer->checkStatus(true);
		m_framebuffer->unbind();

		uint32_t invalidIndex = 0xFFFFFFFF;
		glBindTexture(GL_TEXTURE_2D, m_pixelNodeHeadTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glClearTexImage(m_pixelNodeHeadTexture, 0, GL_R32UI, GL_RED_INTEGER, &invalidIndex);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		this->setAllocatedNodes(width * height * 4);



		//delete t_cubeDepthFramebuffer;
		//t_cubeDepthFramebuffer = new Framebuffer();
		//t_cubeDepthFramebuffer->bind(128, 128);
		//t_cubeDepthFramebuffer->setDrawBuffer(GL_NONE);
		//t_cubeDepthFramebuffer->setReadBuffer(GL_NONE);
		//
		//static_cast<CubeMap*>(t_cubeDepthTexture)->setSize(128, 128, GL_DEPTH_COMPONENT);
		//
		//t_cubeDepthFramebuffer->createDepthTextureAttachment(t_cubeDepthTexture->getTextureName());
		////glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, t_cubeDepthTexture->getTextureName(), 0);
		//
		//t_cubeDepthFramebuffer->checkStatus(true);
		//t_cubeDepthFramebuffer->unbind();
		//
		//// m_layeredDepthBuffer->setSize(width, height, sizeof(Fragment));
	}
}

void ScreenRenderer::setAllocatedNodes(uint32_t allocatedNodes) {
	if (allocatedNodes != m_allocatedNodes) {
		m_allocatedNodes = allocatedNodes;
		uint64_t size = sizeof(PackedFragment) * m_allocatedNodes;
		info("Allocating %d screen fragment buffer nodes (%.2f MiB) :- %.1f average pixel depth\n", allocatedNodes, size / (1024.0 * 1024.0), allocatedNodes / double(m_width * m_height));
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pixelNodeBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}

void ScreenRenderer::initViewport() const {
	glViewport(0, 0, m_width, m_height);
}

void ScreenRenderer::bindFramebuffer() const {
	m_framebuffer->bind(m_width, m_height);
}

void ScreenRenderer::unbindFramebuffer() const {
	m_framebuffer->unbind();
}

void ScreenRenderer::setFramebufferWriteEnabled(bool enabled) {
	//glColorMaski(0, enabled, enabled, enabled, enabled);
	//glColorMaski(1, enabled, enabled, enabled, enabled);
	//glColorMaski(2, enabled, enabled, enabled, enabled);
	//glColorMaski(3, enabled, enabled, enabled, enabled);
	//glColorMaski(4, enabled, enabled, enabled, enabled);
	//glColorMaski(5, enabled, enabled, enabled, enabled);
	//glColorMaski(6, enabled, enabled, enabled, enabled);
	//glColorMask(enabled, enabled, enabled, enabled);
	glDepthMask(enabled);
}

Framebuffer* ScreenRenderer::getFramebuffer() const {
	return m_framebuffer;
}

Texture* ScreenRenderer::getNormalTexture() const {
	return m_normalTexture;
}

Texture* ScreenRenderer::getTangentTexture() const {
	return m_tangentTexture;
}

Texture* ScreenRenderer::getVelocityTexture() const {
	return m_velocityTexture;
}

Texture* ScreenRenderer::getTextureCoordTexture() const {
	return m_textureCoordTexture;
}

Texture* ScreenRenderer::getMaterialIndexTexture() const {
	return m_materialIndexTexture;
}

Texture* ScreenRenderer::getLinearDepthTexture() const {
	return m_linearDepthTexture;
}

Texture* ScreenRenderer::getDepthTexture() const {
	return m_depthTexture;
}

Texture* ScreenRenderer::getPrevTextureCoordTexture() const {
	return m_prevTextureCoordTexture;
}

Texture* ScreenRenderer::getPrevMaterialIndexTexture() const {
	return m_prevMaterialIndexTexture;
}

Texture* ScreenRenderer::getPrevLinearDepthTexture() const {
	return m_prevLinearDepthTexture;
}

Texture* ScreenRenderer::getPrevDepthTexture() const {
	return m_prevDepthTexture;
}

Texture* ScreenRenderer::getReprojectionHistoryTexture() const {
	return m_reprojectionHistoryTexture;
}

Texture* ScreenRenderer::getPrevReprojectionHistoryTexture() const {
	return m_prevReprojectionHistoryTexture;
}

uint32_t ScreenRenderer::getFrameCount() const {
	return m_frameCount;
}

void ScreenRenderer::addFragmentOutputs(ShaderProgram* shaderProgram) {
	shaderProgram->addDataLocation(0, "outNormal");
	shaderProgram->addDataLocation(1, "outTangent");
	shaderProgram->addDataLocation(2, "outVelocity");
	shaderProgram->addDataLocation(3, "outTextureCoord");
	shaderProgram->addDataLocation(4, "outMaterialIndex");
	shaderProgram->addDataLocation(5, "outLinearDepth");
}

uint32_t ScreenRenderer::getGBufferTextureCount() const {
	return 6;
}
