#pragma once

#include "core/pch.h"

class ShaderProgram;
class Texture;

class RaytraceRenderer {
public:
	RaytraceRenderer(uint32_t renderWidth, uint32_t renderHeight);

	~RaytraceRenderer();

	void render(double dt, double partialTicks);

	void applyUniforms(ShaderProgram* program);

	uint32_t getRenderWidth() const;

	uint32_t getRenderHeight() const;

	void setRenderResolution(uint32_t width, uint32_t height);

	void bindTexture(uint32_t unit);
private:
	uint32_t m_renderWidth;
	uint32_t m_renderHeight;

	Texture* m_texture;

	ShaderProgram* m_raytraceShader;
};

