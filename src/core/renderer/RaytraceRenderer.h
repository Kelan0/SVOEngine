#pragma once

#include "core/pch.h"

class ShaderProgram;
class Texture2D;

class RaytraceRenderer {
public:
	RaytraceRenderer(uint32_t renderWidth, uint32_t renderHeight);

	~RaytraceRenderer();

	void render(double dt, double partialTicks);

	void applyUniforms(ShaderProgram* program);

	uint32_t getRenderWidth() const;

	uint32_t getRenderHeight() const;

	void setRenderResolution(uint32_t width, uint32_t height);

	void bindFrameTexture(uint32_t unit);

	Texture2D* getFrameTexture();
private:
	uint32_t m_frequencyScale;
	uint32_t m_renderWidth;
	uint32_t m_renderHeight;

	Texture2D* m_lowResolutionFrameTexture;
	Texture2D* m_FullResolutionFrameTexture;

	ShaderProgram* m_raytraceShader;
};

