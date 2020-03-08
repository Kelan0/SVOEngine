#include "core/renderer/RaytraceRenderer.h"
#include "core/renderer/Texture.h"
#include "core/renderer/ShaderProgram.h"
#include "core/renderer/EnvironmentMap.h"
#include "core/renderer/ScreenRenderer.h"
#include "core/renderer/MaterialManager.h"
#include "core/renderer/geometry/GeometryBuffer.h"
#include "core/scene/Scene.h"
#include "core/Engine.h"

RaytraceRenderer::RaytraceRenderer(uint32_t renderWidth, uint32_t renderHeight):
	m_renderWidth(renderWidth),
	m_renderHeight(renderHeight) {

	m_texture = new Texture2D(renderWidth, renderHeight, TextureFormat::R32_G32_B32_A32_FLOAT, TextureFilter::NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	this->setRenderResolution(renderWidth, renderHeight);

	m_raytraceShader = new ShaderProgram();
	m_raytraceShader->addShader(GL_COMPUTE_SHADER, "shaders/raytrace/lighting/comp.glsl");
	m_raytraceShader->completeProgram();
}

RaytraceRenderer::~RaytraceRenderer() {
	delete m_texture;
	delete m_raytraceShader;
}

void RaytraceRenderer::render(double dt, double partialTicks) {
	constexpr int workgroupSizeX = 4;
	constexpr int workgroupSizeY = 4;
	ShaderProgram::use(m_raytraceShader);
	Engine::scene()->applyUniforms(m_raytraceShader);
	Engine::scene()->getStaticGeometryBuffer()->bindVertexBuffer(0);
	Engine::scene()->getStaticGeometryBuffer()->bindTriangleBuffer(1);
	Engine::scene()->getStaticGeometryBuffer()->bindBVHNodeBuffer(2);
	Engine::scene()->getStaticGeometryBuffer()->bindBVHReferenceBuffer(3);
	Engine::scene()->getMaterialManager()->bindMaterialBuffer(4);
	this->bindTexture(0);
	int32_t index = 0;
	
	LightProbe* skybox = Engine::scene()->getGlobalEnvironmentMap();
	
	Engine::screenRenderer()->getAlbedoTexture()->bind(index);
	m_raytraceShader->setUniform("albedoTexture", index++);
	
	Engine::screenRenderer()->getEmissionTexture()->bind(index);
	m_raytraceShader->setUniform("emissionTexture", index++);
	
	Engine::screenRenderer()->getNormalTexture()->bind(index);
	m_raytraceShader->setUniform("normalTexture", index++);
	
	Engine::screenRenderer()->getRoughnessTexture()->bind(index);
	m_raytraceShader->setUniform("roughnessTexture", index++);
	
	Engine::screenRenderer()->getMetalnessTexture()->bind(index);
	m_raytraceShader->setUniform("metalnessTexture", index++);
	
	Engine::screenRenderer()->getAmbientOcclusionTexture()->bind(index);
	m_raytraceShader->setUniform("ambientOcclusionTexture", index++);
	
	Engine::screenRenderer()->getIrradianceTexture()->bind(index);
	m_raytraceShader->setUniform("irradianceTexture", index++);
	
	Engine::screenRenderer()->getReflectionTexture()->bind(index);
	m_raytraceShader->setUniform("reflectionTexture", index++);
	
	Engine::screenRenderer()->getDepthTexture()->bind(index);
	m_raytraceShader->setUniform("depthTexture", index++);
	
	skybox->bindEnvironmentMap(index);
	m_raytraceShader->setUniform("skyboxEnvironmentTexture", index++);
	
	skybox->bindEnvironmentMap(index);
	m_raytraceShader->setUniform("skyboxEnvironmentTexture", index++);
	
	skybox->bindDiffuseIrradianceMap(index);
	m_raytraceShader->setUniform("skyboxDiffuseIrradianceTexture", index++);
	
	skybox->bindSpecularReflectionMap(index);
	m_raytraceShader->setUniform("skyboxPrefilteredEnvironmentTexture", index++);
	
	LightProbe::getBRDFIntegrationMap()->bind(index);
	m_raytraceShader->setUniform("BRDFIntegrationMap", index++);
	
	glDispatchCompute((m_renderWidth + workgroupSizeX) / workgroupSizeX, (m_renderHeight + workgroupSizeY) / workgroupSizeY, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void RaytraceRenderer::applyUniforms(ShaderProgram* program) {

}

uint32_t RaytraceRenderer::getRenderWidth() const {
	return m_renderWidth;
}

uint32_t RaytraceRenderer::getRenderHeight() const {
	return m_renderHeight;
}

void RaytraceRenderer::setRenderResolution(uint32_t width, uint32_t height) {
	if (width != m_renderWidth || height != m_renderHeight) {
		m_renderWidth = width;
		m_renderHeight = height;

		dynamic_cast<Texture2D*>(m_texture)->setSize(width, height);
	}
}

void RaytraceRenderer::bindTexture(uint32_t unit) {
	glBindImageTexture(unit, m_texture->getTextureName(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}
