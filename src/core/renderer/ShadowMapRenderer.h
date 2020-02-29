#pragma once
#include "core/pch.h"

class PointLight;
class ShaderProgram;
class Camera;
class CubemapCamera;
class CubeMap;
class Framebuffer;

class ShadowMap {
public:
	ShadowMap(uint32_t width, uint32_t height);

	~ShadowMap();

	void setSize(uint32_t width, uint32_t height);

	void renderShadow(PointLight* light, double dt, double partialTicks);

	bool bindShadowTexture(uint32_t unit) const;

	uint32_t getWidth() const;

	uint32_t getHeight() const;
	
	static ShaderProgram* shadowMapShader();

private:
	uint32_t m_width;
	uint32_t m_height;

	CubeMap* m_shadowTexture;
	CubemapCamera* m_shadowCamera;
	Framebuffer* m_shadowFramebuffer;

	static ShaderProgram* s_shadowMapShader;
};

// class ShadowHandler {
// public:
// 	ShadowHandler();
// 
// 	~ShadowHandler();
// 
// private:
// 	ShadowMap* m_shadowMap;
// };