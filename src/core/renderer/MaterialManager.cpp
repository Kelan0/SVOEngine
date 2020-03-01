#include "MaterialManager.h"

MaterialManager::MaterialManager() {
	glGenBuffers(1, &m_materialBuffer);
}

MaterialManager::~MaterialManager() {
	glDeleteBuffers(1, &m_materialBuffer);
}

uint32_t MaterialManager::loadMaterial(MaterialConfiguration& materialConfiguration) {
	Material* material = new Material(materialConfiguration);
	uint32_t materialIndex = m_materials.size();
	info("Initialized material index %d\n", materialIndex);

	m_materials.push_back(material);
	return materialIndex;
}

uint32_t MaterialManager::loadNamedMaterial(std::string materialName, MaterialConfiguration& materialConfiguration) {
	uint32_t& materialIndex = m_materialNames[materialName];

	if (materialIndex == 0) { // NULL
		materialIndex = this->loadMaterial(materialConfiguration) + 1;
	}

	return materialIndex - 1;
}

Material* MaterialManager::getMaterial(uint32_t index) {
	assert(index < m_materials.size());
	return m_materials[index];
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

			packedMaterial.albedo = u8vec4(dvec4(material->getAlbedo(), 1.0) * 255.0);
			packedMaterial.transmission = u8vec4(dvec4(material->getTransmission(), 1.0) * 255.0);
			packedMaterial.roughnessR16 = u16vec1(material->getRoughness() * 65536.0);
			packedMaterial.metalnessR16 = u16vec1(material->getMetalness() * 65536.0);

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

		info("Initializing %d materials\n", packedMaterials.size());

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_materialBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(PackedMaterial) * packedMaterials.size(), &packedMaterials[0], GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}
