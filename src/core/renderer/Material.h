#pragma once
#include "core/pch.h"
#include "core/renderer/Texture.h"
#include "core/ResourceHandler.h"

class ShaderProgram;

struct MaterialConfiguration {
	Resource<Texture2D> albedoMap; // The texture map defining the albedo (RGB colour before lighting) for every point on the surface this material is applied to.
	Resource<Texture2D> normalMap; // The texture map defining the normal (tangent-space XYZ unit vector) for every point on the surface this material is applied to.
	Resource<Texture2D> displacementMap; // The texture map defining the displacement (apparent offset from the surface) for every point on the surface this material is applied to.
	Resource<Texture2D> roughnessMap; // The texture map defining the roughness (distribution of microscopic surface normals) for every point on the surface this material is applied to.
	Resource<Texture2D> metalnessMap; // The texture map defining the metalness (percentage of non-dielectric material) for every point on the surface this material is applied to.
	Resource<Texture2D> ambientOcclusionMap;
	Resource<Texture2D> alphaMap;

	std::string albedoMapPath = "";
	std::string normalMapPath = "";
	std::string displacementMapPath = "";
	std::string roughnessMapPath = "";
	std::string metalnessMapPath = "";
	std::string ambientOcclusionMapPath = "";
	std::string alphaMapPath = "";

	std::string materialResourcePath = "";

	dvec3 albedo = dvec3(1.0);
	dvec3 transmission = dvec3(0.0);
	dvec3 emission = dvec3(0.0);
	double roughness = 1.0;
	double metalness = 0.0;
	double displacementScale = 0.025; // meters
	bool roughnessInverted = false;

	TextureFilter minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR_PIXEL;
	TextureFilter magFilter = TextureFilter::LINEAR_PIXEL;
	double anisotropy = 16.0;

	MaterialConfiguration(const char* materialResourcePath = "") {
		if (materialResourcePath != NULL) {
			this->materialResourcePath = materialResourcePath;
		}
	}

	bool operator==(MaterialConfiguration& other) const {
		if (this->albedoMapPath != other.albedoMapPath) return false;
		if (this->normalMapPath != other.normalMapPath) return false;
		if (this->displacementMapPath != other.displacementMapPath) return false;
		if (this->roughnessMapPath != other.roughnessMapPath) return false;
		if (this->metalnessMapPath != other.metalnessMapPath) return false;
		if (this->ambientOcclusionMapPath != other.ambientOcclusionMapPath) return false;

		if (this->materialResourcePath != other.materialResourcePath) return false;

		if (any(epsilonNotEqual(this->albedo, other.albedo, 1e-5))) return false;
		if (epsilonNotEqual(this->roughness, other.roughness, 1e-5)) return false;
		if (epsilonNotEqual(this->metalness, other.metalness, 1e-5)) return false;
		if (epsilonNotEqual(this->displacementScale, other.displacementScale, 1e-5)) return false;
		if (this->roughnessInverted == other.roughnessInverted) return false;

		if (this->minFilter != other.minFilter) return false;
		if (this->magFilter != other.magFilter) return false;
		if (epsilonNotEqual(this->anisotropy, other.anisotropy, 1e-5)) return false;

		return true;
	}

	bool operator!=(MaterialConfiguration& other) const {
		return !(*this == other);
	}
};

class Material {
public:
	Material(MaterialConfiguration configuration);

	~Material();

	void bind(ShaderProgram* shaderProgram, std::string uniformName, int32_t textureUnit = 0) const;

	void unbind() const;

	void makeResident(bool resident);

	Resource<Texture2D> getAlbedoMap() const;

	Resource<Texture2D> getNormalMap() const;

	Resource<Texture2D> getRoughnessMap() const;

	Resource<Texture2D> getMetalnessMap() const;

	Resource<Texture2D> getAmbientOcclusionMap() const;

	Resource<Texture2D> getAlphaMap() const;

	dvec3 getAlbedo() const;

	dvec3 getTransmission() const;

	dvec3 getEmission() const;

	double getRoughness() const;

	double getMetalness() const;

	bool isRoughnessInverted() const;

	bool isTransparent() const;

	bool isEmissive() const;

	bool isDoubleSided() const;

	TextureFilter getMinificationFilter() const;

	TextureFilter getMagnificationFilter() const;

	double getAnisotropy() const;

	void setFilterMode(TextureFilter minFilter, TextureFilter magFilter, double anisotropy = 16.0);

private:
	//bool m_ownsAlbedoMap;
	//bool m_ownsNormalMap;
	//bool m_ownsRoughnessMap;
	//bool m_ownsMetalnessMap;
	//bool m_ownsAmbientOcclusionMap;
	Resource<Texture2D> m_albedoMap;
	Resource<Texture2D> m_normalMap;
	Resource<Texture2D> m_roughnessMap;
	Resource<Texture2D> m_metalnessMap;
	Resource<Texture2D> m_ambientOcclusionMap;
	Resource<Texture2D> m_alphaMap;
	dvec3 m_albedo = dvec3(1.0);
	dvec3 m_transmission = dvec3(0.0);
	dvec3 m_emission = dvec3(0.0);
	double m_roughness = 1.0;
	double m_metalness = 0.0;
	bool m_roughnessInverted = false; // true if higher roughness values represent gloss/specular/shininess
	bool m_transparent = false; // true if the material transmits light at any point on the surface.
	bool m_emissive = false; // true if the material emits light at any point on the surface.
	bool m_doubleSided = false; // True if the material surface should be rendered from both front and back sides.

	TextureFilter m_minFilter;
	TextureFilter m_magFilter;
	double m_anisotropy;
};

