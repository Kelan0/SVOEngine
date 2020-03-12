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
	m_frequencyScale(1),
	m_renderWidth(1),
	m_renderHeight(1) {

	m_lowResolutionFrameTexture = new Texture2D(1, 1, TextureFormat::R32_G32_B32_A32_FLOAT, TextureFilter::LINEAR_PIXEL, TextureFilter::LINEAR_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	m_FullResolutionFrameTexture = new Texture2D(1, 1, TextureFormat::R32_G32_B32_A32_FLOAT, TextureFilter::LINEAR_PIXEL, TextureFilter::LINEAR_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
	this->setRenderResolution(renderWidth, renderHeight);

	m_raytraceShader = new ShaderProgram();
	m_raytraceShader->addShader(GL_COMPUTE_SHADER, "shaders/raytrace/lighting/comp.glsl");
	m_raytraceShader->completeProgram();
}

RaytraceRenderer::~RaytraceRenderer() {
	delete m_lowResolutionFrameTexture;
	delete m_raytraceShader;
}

void RaytraceRenderer::render(double dt, double partialTicks) {
	constexpr int workgroupSizeX = 8;
	constexpr int workgroupSizeY = 8;
	ShaderProgram::use(m_raytraceShader);
	Engine::scene()->applyUniforms(m_raytraceShader);
	Engine::scene()->getStaticGeometryBuffer()->bindVertexBuffer(0);
	Engine::scene()->getStaticGeometryBuffer()->bindTriangleBuffer(1);
	Engine::scene()->getStaticGeometryBuffer()->bindBVHNodeBuffer(2);
	Engine::scene()->getStaticGeometryBuffer()->bindBVHReferenceBuffer(3);
	Engine::scene()->getStaticGeometryBuffer()->bindEmissiveTriangleBuffer(4);
	Engine::scene()->getMaterialManager()->bindMaterialBuffer(5);
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


	//m_raytraceShader->setUniform("lowResolutionFramePass", true);
	//glBindImageTexture(0, m_lowResolutionFrameTexture->getTextureName(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	//glDispatchCompute((m_lowResolutionFrameTexture->getWidth() + workgroupSizeX) / workgroupSizeX, (m_lowResolutionFrameTexture->getHeight() + workgroupSizeY) / workgroupSizeY, 1);
	//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	m_raytraceShader->setUniform("lowResolutionFramePass", false);
	m_lowResolutionFrameTexture->bind(index);
	m_raytraceShader->setUniform("lowResolutionFrame", index++);
	glBindImageTexture(0, m_FullResolutionFrameTexture->getTextureName(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glDispatchCompute((m_FullResolutionFrameTexture->getWidth() + workgroupSizeX) / workgroupSizeX, (m_FullResolutionFrameTexture->getHeight() + workgroupSizeY) / workgroupSizeY, 1);
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

		dynamic_cast<Texture2D*>(m_lowResolutionFrameTexture)->setSize(m_renderWidth / m_frequencyScale, m_renderHeight / m_frequencyScale);
		dynamic_cast<Texture2D*>(m_FullResolutionFrameTexture)->setSize(m_renderWidth, m_renderHeight);
	}
}

void RaytraceRenderer::bindFrameTexture(uint32_t unit) {
	glBindImageTexture(unit, m_FullResolutionFrameTexture->getTextureName(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}

Texture2D* RaytraceRenderer::getFrameTexture() {
	return m_FullResolutionFrameTexture;
}