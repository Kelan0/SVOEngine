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
		u8vec4 albedoRGBA;
		uint32_t packedAlbedoRGBA;
	};

	union {
		u8vec4 transmissionRGBA;
		uint32_t packedTransmissionRGBA;
	};

	union {
		struct {
			u16vec1 emissionR16;
			u16vec1 emissionG16;
		};
		uint32_t packedEmissionRG;
	};

	union {
		struct {
			u16vec1 emissionB16;
			struct {
				u8vec1 roughnessR8;
				u8vec1 metalnessR8;
			};
		};
		uint32_t packedEmissionB_roughnessR_metalnessR;
	};

	uint32_t flags;
	uint32_t padding0;
	uint32_t padding1;
	uint32_t padding2;
};

class MaterialManager {
public:
	MaterialManager();

	~MaterialManager();

	uint32_t loadNamedMaterial(std::string materialName, MaterialConfiguration& materialConfiguration);

	Material* getMaterial(uint32_t materialIndex);

	Material* getMaterial(std::string materialName);

	uint32_t getMaterialCount() const;

	void bindMaterialBuffer(uint32_t index);

	void render(double dt, double partialTicks);

	void applyUniforms(ShaderProgram* shaderProgram);

private:
	void initializeMaterialBuffer();
	
	std::vector<Material*> m_materials;
	std::map<std::string, uint32_t> m_namedMaterialIndexes;

	uint32_t m_materialBuffer;
	uint32_t m_materialCount;
};

