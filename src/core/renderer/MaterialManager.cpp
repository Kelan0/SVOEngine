#include "MaterialManager.h"

MaterialManager::MaterialManager() {
	glGenBuffers(1, &m_materialBuffer);
}

MaterialManager::~MaterialManager() {
	glDeleteBuffers(1, &m_materialBuffer);
}

uint32_t MaterialManager::loadNamedMaterial(std::string materialName, MaterialConfiguration& materialConfiguration) {
	uint32_t& materialIndex = m_namedMaterialIndexes[materialName];

	if (materialIndex == 0) { // NULL
		materialIndex = m_materials.size() + 1;

		m_materials.push_back(new Material(materialConfiguration));
	}

	return materialIndex - 1;
}

Material* MaterialManager::getMaterial(uint32_t materialIndex) {
	assert(materialIndex < m_materials.size());
	return m_materials[materialIndex];
}

Material* MaterialManager::getMaterial(std::string materialName) {
	auto it = m_namedMaterialIndexes.find(materialName);
	if (it == m_namedMaterialIndexes.end()) {
		return NULL;
	}

	return this->getMaterial(it->second);
}

uint32_t MaterialManager::getMaterialCount() const {
	return m_materials.size();
}

void MaterialManager::bindMaterialBuffer(uint32_t index) {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_materialBuffer);
}

void MaterialManager::render(double dt, double partialTicks) {
	this->initializeMaterialBuffer();

	for (int i = 0; i < m_materials.size(); i++) {
		m_materials[i]->makeResident(true);
	}
}

void MaterialManager::applyUniforms(ShaderProgram* shaderProgram) {
	
}

void MaterialManager::initializeMaterialBuffer() {
	if (m_materialCount != m_materials.size()) {
		m_materialCount = m_materials.size();

		std::vector<PackedMaterial> packedMaterials;
		packedMaterials.resize(m_materials.size());

		for (int i = 0; i < m_materials.size(); i++) {
			PackedMaterial packedMaterial;
			Material* material = m_materials[i];

			bool hasAlbedoTexture = material->getAlbedoMap() != NULL;
			bool hasNormalTexture = material->getNormalMap() != NULL;
			bool hasRoughnessTexture = material->getRoughnessMap() != NULL;
			bool hasMetalnessTexture = material->getMetalnessMap() != NULL;
			bool hasAmbientOcclusionTexture = material->getAmbientOcclusionMap() != NULL;
			bool hasAlphaTexture = material->getAlphaMap() != NULL;

			if (hasAlbedoTexture) packedMaterial.albedoTextureHandle = material->getAlbedoMap()->getTextureHandle();
			if (hasNormalTexture) packedMaterial.normalTextureHandle = material->getNormalMap()->getTextureHandle();
			if (hasRoughnessTexture) packedMaterial.roughnessTextureHandle = material->getRoughnessMap()->getTextureHandle();
			if (hasMetalnessTexture) packedMaterial.metalnessTextureHandle = material->getMetalnessMap()->getTextureHandle();
			if (hasAmbientOcclusionTexture) packedMaterial.ambientOcclusionTextureHandle = material->getAmbientOcclusionMap()->getTextureHandle();
			if (hasAlphaTexture) packedMaterial.alphaTextureHandle = material->getAlphaMap()->getTextureHandle();

			packedMaterial.albedoRGBA = u8vec4(clamp(dvec4(material->getAlbedo(), 1.0) * 255.0, 0.0, 255.0));
			packedMaterial.transmissionRGBA = u8vec4(clamp(dvec4(material->getTransmission(), 1.0) * 255.0, 0.0, 255.0));
			packedMaterial.emissionR16 = u16vec1(clamp(material->getEmission().r * 256.0, 0.0, 65535.0));
			packedMaterial.emissionG16 = u16vec1(clamp(material->getEmission().g * 256.0, 0.0, 65535.0));
			packedMaterial.emissionB16 = u16vec1(clamp(material->getEmission().b * 256.0, 0.0, 65535.0));
			packedMaterial.roughnessR8 = u8vec1(clamp(material->getRoughness() * 255.0, 0.0, 255.0));
			packedMaterial.metalnessR8 = u8vec1(clamp(material->getMetalness() * 255.0, 0.0, 255.0));

			packedMaterial.flags = 0u;
			packedMaterial.flags |= (hasAlbedoTexture ? 1 : 0) << 0;
			packedMaterial.flags |= (hasNormalTexture ? 1 : 0) << 1;
			packedMaterial.flags |= (hasRoughnessTexture ? 1 : 0) << 2;
			packedMaterial.flags |= (hasMetalnessTexture ? 1 : 0) << 3;
			packedMaterial.flags |= (hasAmbientOcclusionTexture ? 1 : 0) << 4;
			packedMaterial.flags |= (hasAlphaTexture ? 1 : 0) << 5;
			packedMaterial.flags |= (material->isTransparent() ? 1 : 0) << 6;
			packedMaterial.flags |= (material->isRoughnessInverted() ? 1 : 0) << 7;

			packedMaterials[i] = packedMaterial;
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_materialBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(PackedMaterial) * packedMaterials.size(), &packedMaterials[0], GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}
