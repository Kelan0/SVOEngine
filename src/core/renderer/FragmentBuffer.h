#pragma once
#include "core/pch.h"

class Texture;

struct PackedFragment {
	uint32_t albedoRGB_roughnessR; // rgb8 + r8
	uint32_t transmissionRGB_metalnessR; // rgb8 + r8
	uint32_t normal; // rg16f
	uint32_t irradianceRG; // rg16f
	uint32_t irradianceB_reflectionR; // r16f + r16f
	uint32_t reflectionGB; // rg16f
	float depth; // r32f
};

class FragmentBuffer : public NotCopyable {
public:
	FragmentBuffer(uint32_t width, uint32_t height);

	~FragmentBuffer();

	void setResolution(uint32_t width, uint32_t height);

	Texture* getAlbedoTexture() const;

	Texture* getNormalTexture() const;

	Texture* getRoughnessTexture() const;

	Texture* getMetalnessTexture() const;

	Texture* getAmbientOcclusionTexture() const;

	Texture* getIrradianceTexture() const;

	Texture* getReflectionTexture() const;

	Texture* getDepthTexture() const;

	uint32_t getWidth() const;

	uint32_t getHeight() const;

private:
	Texture* m_albedoTexture;
	Texture* m_normalTexture;
	Texture* m_roughnessTexture;
	Texture* m_metalnessTexture;
	Texture* m_ambientOcclusionTexture;
	Texture* m_irradianceTexture;
	Texture* m_reflectionTexture;
	Texture* m_depthTexture;

	uint32_t m_width;
	uint32_t m_height;
};

