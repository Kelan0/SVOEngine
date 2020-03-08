#pragma once

#include "core/pch.h"

class ShaderProgram;
class Texture;
class Framebuffer;
class LayeredDepthBuffer;

struct PackedFragment {
	uint32_t albedoRGB_roughnessR; // rgb8 + r8
	uint32_t transmissionRGB_metalnessR; // rgb8 + r8
	uint32_t normal; // rg16f
	uint32_t irradianceRG; // rg16f
	uint32_t irradianceB_reflectionR; // r16f + r16f
	uint32_t reflectionGB; // rg16f
	float depth; // r32f
};

//struct Fragment {
//	vec3 albedo; // 3 bytes
//	vec2 normal; // 4 bytes
//	float roughness; // 2 bytes
//	float metalness; // 2 bytes
//	float ambientOcclusion; // 2 bytes
//	vec3 irradiance; // 6 bytes
//	vec3 reflection; // 6 bytes
//	float depth; // 4 bytes 
//	uint next; // 4 bytes
//};

class ScreenRenderer : public NotCopyable {
public:
	ScreenRenderer(uint32_t width, uint32_t height);

	virtual ~ScreenRenderer();

	void drawFullscreenQuad();

	void render(double dt, double partialTicks);

	void applyUniforms(ShaderProgram* shaderProgram);

	void setResolution(uint32_t width, uint32_t height);

	void setAllocatedNodes(uint32_t allocatedNodes);

	void initViewport() const;

	void bindFramebuffer() const;

	void unbindFramebuffer() const;

	void setFramebufferWriteEnabled(bool enabled);

	Framebuffer* getFramebuffer() const;

	Texture* getAlbedoTexture() const;

	Texture* getEmissionTexture() const;

	Texture* getNormalTexture() const;

	Texture* getRoughnessTexture() const;

	Texture* getMetalnessTexture() const;

	Texture* getAmbientOcclusionTexture() const;

	Texture* getIrradianceTexture() const;

	Texture* getReflectionTexture() const;

	Texture* getDepthTexture() const;

	Texture* getPrevDepthTexture() const;

	Texture* getReprojectionHistoryTexture() const;

	static void addFragmentOutputs(ShaderProgram* shaderProgram);

	uint32_t getGBufferTextureCount() const;

	//LayeredDepthBuffer* getLayeredDepthBuffer();

	Framebuffer* t_cubeDepthFramebuffer;
	Texture* t_cubeDepthTexture;

protected:
	Framebuffer* m_framebuffer;
	Texture* m_albedoTexture;
	Texture* m_emissionTexture;
	Texture* m_normalTexture;
	Texture* m_roughnessTexture;
	Texture* m_metalnessTexture;
	Texture* m_ambientOcclusionTexture;
	Texture* m_irradianceTexture;
	Texture* m_reflectionTexture;
	Texture* m_depthTexture;
	Texture* m_prevDepthTexture;
	Texture* m_reprojectionHistoryTexture;

	uint32_t m_pixelNodeHeadTexture;
	uint32_t m_pixelNodeBuffer;
	uint32_t m_pixelNodeCounter;

	//LayeredDepthBuffer* m_layeredDepthBuffer;

	ShaderProgram* m_screenShader;
	uint32_t m_fullscreenQuadVAO;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_layers;
	uint32_t m_allocatedNodes;

	Texture* m_pointLightIcon;
};