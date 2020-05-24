#include "FirstPersonController.h"
#include "core/scene/Camera.h"
#include "core/InputHandler.h"
#include "core/Engine.h"

FirstPersonController::FirstPersonController(double walkSpeed, double mouseSensitivity) {
	m_walkSpeed = walkSpeed;
	m_mouseSensitivity = mouseSensitivity;
	m_fallDistance = 0.0;
	m_flightEnabled = true;
	m_euler = dvec3(0.0);
	m_velocity = dvec3(0.0);
}

FirstPersonController::~FirstPersonController() {

}

void FirstPersonController::update(double dt, double partialTicks) {
	InputHandler* input = Engine::instance()->getInputHandler();

	m_prevTransform = Transformation(m_transform);

	dvec3 movement = dvec3(0.0);
	if (input->keyDown(SDL_SCANCODE_W)) movement.z++;
	if (input->keyDown(SDL_SCANCODE_S)) movement.z--;
	if (input->keyDown(SDL_SCANCODE_A)) movement.x++;
	if (input->keyDown(SDL_SCANCODE_D)) movement.x--;

	if (m_flightEnabled) {
		if (input->keyDown(SDL_SCANCODE_SPACE)) movement.y++;
		if (input->keyDown(SDL_SCANCODE_LSHIFT)) movement.y--;
	} else {
		if (input->keyPressed(SDL_SCANCODE_SPACE)) { /* jump */  }
		if (input->keyPressed(SDL_SCANCODE_LSHIFT)) { /* crouch */ }
	}
	if (input->keyPressed(SDL_SCANCODE_ESCAPE)) {
		input->toggleMouseGrabbed();
	}
	if (input->isMouseGrabbed()) {
		dvec2 dv = input->getRelativeMouseState();// dvec2(input->getMousePixelCoord()) - Engine::instance()->getWindowCenter();
		m_euler.x -= dv.x * m_mouseSensitivity * 0.001; // TODO: make this constant something more meaningful... maybe degrees per pixel
		m_euler.y += dv.y * m_mouseSensitivity * 0.001;
		if (dot(dv, dv) > 1e-10) {
			if (m_euler.x > +M_PI) m_euler.x -= M_PI * 2.0;
			if (m_euler.x < -M_PI) m_euler.x += M_PI * 2.0;
			if (m_euler.y > +M_PI * 0.5) m_euler.y = +M_PI * 0.5;
			if (m_euler.y < -M_PI * 0.5) m_euler.y = -M_PI * 0.5;
			//m_euler.x = glm::clamp(m_euler.x, -M_PI * 0.5, +M_PI * 0.5);
			//while (m_euler.y >= +M_PI) m_euler.y -= M_PI;
			//while (m_euler.y <= -M_PI) m_euler.y += M_PI;

			//info("%f, %f\n", m_euler.x * 180.0 / M_PI, m_euler.y * 180.0 / M_PI);

			dquat yaw = normalize(angleAxis(m_euler.x, dvec3(0.0, 1.0, 0.0)));
			dquat pitch = normalize(angleAxis(m_euler.y, dvec3(
				1.0 - 2.0 * ((yaw.y * yaw.y) + (yaw.z * yaw.z)),
				2.0 * ((yaw.x * yaw.y) + (yaw.w * yaw.z)),
				2.0 * ((yaw.x * yaw.z) - (yaw.w * yaw.y))
			)));
			m_transform.setAxisVectors(mat3_cast(pitch * yaw));
		}
	}

	double acceleration = 0.07; // seconds to start upto full speed
	double motionSq = dot(movement, movement);
	if (motionSq > 0.0) {
		movement /= sqrt(motionSq);
		m_velocity += movement * (dt / acceleration);
	} else {
		dvec3 v0 = m_velocity;
		m_velocity -= m_velocity * (dt / acceleration);
		if (dot(m_velocity, v0) < 0.0) {
			m_velocity = vec3(0.0);
		}
	}

	double speed = dot(m_velocity, m_velocity);
	if (speed > 0.0) {
		speed = sqrt(speed);
		//double retrograde = -(dot(movement, m_velocity / speed) - 1.0) * 0.5;
		//m_velocity += movement * (dt / acceleration) * retrograde; // faster acceleration / extra kick when moving at a sudden different direction

		if (speed > m_walkSpeed)
			m_velocity = (m_velocity / speed) * m_walkSpeed;

		//info("%f %f %f - %f\n", m_velocity.x, m_velocity.y, m_velocity.z, speed);
	} else {
		m_velocity = dvec3(0.0);
	}

	dmat3 axis = m_transform.getAxisVectors();
	//splitAxis[1] = dvec3(0.0, 1.0, 0.0); // keep x and z
	m_transform.translate(m_velocity * dt, axis);

	//Transformation t0 = m_prevTransform;
	//Transformation t1 = m_transform; // (m_transform - m_prevTransform) + m_prevTransform;
	//Transformation t2 = m_transform - m_prevTransform; // (m_transform - m_prevTransform) + m_prevTransform;
	//info("\n"
	//	"[%+.3f %+.3f %+.3f %+.3f] - [%+.3f %+.3f %+.3f %+.3f] = [%+.3f %+.3f %+.3f %+.3f]\n"
	//	"[%+.3f %+.3f %+.3f %+.3f] - [%+.3f %+.3f %+.3f %+.3f] = [%+.3f %+.3f %+.3f %+.3f]\n"
	//	"[%+.3f %+.3f %+.3f %+.3f] - [%+.3f %+.3f %+.3f %+.3f] = [%+.3f %+.3f %+.3f %+.3f]\n"
	//	"[%+.3f %+.3f %+.3f %+.3f] - [%+.3f %+.3f %+.3f %+.3f] = [%+.3f %+.3f %+.3f %+.3f]\n",
	//	t0.getModelMatrix()[0][0], t0.getModelMatrix()[1][0], t0.getModelMatrix()[2][0], t0.getModelMatrix()[3][0], t1.getModelMatrix()[0][0], t1.getModelMatrix()[1][0], t1.getModelMatrix()[2][0], t1.getModelMatrix()[3][0], t2.getModelMatrix()[0][0], t2.getModelMatrix()[1][0], t2.getModelMatrix()[2][0], t2.getModelMatrix()[3][0],
	//	t0.getModelMatrix()[0][1], t0.getModelMatrix()[1][1], t0.getModelMatrix()[2][1], t0.getModelMatrix()[3][1], t1.getModelMatrix()[0][1], t1.getModelMatrix()[1][1], t1.getModelMatrix()[2][1], t1.getModelMatrix()[3][1], t2.getModelMatrix()[0][1], t2.getModelMatrix()[1][1], t2.getModelMatrix()[2][1], t2.getModelMatrix()[3][1],
	//	t0.getModelMatrix()[0][2], t0.getModelMatrix()[1][2], t0.getModelMatrix()[2][2], t0.getModelMatrix()[3][2], t1.getModelMatrix()[0][2], t1.getModelMatrix()[1][2], t1.getModelMatrix()[2][2], t1.getModelMatrix()[3][2], t2.getModelMatrix()[0][2], t2.getModelMatrix()[1][2], t2.getModelMatrix()[2][2], t2.getModelMatrix()[3][2],
	//	t0.getModelMatrix()[0][3], t0.getModelMatrix()[1][3], t0.getModelMatrix()[2][3], t0.getModelMatrix()[3][3], t1.getModelMatrix()[0][3], t1.getModelMatrix()[1][3], t1.getModelMatrix()[2][3], t1.getModelMatrix()[3][3], t2.getModelMatrix()[0][3], t2.getModelMatrix()[1][3], t2.getModelMatrix()[2][3], t2.getModelMatrix()[3][3]
	//);
}

void FirstPersonController::applyCamera(Camera* camera) {
	camera->transform() = Transformation(m_transform);
}

double FirstPersonController::getWalkSpeed() const {
	return m_walkSpeed;
}

void FirstPersonController::setWalkSpeed(double walkSpeed) {
	m_walkSpeed = walkSpeed;
}

double FirstPersonController::getMouseSensitivity() const {
	return m_mouseSensitivity;
}

void FirstPersonController::setMouseSensitivity(double mouseSensitivity) {
	m_mouseSensitivity = mouseSensitivity;
}

bool FirstPersonController::isFlightEnabled() const {
	return m_flightEnabled;
}

void FirstPersonController::setFlightEnabled(bool flightEnabled) {
	m_flightEnabled = flightEnabled;
}

double FirstPersonController::getFallDistance() const {
	return m_fallDistance;
}

Transformation& FirstPersonController::transform() {
	return m_transform;
}

Transformation FirstPersonController::getDeltaTransform() const {
	return m_transform - m_prevTransform;
}
