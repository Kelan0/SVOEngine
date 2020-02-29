#pragma once

#include "core/pch.h"
#include "core/renderer/ShaderProgram.h"
#include "core/renderer/ShadowMapRenderer.h"

enum LightType {
	POINT_LIGHT,
	DIRECTION_LIGHT,
	SPOT_LIGHT,
	//AREA_LIGHT
};

class Light {
public:
	Light(dvec3 colour = dvec3(1.0));

	virtual ~Light() = default;

	virtual void render(double dt, double partialTicks);

	virtual void applyUniforms(std::string uniformName, ShaderProgram* shaderProgram) const;

	virtual LightType getLightType() const = 0;

	virtual bool isOmnidirectional() const = 0;

	dvec3 getColour() const;

	void setColour(dvec3 colour);
protected:
	dvec3 m_colour; // The RGB colour of the light
};

// A light that follows the attenuation function "f(x)=ax^2+bx+c"
// where 'x' is the distance from the light, a is the quadratic term,
// b is the linear term and c is the constant offset.
class AttenuableLight : public Light {
public:
	AttenuableLight(dvec3 colour = dvec3(1.0), double attenuationQuadratic = 1.0, double attenuationLinear = 0.0, double attenuationOffset = 1.0);

	AttenuableLight(double attenuationQuadratic = 1.0, double attenuationLinear = 0.0, double attenuationOffset = 1.0);

	virtual ~AttenuableLight() = default;

	virtual void applyUniforms(std::string uniformName, ShaderProgram* shaderProgram) const override;

	double getRadius() const;

	void setRadius(double radius); // Any negative value will cause the radius to be automatically calculated based on the attenuation parameters.

	double getAttenuationOffset() const;

	void setAttenuationOffset(double attenuationOffset);

	double getAttenuationLinear() const;

	void setAttenuationLinear(double attenuationLinear);

	double getAttenuationQuadratic() const;

	void setAttenuationQuadratic(double attenuationQuadratic);

	double calculateAutomaticRadius(double cutoff = 0.001) const;

protected:
	double m_radius; // The radius of the light. Fragments falling outside this radius are ignored. The radius should be chosen so that the attenuation sufficiently falls inside before being cut off.
	double m_attenuationOffset; // Constant attenuation offset. "f(x)=ax^2+bx+c" :- this represents the "c" value.
	double m_attenuationLinear; // Linear attenuation falloff. "f(x)=ax^2+bx+c" :- this represents the "b" value.
	double m_attenuationQuadratic; // Quadratic attenuation offset. "f(x)=ax^2+bx+c" :- this represents the "a" value.
};

class PointLight : public AttenuableLight {
public:
	PointLight(dvec3 position, dvec3 colour = dvec3(1.0), double attenuationQuadratic = 1.0, double attenuationLinear = 0.0, double attenuationOffset = 1.0, ShadowMap* shadowMap = NULL);
	
	~PointLight();

	virtual void render(double dt, double partialTicks) override;

	virtual void applyUniforms(std::string uniformName, ShaderProgram* shaderProgram) const override;

	virtual LightType getLightType() const override;

	virtual bool isOmnidirectional() const override;

	dvec3 getPosition() const;

	void setPosition(dvec3 position);

	ShadowMap* getShadowMap();

	void setShadowMap(ShadowMap* shadowMap);

protected:
	dvec3 m_position; // Position of the light in the world.
	ShadowMap* m_shadowMap;
};

class DirectionLight : public Light {
public:
	DirectionLight(dvec3 direction, dvec3 colour = dvec3(1.0));

	~DirectionLight() = default;

	virtual void applyUniforms(std::string uniformName, ShaderProgram* shaderProgram) const override;

	virtual LightType getLightType() const override;

	virtual bool isOmnidirectional() const override;

	dvec3 getDirection() const;

	void setDirection(dvec3 direction, bool normalized = false);

protected:
	dvec3 m_direction;
};

class SpotLight : public PointLight {
public:
	SpotLight(dvec3 position, dvec3 direction, double innerRadius = M_PI/8.0, double outerRadius = M_PI/6.0, dvec3 colour = dvec3(1.0), double attenuationQuadratic = 1.0, double attenuationLinear = 0.0, double attenuationOffset = 1.0);

	virtual void applyUniforms(std::string uniformName, ShaderProgram* shaderProgram) const override;

	virtual LightType getLightType() const override;

	virtual bool isOmnidirectional() const override;

	dvec3 getDirection() const;

	void setDirection(dvec3 direction, bool normalized = false);

	double getInnerRadius() const;

	void setInnerRadius(double innerRadius);

	double getOuterRadius() const;

	void setOuterRadius(double outerRadius);

protected:
	dvec3 m_direction;
	double m_innerRadius;
	double m_outerRadius;
};
