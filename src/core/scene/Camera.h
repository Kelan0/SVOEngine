#pragma once
#include "core/pch.h"
#include "core/scene/Transformation.h"

class ShaderProgram;

class Camera {
public:
	Camera(double fov = M_PI/2.0, double aspect = 4.0/3.0, double near = 0.1, double far = 1000.0);

	virtual void render(double dt, double partialTicks);

	virtual void applyUniforms(ShaderProgram* shaderProgram);

	virtual double getFieldOfView() const;

	virtual double getAspect() const;

	virtual double getNearPlane() const;

	virtual double getFarPlane() const;

	virtual void setFieldOfView(double fov);

	virtual void setAspect(double aspect);

	virtual void setNearPlane(double nearPlane);

	virtual void setFarPlane(double farPlane);

	Transformation& transform();

	dmat4 getProjectionMatrix() const;

	dmat4 getViewMatrix() const;

	dmat4 getViewProjectionMatrix() const;

	dmat4 getInverseViewProjectionMatrix() const;

	dmat4 getPrevViewProjectionMatrix() const;

	dmat4x3 getScreenRays() const;

	dvec3 getScreenRay(dvec2 screenCoord) const;

	dvec3 getPixelRay(ivec2 pixelCoord) const;

protected:
	Transformation m_transform;
	double m_fov;
	double m_aspect;
	double m_near;
	double m_far;

	bool m_projectionChanged;
	bool m_cameraMoved;

	dmat4 m_projectionMatrix;
	dmat4 m_viewMatrix;
	dmat4 m_viewProjectionMatrix;
	dmat4 m_invViewProjectionMatrix;

	dmat4 m_prevViewProjectionMatrix; // Used for frame reprojection

	dmat4x3 m_screenRays;
};

class CubemapCamera : public Camera {
public:
	CubemapCamera(double near = 0.1, double far = 1000.0);

	void render(double dt, double partialTicks) override;

	void applyUniforms(ShaderProgram* shaderProgram) override;

	void setFieldOfView(double fov) override;

	void setAspect(double aspect) override;

	dmat4 getViewMatrix(uint32_t index) const;

	dmat4 getViewProjectionMatrix(uint32_t index) const;

	dmat4 getInverseViewProjectionMatrix(uint32_t index) const;
private:
	dmat4 m_viewMatrixDirections[6]; // view matrix for each cubemap face
	dmat4 m_viewProjectionMatrixDirections[6]; // view-projection matrix for each cubemap face
	dmat4 m_invViewProjectionMatrixDirections[6]; // inverse view-projection matrix for each cubemap face
};
