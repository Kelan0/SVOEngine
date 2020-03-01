#include "core/renderer/EnvironmentMap.h"
#include "core/renderer/ShaderProgram.h"

Texture2D* LightProbe::s_BRDFIntegrationMap = NULL;
ShaderProgram* LightProbe::s_diffuseIrradianceConvolutionShader = NULL;
ShaderProgram* LightProbe::s_specularIrradianceConvolutionShader = NULL;
ShaderProgram* LightProbe::s_specularRoughnessConvolutionShader = NULL;
ShaderProgram* LightProbe::s_BRDFIntegrationShader = NULL;

LightProbe::LightProbe(CubeMap* environmentMap) {
	this->setEnvironmentMap(environmentMap);
}

LightProbe::LightProbe(CubemapConfiguration environmentMapConfig) {
	this->setEnvironmentMap(environmentMapConfig);
}

LightProbe::~LightProbe() {
	delete this->m_environmentMap;
	delete this->m_diffuseIrradianceMap;
}

bool LightProbe::setEnvironmentMap(CubeMap* environmentMap) {
	m_environmentMap = environmentMap;
	return true;
}

bool LightProbe::setEnvironmentMap(CubemapConfiguration environmentMapConfig) {
	CubeMap* environmentMap = NULL;
	if (CubeMap::load(environmentMapConfig, &environmentMap)) {
		return this->setEnvironmentMap(environmentMap);
	}
	return false;
}

LightProbe* LightProbe::calculateDiffuseIrradianceMap() {
	if (s_diffuseIrradianceConvolutionShader == NULL) {
		s_diffuseIrradianceConvolutionShader = new ShaderProgram();
		s_diffuseIrradianceConvolutionShader->addShader(GL_COMPUTE_SHADER, "shaders/compute/irradiance/diffuse/comp.glsl");
		s_diffuseIrradianceConvolutionShader->completeProgram();
	}

	int envMapWidth = (int) m_environmentMap->getWidth();
	int envMapHeight = (int) m_environmentMap->getHeight();
	int irrMapWidth = 32;
	int irrMapHeight = 32;

	if (m_diffuseIrradianceMap == NULL) {
		m_diffuseIrradianceMap = new CubeMap(
			irrMapWidth,
			irrMapHeight,
			m_environmentMap->getFormat(), 
			TextureFilter::LINEAR_PIXEL,//m_environmentMap->getMinificationFilterMode(), 
			TextureFilter::LINEAR_PIXEL,//m_environmentMap->getMagnificationFilterMode(), 
			m_environmentMap->getUWrapFilterMode(),
			m_environmentMap->getVWrapFilterMode(), 
			m_environmentMap->getWWrapFilterMode()
		);
		m_diffuseIrradianceMap->setSize(irrMapWidth, irrMapHeight, GL_RGBA);
	}

	OpenGLTextureFormat format = Texture::getOpenGLTextureFormat(m_environmentMap->getFormat());
	ShaderProgram::use(s_diffuseIrradianceConvolutionShader);
	s_diffuseIrradianceConvolutionShader->setUniform("srcMapSize", envMapWidth, envMapHeight);
	s_diffuseIrradianceConvolutionShader->setUniform("dstMapSize", irrMapWidth, irrMapHeight);
	glBindImageTexture(0, m_environmentMap->getTextureName(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, m_diffuseIrradianceMap->getTextureName(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glDispatchCompute((int)ceil(irrMapWidth), (int)ceil(irrMapHeight), 6);
	ShaderProgram::use(NULL);

	return this;
}

LightProbe* LightProbe::calculateSpecularReflectionMap() {
	if (s_specularIrradianceConvolutionShader == NULL) {
		s_specularIrradianceConvolutionShader = new ShaderProgram();
		s_specularIrradianceConvolutionShader->addShader(GL_COMPUTE_SHADER, "shaders/compute/irradiance/specular/comp.glsl");
		s_specularIrradianceConvolutionShader->completeProgram();
	}

	if (s_specularRoughnessConvolutionShader == NULL) {
		s_specularRoughnessConvolutionShader = new ShaderProgram();
		s_specularRoughnessConvolutionShader->addShader(GL_COMPUTE_SHADER, "shaders/compute/irradiance/roughnessFilter/comp.glsl");
		s_specularRoughnessConvolutionShader->completeProgram();
	}

	int envMapWidth = (int)m_environmentMap->getWidth();
	int envMapHeight = (int)m_environmentMap->getHeight();
	int prfMapWidth = 256;
	int prfMapHeight = 256;

	int levels = 5;

	if (m_specularReflectionMap == NULL) {
		m_specularReflectionMap = new CubeMap(
			prfMapWidth,
			prfMapHeight,
			m_environmentMap->getFormat(),
			TextureFilter::LINEAR_MIPMAP_LINEAR_PIXEL,//m_environmentMap->getMinificationFilterMode(), 
			TextureFilter::LINEAR_PIXEL,//m_environmentMap->getMagnificationFilterMode(), 
			m_environmentMap->getUWrapFilterMode(),
			m_environmentMap->getVWrapFilterMode(),
			m_environmentMap->getWWrapFilterMode()
		);
		m_specularReflectionMap->setSize(prfMapWidth, prfMapHeight, GL_RGBA);
	}

	m_specularReflectionMap->generateMipmap(levels);
	m_environmentMap->generateMipmap(levels);

	for (int i = 0; i <= levels; i++) {
		int scale = 1 << i;
		int srcMapWidth = envMapWidth;
		int srcMapHeight = envMapHeight;
		int dstMapWidth = prfMapWidth / scale;
		int dstMapHeight = prfMapHeight / scale;
		float roughness = (float) i / (levels);

		ShaderProgram::use(s_specularRoughnessConvolutionShader);
		s_specularRoughnessConvolutionShader->setUniform("srcMapSize", srcMapWidth, srcMapHeight);
		s_specularRoughnessConvolutionShader->setUniform("dstMapSize", dstMapWidth, dstMapHeight);
		s_specularRoughnessConvolutionShader->setUniform("roughness", roughness);
		glBindImageTexture(0, m_environmentMap->getTextureName(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, m_specularReflectionMap->getTextureName(), i, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glDispatchCompute((int)ceil(dstMapWidth), (int)ceil(dstMapWidth), 6);
	}

	ShaderProgram::use(NULL);
	return this;
}

CubeMap* LightProbe::getEnvironmentMap() const {
	return m_environmentMap;
}

CubeMap* LightProbe::getDiffuseIrradianceMap() const {
	return m_diffuseIrradianceMap;
}

CubeMap* LightProbe::getSpecularReflectionMap() const {
	return m_specularReflectionMap;
}

bool LightProbe::bindEnvironmentMap(uint32_t textureUnit) const {
	if (m_environmentMap != NULL) {
		m_environmentMap->bind(textureUnit);
		return true;
	}
	return false;
}

bool LightProbe::bindDiffuseIrradianceMap(uint32_t textureUnit) const {
	if (m_diffuseIrradianceMap != NULL) {
		m_diffuseIrradianceMap->bind(textureUnit);
		return true;
	}
	return false;
}

bool LightProbe::bindSpecularReflectionMap(uint32_t textureUnit) const {
	if (m_specularReflectionMap != NULL) {
		m_specularReflectionMap->bind(textureUnit);
		return true;
	}
	return false;
}

Texture2D* LightProbe::getBRDFIntegrationMap() {
	if (s_BRDFIntegrationMap == NULL) {
		if (s_BRDFIntegrationShader == NULL) {
			s_BRDFIntegrationShader = new ShaderProgram();
			s_BRDFIntegrationShader->addShader(GL_COMPUTE_SHADER, "shaders/compute/irradiance/BRDFIntegrationMap/comp.glsl");
			s_BRDFIntegrationShader->completeProgram();
		}

		s_BRDFIntegrationMap = new Texture2D(1024, 1024, TextureFormat::R32_G32_FLOAT, TextureFilter::LINEAR_PIXEL, TextureFilter::LINEAR_PIXEL, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
		//s_BRDFIntegrationMap->setSize(1024, 1024, GL_RGBA);

		int w = (int)s_BRDFIntegrationMap->getWidth();
		int h = (int)s_BRDFIntegrationMap->getHeight();

		ShaderProgram::use(s_BRDFIntegrationShader);
		s_BRDFIntegrationShader->setUniform("mapSize", w, h);
		glBindImageTexture(0, s_BRDFIntegrationMap->getTextureName(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32F);
		glDispatchCompute(w / 8, h / 8, 1);
		ShaderProgram::use(NULL);
	}

	return s_BRDFIntegrationMap;
}
