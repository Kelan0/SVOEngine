#pragma once
#include "core/pch.h"
#include "core/renderer/CubeMap.h"

class ShaderProgram;

class LightProbe {
public:
	LightProbe(CubeMap* environmentMap);

	LightProbe(CubemapConfiguration environmentMapConfig);

	~LightProbe();

	bool setEnvironmentMap(CubeMap* environmentMap);

	bool setEnvironmentMap(CubemapConfiguration environmentMapConfig);

	LightProbe* calculateDiffuseIrradianceMap();

	LightProbe* calculateSpecularReflectionMap();

	CubeMap* getEnvironmentMap() const;

	CubeMap* getDiffuseIrradianceMap() const;

	CubeMap* getSpecularReflectionMap() const;

	bool bindEnvironmentMap(uint32_t textureUnit = 0) const;

	bool bindDiffuseIrradianceMap(uint32_t textureUnit = 0) const;

	bool bindSpecularReflectionMap(uint32_t textureUnit = 0) const;

	static Texture2D* getBRDFIntegrationMap();

private:
	CubeMap* m_environmentMap; // Direct environment map
	CubeMap* m_diffuseIrradianceMap; // Irradiance map, each pixel represents the total diffuse light received from that direction
	CubeMap* m_specularReflectionMap; // Environment map pre-filtered based on the reflection for various roughness values.

	static Texture2D* s_BRDFIntegrationMap;
	static ShaderProgram* s_diffuseIrradianceConvolutionShader;
	static ShaderProgram* s_specularIrradianceConvolutionShader;
	static ShaderProgram* s_specularRoughnessConvolutionShader;
	static ShaderProgram* s_BRDFIntegrationShader;
};

