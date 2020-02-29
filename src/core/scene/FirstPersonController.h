#pragma once
#include "core/pch.h"
#include "core/scene/Transformation.h"

class Camera;
class Transformation;

class FirstPersonController {
public:
	FirstPersonController(double walkSpeed = 3.0, double mouseSensitivity = 1.0);

	~FirstPersonController();

	void update(double dt, double partialTicks);

	void applyCamera(Camera* camera);

	double getWalkSpeed() const;

	void setWalkSpeed(double walkSpeed);

	double getMouseSensitivity() const;

	void setMouseSensitivity(double mouseSensitivity);

	bool isFlightEnabled() const;

	void setFlightEnabled(bool flightEnabled);

	double getFallDistance() const;

	Transformation& transform();

	Transformation getDeltaTransform() const;

private:
	double m_walkSpeed;
	double m_mouseSensitivity;
	double m_fallDistance;
	bool m_flightEnabled;

	Transformation m_transform;
	Transformation m_prevTransform;
	dvec3 m_euler;
	dvec3 m_velocity;
};

