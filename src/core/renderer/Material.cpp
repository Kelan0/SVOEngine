#include "core/renderer/Material.h"
#include "core/renderer/Texture.h"
#include "core/renderer/ShaderProgram.h"
#include "core/Engine.h"

Material::Material(MaterialConfiguration configuration) {
	m_albedo = configuration.albedo;
	m_transmission = configuration.transmission;
	m_emission = configuration.emission;
	m_roughness = configuration.roughness;
	m_metalness = configuration.metalness;

	m_minFilter = configuration.minFilter;
	m_magFilter = configuration.magFilter;
	m_anisotropy = configuration.anisotropy;
	m_roughnessInverted = configuration.roughnessInverted;

	std::string albedoMapPath = configuration.albedoMapPath;
	std::string normalMapPath = configuration.normalMapPath;
	std::string roughnessMapPath = configuration.roughnessMapPath;
	std::string metalnessMapPath = configuration.metalnessMapPath;
	std::string ambientOcclusionMapPath = configuration.ambientOcclusionMapPath;
	std::string alphaMapPath = configuration.alphaMapPath;

	if (!configuration.materialResourcePath.empty()) {
		if (albedoMapPath.empty()) albedoMapPath = configuration.materialResourcePath + "/albedo.png";
		if (normalMapPath.empty()) normalMapPath = configuration.materialResourcePath + "/normal.png";
		if (roughnessMapPath.empty()) roughnessMapPath = configuration.materialResourcePath + "/roughness.png";
		if (metalnessMapPath.empty()) metalnessMapPath = configuration.materialResourcePath + "/metalness.png";
		if (ambientOcclusionMapPath.empty()) ambientOcclusionMapPath = configuration.materialResourcePath + "/ambientOcclusion.png";
		if (alphaMapPath.empty()) alphaMapPath = configuration.alphaMapPath + "/alpha.png";
	}

	//m_albedoMap = NULL;
	//m_ownsAlbedoMap = false;
	if (configuration.albedoMap != NULL) {
		m_albedoMap = configuration.albedoMap;
	} else if (!albedoMapPath.empty()) {
		//Texture2D* map = NULL;
		//if (Texture2D::load(albedoMapPath, &map)) {
		Resource<Texture2D> map = Engine::resourceHandler()->request<Texture2D>(albedoMapPath, albedoMapPath);
		if (map != NULL && map.status() == ResourceStatus::Ready) {
			map->setFilterMode(m_minFilter, m_magFilter, m_anisotropy);
			m_albedoMap = map;
			//m_ownsAlbedoMap = true;
		}
	}

	//m_normalMap = NULL;
	//m_ownsNormalMap = false;
	if (configuration.normalMap != NULL) {
		m_normalMap = configuration.normalMap;
	} else if (!normalMapPath.empty()) {
		//Texture2D* map = NULL;
		//if (Texture2D::load(normalMapPath, &map)) {
		Resource<Texture2D> map = Engine::resourceHandler()->request<Texture2D>(normalMapPath, normalMapPath);
		if (map != NULL && map.status() == ResourceStatus::Ready) {
			map->setFilterMode(m_minFilter, m_magFilter, m_anisotropy);
			m_normalMap = map;
			//m_ownsNormalMap = true;
		}
	}

	//m_roughnessMap = NULL;
	//m_ownsRoughnessMap = false;
	if (configuration.roughnessMap != NULL) {
		m_roughnessMap = configuration.roughnessMap;
	} else if (!roughnessMapPath.empty()) {
		//Texture2D* map = NULL;
		//if (Texture2D::load(roughnessMapPath, &map)) {
		Resource<Texture2D> map = Engine::resourceHandler()->request<Texture2D>(roughnessMapPath, roughnessMapPath);
		if (map != NULL && map.status() == ResourceStatus::Ready) {
			map->setFilterMode(m_minFilter, m_magFilter, m_anisotropy);
			m_roughnessMap = map;
			//m_ownsRoughnessMap = true;
		}
	}

	//m_metalnessMap = NULL;
	//m_ownsMetalnessMap = false;
	if (configuration.metalnessMap != NULL) {
		m_metalnessMap = configuration.metalnessMap;
	} else if (!metalnessMapPath.empty()) {
		//Texture2D* map = NULL;
		//if (Texture2D::load(metalnessMapPath, &map)) {
		Resource<Texture2D> map = Engine::resourceHandler()->request<Texture2D>(metalnessMapPath, metalnessMapPath);
		if (map != NULL && map.status() == ResourceStatus::Ready) {
			map->setFilterMode(m_minFilter, m_magFilter, m_anisotropy);
			m_metalnessMap = map;
			//m_ownsMetalnessMap = true;
		}
	}

	//m_ambientOcclusionMap = NULL;
	//m_ownsAmbientOcclusionMap = false;
	if (configuration.ambientOcclusionMap != NULL) {
		m_ambientOcclusionMap = configuration.ambientOcclusionMap;
	} else if (!ambientOcclusionMapPath.empty()) {
		//Texture2D* map = NULL;
		//if (Texture2D::load(ambientOcclusionMapPath, &map)) {
		Resource<Texture2D> map = Engine::resourceHandler()->request<Texture2D>(ambientOcclusionMapPath, ambientOcclusionMapPath);
		if (map != NULL && map.status() == ResourceStatus::Ready) {
			map->setFilterMode(m_minFilter, m_magFilter, m_anisotropy);
			m_ambientOcclusionMap = map;
			//m_ownsAmbientOcclusionMap = true;
		}
	}

	//m_alphaMap = NULL;
	//m_ownsAlphaMap = false;
	if (configuration.alphaMap != NULL) {
		m_alphaMap = configuration.alphaMap;
	} else if (!alphaMapPath.empty()) {
		//Texture2D* map = NULL;
		//if (Texture2D::load(alphaMapPath, &map)) {
		Resource<Texture2D> map = Engine::resourceHandler()->request<Texture2D>(alphaMapPath, alphaMapPath);
		if (map != NULL && map.status() == ResourceStatus::Ready) {
			map->setFilterMode(m_minFilter, m_magFilter, m_anisotropy);
			m_alphaMap = map;
			//m_ownsAlphaMap = true;
		}
	}


	double eps = 1.0 / 255.0;
	if (m_alphaMap != NULL || m_transmission.r > eps || m_transmission.g > eps || m_transmission.b > eps) {
		m_transparent = true;
	} else {
		m_transparent = false;
	}
}

Material::~Material() {

	this->makeResident(false);

	//info("-DECREASED ENGINE MATERIALS TO %d\n", --Engine::materialCount);

	//info("Deleting material\n");
	//if (m_ownsAlbedoMap) delete m_albedoMap;
	//m_albedoMap = NULL;
	Engine::resourceHandler()->release(&m_albedoMap);

	//if (m_ownsNormalMap) delete m_normalMap;
	//m_normalMap = NULL;
	Engine::resourceHandler()->release(&m_normalMap);

	//if (m_ownsRoughnessMap) delete m_roughnessMap;
	//m_roughnessMap = NULL;
	Engine::resourceHandler()->release(&m_roughnessMap);

	//if (m_ownsMetalnessMap) delete m_metalnessMap;
	//m_metalnessMap = NULL;
	Engine::resourceHandler()->release(&m_metalnessMap);

	//if (m_ownsAmbientOcclusionMap) delete m_ambientOcclusionMap;
	//m_ambientOcclusionMap = NULL;
	Engine::resourceHandler()->release(&m_ambientOcclusionMap);

	//if (m_ownsAlphaMap) delete m_alphaMap;
	//m_alphaMap = NULL;
	Engine::resourceHandler()->release(&m_alphaMap);
}

void Material::bind(ShaderProgram* shaderProgram, std::string uniformName, int32_t textureUnit) const {
	assert(shaderProgram != NULL);
	assert(!uniformName.empty());
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// TODO: BINDLESS TEXTURES
	shaderProgram->setUniform(uniformName + ".albedo", fvec3(m_albedo));
	shaderProgram->setUniform(uniformName + ".transmission", fvec3(m_transmission));
	shaderProgram->setUniform(uniformName + ".emission", fvec3(m_emission));
	shaderProgram->setUniform(uniformName + ".roughness", float(m_roughness));
	shaderProgram->setUniform(uniformName + ".metalness", float(m_metalness));
	shaderProgram->setUniform(uniformName + ".roughnessInverted", m_roughnessInverted);

	shaderProgram->setUniform(uniformName + ".hasAlbedoMap", m_albedoMap != NULL);
	if (m_albedoMap != NULL) {
		m_albedoMap->bind(textureUnit + 0);
		shaderProgram->setUniform(uniformName + ".albedoMap", textureUnit + 0);
	}

	shaderProgram->setUniform(uniformName + ".hasNormalMap", m_normalMap != NULL);
	if (m_normalMap != NULL) {
		m_normalMap->bind(textureUnit + 1);
		shaderProgram->setUniform(uniformName + ".normalMap", textureUnit + 1);
	}

	shaderProgram->setUniform(uniformName + ".hasRoughnessMap", m_roughnessMap != NULL);
	if (m_roughnessMap != NULL) {
		m_roughnessMap->bind(textureUnit + 2);
		shaderProgram->setUniform(uniformName + ".roughnessMap", textureUnit + 2);
	}

	shaderProgram->setUniform(uniformName + ".hasMetalnessMap", m_metalnessMap != NULL);
	if (m_metalnessMap != NULL) {
		m_metalnessMap->bind(textureUnit + 3);
		shaderProgram->setUniform(uniformName + ".metalnessMap", textureUnit + 3);
	}

	shaderProgram->setUniform(uniformName + ".hasAmbientOcclusionMap", m_ambientOcclusionMap != NULL);
	if (m_ambientOcclusionMap != NULL) {
		m_ambientOcclusionMap->bind(textureUnit + 4);
		shaderProgram->setUniform(uniformName + ".ambientOcclusionMap", textureUnit + 4);
	}

	shaderProgram->setUniform(uniformName + ".hasAlphaMap", m_alphaMap != NULL);
	if (m_alphaMap != NULL) {
		m_alphaMap->bind(textureUnit + 5);
		shaderProgram->setUniform(uniformName + ".alphaMap", textureUnit + 5);
	}

	if (this->isDoubleSided() || this->isTransparent()) {
		glDisable(GL_CULL_FACE);
	} else {
		glEnable(GL_CULL_FACE);
	}
}

void Material::unbind() const {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Material::makeResident(bool resident) {
	if (m_albedoMap != NULL) m_albedoMap->makeResident(resident);
	if (m_normalMap != NULL) m_normalMap->makeResident(resident);
	if (m_roughnessMap != NULL) m_roughnessMap->makeResident(resident);
	if (m_metalnessMap != NULL) m_metalnessMap->makeResident(resident);
	if (m_ambientOcclusionMap != NULL) m_ambientOcclusionMap->makeResident(resident);
	if (m_alphaMap != NULL) m_alphaMap->makeResident(resident);
}

Resource<Texture2D> Material::getAlbedoMap() const {
	return m_albedoMap;
}

Resource<Texture2D> Material::getNormalMap() const {
	return m_normalMap;
}

Resource<Texture2D> Material::getRoughnessMap() const {
	return m_roughnessMap;
}

Resource<Texture2D> Material::getMetalnessMap() const {
	return m_metalnessMap;
}

Resource<Texture2D> Material::getAmbientOcclusionMap() const {
	return m_ambientOcclusionMap;
}

Resource<Texture2D> Material::getAlphaMap() const {
	return m_alphaMap;
}

dvec3 Material::getAlbedo() const {
	return m_albedo;
}

dvec3 Material::getTransmission() const {
	return m_transmission;
}

dvec3 Material::getEmission() const {
	return m_emission;
}

double Material::getRoughness() const {
	return m_roughness;
}

double Material::getMetalness() const {
	return m_metalness;
}

bool Material::isRoughnessInverted() const {
	return m_roughnessInverted;
}

bool Material::isTransparent() const {
	return m_transparent;
}

bool Material::isDoubleSided() const {
	return m_doubleSided;
}

TextureFilter Material::getMinificationFilter() const {
	return m_minFilter;
}

TextureFilter Material::getMagnificationFilter() const {
	return m_magFilter;
}

double Material::getAnisotropy() const {
	return m_anisotropy;
}

void Material::setFilterMode(TextureFilter minFilter, TextureFilter magFilter, double anisotropy) {
	if (minFilter == m_minFilter && magFilter == m_magFilter && epsilonEqual(anisotropy, m_anisotropy, 0.01)) {
		return;
	}

	if (m_albedoMap != NULL) m_albedoMap->setFilterMode(minFilter, magFilter, anisotropy);
	if (m_normalMap != NULL) m_normalMap->setFilterMode(minFilter, magFilter, anisotropy);
	if (m_roughnessMap != NULL) m_roughnessMap->setFilterMode(minFilter, magFilter, anisotropy);
	if (m_metalnessMap != NULL) m_metalnessMap->setFilterMode(minFilter, magFilter, anisotropy);
	if (m_ambientOcclusionMap != NULL) m_ambientOcclusionMap->setFilterMode(minFilter, magFilter, anisotropy);
	if (m_alphaMap != NULL) m_alphaMap->setFilterMode(minFilter, magFilter, anisotropy);
}
