#include "Lights.h"
#include "core/Engine.h"
#include "core/scene/Scene.h"
#include "core/scene/Camera.h"
#include "core/InputHandler.h"

Light::Light(dvec3 colour) :
	m_colour(colour) {
}

void Light::render(double dt, double partialTicks) {

}

void Light::applyUniforms(std::string uniformName, ShaderProgram* shaderProgram) const {
	assert(shaderProgram != NULL);
	shaderProgram->setUniform(uniformName + ".colour", fvec3(m_colour));
}

dvec3 Light::getColour() const {
	return m_colour;
}

void Light::setColour(dvec3 colour) {
	m_colour = colour;
}



AttenuableLight::AttenuableLight(dvec3 colour, double attenuationQuadratic, double attenuationLinear, double attenuationOffset):
	Light(colour),
	m_radius(-1.0),
	m_attenuationQuadratic(attenuationQuadratic),
	m_attenuationLinear(attenuationLinear),
	m_attenuationOffset(attenuationOffset) {
	this->setRadius(-1.0);
}

AttenuableLight::AttenuableLight(double attenuationQuadratic, double attenuationLinear, double attenuationOffset):
	Light(),
	m_radius(-1.0),
	m_attenuationQuadratic(attenuationQuadratic),
	m_attenuationLinear(attenuationLinear),
	m_attenuationOffset(attenuationOffset) {
	this->setRadius(-1.0);
}

void AttenuableLight::applyUniforms(std::string uniformName, ShaderProgram* shaderProgram) const {
	Light::applyUniforms(uniformName, shaderProgram);
	shaderProgram->setUniform(uniformName + ".attenuation", fvec3(m_attenuationQuadratic, m_attenuationLinear, m_attenuationOffset));
	shaderProgram->setUniform(uniformName + ".radius", float(m_radius));
}

double AttenuableLight::getRadius() const {
	return m_radius;
}

void AttenuableLight::setRadius(double radius) {
	if (radius > 0.0) {
		m_radius = radius;
	} else {
		m_radius = this->calculateAutomaticRadius();
		info("Calculated light radius for attenuation [%f %f %f] as %f\n", m_attenuationQuadratic, m_attenuationLinear, m_attenuationOffset, m_radius);
	}
}

double AttenuableLight::getAttenuationOffset() const {
	return m_attenuationOffset;
}

void AttenuableLight::setAttenuationOffset(double attenuationOffset) {
	m_attenuationOffset = attenuationOffset;
}

double AttenuableLight::getAttenuationLinear() const {
	return m_attenuationLinear;
}

void AttenuableLight::setAttenuationLinear(double attenuationLinear) {
	m_attenuationLinear = attenuationLinear;
}

double AttenuableLight::getAttenuationQuadratic() const {
	return m_attenuationQuadratic;
}

void AttenuableLight::setAttenuationQuadratic(double attenuationQuadratic) {
	m_attenuationQuadratic = attenuationQuadratic;
}

double AttenuableLight::calculateAutomaticRadius(double cutoff) const {
	cutoff = max(cutoff, 1e-6);

	// find where "(1 / (ax^2 + bx + c)) - cutoff" equals zero.
	// equivalently, find where "(ax^2 + bc + c) - (1 / cutoff)" equals zero

	double maxRadius = 1000000.0;
	double a = m_attenuationQuadratic; // 1
	double b = m_attenuationLinear; // -1.5
	double c = m_attenuationOffset - 1.0 / cutoff; // 1
	double discriminant = b * b - 4 * a * c;

	if (discriminant < 0.0) {
		return maxRadius;
	}

	discriminant = sqrt(discriminant);

	//double t0 = (-b - discriminant) / (2 * a);
	double t1 = (-b + discriminant) / (2 * a);
	return min(maxRadius, t1);
}



PointLight::PointLight(dvec3 position, dvec3 colour, double attenuationQuadratic, double attenuationLinear, double attenuationOffset, ShadowMap* shadowMap):
	AttenuableLight(colour, attenuationQuadratic, attenuationLinear, attenuationOffset),
	m_position(position), 
	m_shadowMap(shadowMap) {
}

PointLight::~PointLight() {
	delete m_shadowMap;
}

void PointLight::render(double dt, double partialTicks) {
	Camera* camera = Engine::scene()->getCamera();
	if (Engine::inputHandler()->keyDown(SDL_SCANCODE_F)) {
		m_position = camera->transform().getTranslation() + camera->transform().getZAxis() * 4.0;
	}

	if (m_shadowMap != NULL) {
		m_shadowMap->renderShadow(this, dt, partialTicks);
	}
}

void PointLight::applyUniforms(std::string uniformName, ShaderProgram* shaderProgram) const {
	AttenuableLight::applyUniforms(uniformName, shaderProgram);
	shaderProgram->setUniform(uniformName + ".position", fvec3(m_position));
	if (m_shadowMap != NULL) {
		m_shadowMap->bindShadowTexture(15);
		shaderProgram->setUniform(uniformName + ".shadowMapTexture", 15);
		shaderProgram->setUniform(uniformName + ".hasShadowMap", true);
	} else {
		shaderProgram->setUniform(uniformName + ".hasShadowMap", false);
	}
}

LightType PointLight::getLightType() const {
	return LightType::POINT_LIGHT;
}

bool PointLight::isOmnidirectional() const {
	return true;
}

dvec3 PointLight::getPosition() const {
	return m_position;
}

void PointLight::setPosition(dvec3 position) {
	m_position = position;
}

ShadowMap* PointLight::getShadowMap() {
	return m_shadowMap;
}

void PointLight::setShadowMap(ShadowMap* shadowMap) {
	m_shadowMap = shadowMap;
}



DirectionLight::DirectionLight(dvec3 direction, dvec3 colour):
	Light(colour),
	m_direction(normalize(direction)) {
}

void DirectionLight::applyUniforms(std::string uniformName, ShaderProgram* shaderProgram) const {
	Light::applyUniforms(uniformName, shaderProgram);
	shaderProgram->setUniform(uniformName + ".direction", fvec3(m_direction));
}

LightType DirectionLight::getLightType() const {
	return LightType::DIRECTION_LIGHT;
}

bool DirectionLight::isOmnidirectional() const {
	return false;
}

dvec3 DirectionLight::getDirection() const {
	return m_direction;
}

void DirectionLight::setDirection(dvec3 direction, bool normalized) {
	if (!normalized) {
		m_direction = normalize(direction);
	} else {
		m_direction = direction;
	}
}



SpotLight::SpotLight(dvec3 position, dvec3 direction, double innerRadius, double outerRadius, dvec3 colour, double attenuationQuadratic, double attenuationLinear, double attenuationOffset):
	PointLight(position, colour, attenuationQuadratic, attenuationLinear, attenuationOffset),
	m_direction(direction),
	m_innerRadius(innerRadius),
	m_outerRadius(outerRadius) {
}

void SpotLight::applyUniforms(std::string uniformName, ShaderProgram* shaderProgram) const {
	PointLight::applyUniforms(uniformName, shaderProgram);

	shaderProgram->setUniform(uniformName + ".direction", fvec3(m_direction));
	shaderProgram->setUniform(uniformName + ".innerRadius", float(m_innerRadius));
	shaderProgram->setUniform(uniformName + ".outerRadius", float(m_outerRadius));
}

LightType SpotLight::getLightType() const {
	return LightType::SPOT_LIGHT;
}

bool SpotLight::isOmnidirectional() const {
	return false;
}

dvec3 SpotLight::getDirection() const {
	return m_direction;
}

void SpotLight::setDirection(dvec3 direction, bool normalized) {
	if (!normalized) {
		m_direction = normalize(direction);
	} else {
		m_direction = direction;
	}
}

double SpotLight::getInnerRadius() const {
	return m_innerRadius;
}

void SpotLight::setInnerRadius(double innerRadius) {
	m_innerRadius = innerRadius;
}

double SpotLight::getOuterRadius() const {
	return m_outerRadius;
}

void SpotLight::setOuterRadius(double outerRadius) {
	m_outerRadius = outerRadius;
}
