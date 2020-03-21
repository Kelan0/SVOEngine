#include "Camera.h"
#include "core/Engine.h"
#include "core/InputHandler.h"
#include "core/scene/Scene.h"
#include "core/scene/SceneComponents.h"
#include "core/renderer/ShaderProgram.h"

Camera::Camera(double fov, double aspect, double near, double far) {
	m_fov = fov;
	m_aspect = aspect;
	m_near = near;
	m_far = far;
	m_projectionChanged = true;
	m_cameraMoved = true;
	m_viewMatrix = dmat4(1.0);
	m_projectionMatrix = dmat4(1.0);
	m_viewProjectionMatrix = dmat4(1.0);
	m_invViewProjectionMatrix = dmat4(1.0);
	m_prevViewProjectionMatrix = dmat4(1.0);
	m_screenRays = dmat4x3(0.0);
}

void Camera::preRender(double dt, double partialTicks) {
	m_prevTransform = Transformation(m_transform);
	m_prevScreenRays = dmat4x3(m_screenRays);
	m_prevViewProjectionMatrix = dmat4(m_viewProjectionMatrix);
}

void Camera::render(double dt, double partialTicks) {
	dvec3 position = m_transform.getTranslation();
	dmat3 axis = m_transform.getAxisVectors();

	dmat4 prevViewMatrix = m_viewMatrix;

	if (m_transform.didChange() || m_projectionChanged) {
		if (m_transform.didChange()) {
			m_transform.setChanged(false);
			axis[0] = normalize(axis[0]);
			axis[1] = normalize(axis[1]);
			axis[2] = normalize(axis[2]);
			m_viewMatrix = glm::lookAt(position, position + axis[2], axis[1]);
		}

		if (m_projectionChanged) {
			m_projectionChanged = false;
			m_projectionMatrix = perspective(m_fov, m_aspect, m_near, m_far);
		}

		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
		m_invViewProjectionMatrix = inverse(m_viewProjectionMatrix);

		//mat4x4 projectedRays = m_invViewProjectionMatrix * dmat4(-1, 1, 0, 1, 1, 1, 0, 1, 1, -1, 0, 1, -1, -1, 0, 1);
		mat4x4 projectedRays = m_invViewProjectionMatrix * dmat4(
			-1, -1, 0, 1, 
			+1, -1, 0, 1, 
			+1, +1, 0, 1, 
			-1, +1, 0, 1
		);
		m_screenRays = mat4x3(projectedRays / projectedRays[3][3]);
		m_screenRays -= position;

		double maxDeltaViewComponent = 0.0;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				maxDeltaViewComponent = max(maxDeltaViewComponent, abs(m_viewProjectionMatrix[i][j] - m_prevViewProjectionMatrix[i][j]));
			}
		}

		m_cameraMoved = maxDeltaViewComponent > 1e-5;
	}

	if (Engine::inputHandler()->mousePressed(SDL_BUTTON_LEFT)) {
		dvec2 mouseScreenCoord = Engine::inputHandler()->getMouseScreenCoord();
		dvec3 rayOrigin = position;
		dvec3 rayDirection = this->getScreenRay(mouseScreenCoord);

		uint64_t t0 = Engine::instance()->getCurrentTime();
		RaycastResult* result = Engine::scene()->raycast(rayOrigin, rayDirection);
		uint64_t t1 = Engine::instance()->getCurrentTime();

		info("Performed raycast at [%f %f], O[%f %f %f] D[%f %f %f], Facing [%f %f %f], D.F %f, D.U %f, took %f msec\n", mouseScreenCoord.x, mouseScreenCoord.y, rayOrigin.x, rayOrigin.y, rayOrigin.z, rayDirection.x, rayDirection.y, rayDirection.z, axis[2].x, axis[2].y, axis[2].z, dot(rayDirection, axis[2]), dot(rayDirection, axis[1]), (t1 - t0) / 1000000.0);

		if (result != NULL) {
			info("RAY HIT %s\n", result->name.c_str());

			if (Engine::scene()->getSelectedMesh() != NULL && result->mesh == Engine::scene()->getSelectedMesh()->mesh)
				result = NULL;
		}

		Engine::scene()->setSelectedMesh(result);

	}
}

void Camera::applyUniforms(ShaderProgram* shaderProgram) {
	shaderProgram->setUniform("cameraRays", m_screenRays);
	shaderProgram->setUniform("prevCameraRays", m_prevScreenRays);
	shaderProgram->setUniform("viewMatrix", m_viewMatrix);
	shaderProgram->setUniform("invViewMatrix", inverse(m_viewMatrix));
	shaderProgram->setUniform("projectionMatrix", m_projectionMatrix);
	shaderProgram->setUniform("invProjectionMatrix", inverse(m_projectionMatrix));
	shaderProgram->setUniform("viewProjectionMatrix", m_viewProjectionMatrix);
	shaderProgram->setUniform("invViewProjectionMatrix", m_invViewProjectionMatrix);
	shaderProgram->setUniform("prevViewProjectionMatrix", m_prevViewProjectionMatrix);
	shaderProgram->setUniform("cameraPosition", m_transform.getTranslation());
	shaderProgram->setUniform("prevCameraPosition", m_prevTransform.getTranslation());
	shaderProgram->setUniform("nearPlane", (float) m_near);
	shaderProgram->setUniform("farPlane", (float) m_far);
	shaderProgram->setUniform("cameraMoved", m_cameraMoved);
}

double Camera::getFieldOfView() const {
	return m_fov;
}

double Camera::getAspect() const {
	return m_aspect;
}

double Camera::getNearPlane() const {
	return m_near;
}

double Camera::getFarPlane() const {
	return m_far;
}

void Camera::setFieldOfView(double fov) {
	m_fov = fov;
	m_projectionChanged = true;
}

void Camera::setAspect(double aspect) {
	m_aspect = aspect;
	m_projectionChanged = true;
}

void Camera::setNearPlane(double nearPlane) {
	m_near = nearPlane;
	m_projectionChanged = true;
}

void Camera::setFarPlane(double farPlane) {
	m_far = farPlane;
	m_projectionChanged = true;
}

Transformation& Camera::transform() {
	return m_transform;
}

dmat4 Camera::getProjectionMatrix() const {
	return m_projectionMatrix;
}

dmat4 Camera::getViewMatrix() const {
	return m_viewMatrix;
}

dmat4 Camera::getViewProjectionMatrix() const {
	return m_viewProjectionMatrix;
}

dmat4 Camera::getInverseViewProjectionMatrix() const {
	return m_invViewProjectionMatrix;
}

dmat4 Camera::getPrevViewProjectionMatrix() const {
	return m_prevViewProjectionMatrix;
}

dmat4x3 Camera::getScreenRays() const {
	return m_screenRays;
}

dvec3 Camera::getScreenRay(dvec2 screenCoord) const {
	vec3 v00 = m_screenRays[0];
	vec3 v10 = m_screenRays[1];
	vec3 v11 = m_screenRays[2];
	vec3 v01 = m_screenRays[3];
	vec3 vy0 = mix(v00, v10, screenCoord.x);
	vec3 vy1 = mix(v01, v11, screenCoord.x);
	vec3 vxy = mix(vy1, vy0, screenCoord.y);
	return normalize(vxy);
}

dvec3 Camera::getPixelRay(ivec2 pixelCoord) const {
	return this->getScreenRay(dvec2(pixelCoord) / dvec2(Engine::instance()->getWindowSize()));
}

CubemapCamera::CubemapCamera(double near, double far):
	Camera(M_PI / 2.0, 1.0, near, far) { // 90deg fov, 1:1 aspect
}

void CubemapCamera::render(double dt, double partialTicks) {
	dvec3 position = m_transform.getTranslation();
	dmat3 axis = m_transform.getAxisVectors();

	m_prevViewProjectionMatrix = m_viewProjectionMatrix;

	// CameraDirection gCameraDirections[NUM_OF_LAYERS] =
	// {
	// 	{ GL_TEXTURE_CUBE_MAP_POSITIVE_X, Vector3f(1.0f, 0.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f) },
	// 	{ GL_TEXTURE_CUBE_MAP_NEGATIVE_X, Vector3f(-1.0f, 0.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f) },
	// 	{ GL_TEXTURE_CUBE_MAP_POSITIVE_Y, Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, 0.0f, -1.0f) },
	// 	{ GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, 0.0f, 1.0f) },
	// 	{ GL_TEXTURE_CUBE_MAP_POSITIVE_Z, Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, -1.0f, 0.0f) },
	// 	{ GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, -1.0f, 0.0f) }
	// };


	if (m_transform.didChange() || m_projectionChanged) {
		if (m_transform.didChange()) {
			m_transform.setChanged(false);
			axis[0] = normalize(axis[0]);
			axis[1] = normalize(axis[1]);
			axis[2] = normalize(axis[2]);
			m_viewMatrixDirections[0] = glm::lookAt(position, position + axis[0], -axis[1]);
			m_viewMatrixDirections[1] = glm::lookAt(position, position - axis[0], -axis[1]);
			m_viewMatrixDirections[2] = glm::lookAt(position, position + axis[1], +axis[2]);
			m_viewMatrixDirections[3] = glm::lookAt(position, position - axis[1], -axis[2]);
			m_viewMatrixDirections[4] = glm::lookAt(position, position + axis[2], -axis[1]);
			m_viewMatrixDirections[5] = glm::lookAt(position, position - axis[2], -axis[1]);
			m_viewMatrix = m_viewMatrixDirections[5];
		}

		if (m_projectionChanged) {
			m_projectionChanged = false;
			m_projectionMatrix = perspective(m_fov, m_aspect, m_near, m_far);
		}

		for (int i = 0; i < 6; i++) {
			m_viewProjectionMatrixDirections[i] = m_projectionMatrix * m_viewMatrixDirections[i];
			m_invViewProjectionMatrixDirections[i] = inverse(m_viewProjectionMatrixDirections[i]);
		}

		m_viewProjectionMatrix = m_viewProjectionMatrixDirections[5];
		m_invViewProjectionMatrix = m_invViewProjectionMatrixDirections[5];

		//mat4x4 projectedRays = m_invViewProjectionMatrix * dmat4(-1, 1, 0, 1, 1, 1, 0, 1, 1, -1, 0, 1, -1, -1, 0, 1);
		mat4x4 projectedRays = m_invViewProjectionMatrix * dmat4(
			-1, -1, 0, 1,
			+1, -1, 0, 1,
			+1, +1, 0, 1,
			-1, +1, 0, 1);
		m_screenRays = mat4x3(projectedRays / projectedRays[3][3]);
		m_screenRays -= position;
	}
}

void CubemapCamera::applyUniforms(ShaderProgram* shaderProgram) {
	Camera::applyUniforms(shaderProgram);
	shaderProgram->setUniform("renderCubemap", true);
	for (int i = 0; i < 6; i++) {
		shaderProgram->setUniform("viewMatrixDirections[" + std::to_string(i) + "]", m_viewMatrixDirections[i]);
		shaderProgram->setUniform("viewProjectionMatrixDirections[" + std::to_string(i) + "]", m_viewProjectionMatrixDirections[i]);
		shaderProgram->setUniform("invViewProjectionMatrixDirections[" + std::to_string(i) + "]", m_invViewProjectionMatrixDirections[i]);
	}
}

void CubemapCamera::setFieldOfView(double fov) {
	error("Cannot set FOV of cubemap camera\n");
}

void CubemapCamera::setAspect(double aspect) {
	error("Cannot set aspect ratio of cubemap camera\n");
}

dmat4 CubemapCamera::getViewMatrix(uint32_t index) const {
	return m_viewMatrixDirections[index];
}

dmat4 CubemapCamera::getViewProjectionMatrix(uint32_t index) const {
	return m_viewProjectionMatrixDirections[index];
}

dmat4 CubemapCamera::getInverseViewProjectionMatrix(uint32_t index) const {
	return m_invViewProjectionMatrixDirections[index];
}
