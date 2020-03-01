#pragma once

#include "core/pch.h"
#include "core/renderer/Material.h"

class ShaderProgram;

struct PackedMaterial {
	union {
		uint64_t albedoTextureHandle;
		uvec2 albedoTexturePackedHandle;
	};

	union {
		uint64_t normalTextureHandle;
		uvec2 normalTexturePackedHandle;
	};

	union {
		uint64_t roughnessTextureHandle;
		uvec2 roughnessTexturePackedHandle;
	};

	union {
		uint64_t metalnessTextureHandle;
		uvec2 metalnessTexturePackedHandle;
	};

	union {
		uint64_t ambientOcclusionTextureHandle;
		uvec2 ambientOcclusionTexturePackedHandle;
	};

	union {
		uint64_t alphaTextureHandle;
		uvec2 alphaTexturePackedHandle;
	};

	union {
		u8vec4 albedo;
		uint32_t packedAlbedo;
	};

	union {
		u8vec4 transmission;
		uint32_t packedTransmission;
	};

	union {
		struct {
			u16vec1 roughnessR16;
			u16vec1 metalnessR16;
		};
		uint32_t roughnessR16_metalnessR16;
	};

	uint32_t flags;
};

class MaterialManager {
public:
	MaterialManager();

	~MaterialManager();

	uint32_t loadMaterial(MaterialConfiguration& materialConfiguration);

	uint32_t loadNamedMaterial(std::string materialName, MaterialConfiguration& materialConfiguration);

	Material* getMaterial(uint32_t index);

	uint32_t getMaterialCount() const;

	void bindMaterialBuffer(uint32_t index);

	void render(double dt, double partialTicks);

	void applyUniforms(ShaderProgram* shaderProgram);

private:
	void initializeMaterialBuffer();
	
	std::vector<Material*> m_materials;
	std::map<std::string, uint32_t> m_materialNames;

	uint32_t m_materialBuffer;
	uint32_t m_materialCount;
};

