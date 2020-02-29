#include "core/scene/Transformation.h"

Transformation::Transformation(dmat4 matrix) {
	this->setFromModelMatrix(matrix);
}

Transformation::Transformation(dvec3 translation, dquat orientation, dvec3 scale) {
	this->setTranslation(translation);
	this->setOrientation(orientation);
	this->setScale(scale);
}

Transformation::~Transformation() {}

dvec3 Transformation::getTranslation() const {
	return dvec3(m_matrix[3]);
}

Transformation* Transformation::setTranslation(dvec3 translation) {
	return this->setTranslation(translation.x, translation.y, translation.z);
}

Transformation* Transformation::setTranslation(double x, double y, double z) {
	m_matrix[3].x = x;
	m_matrix[3].y = y;
	m_matrix[3].z = z;

	m_changed = true;
	return this;
}

Transformation* Transformation::translate(dvec3 translation, dmat3 axis) {
	return this->translate(translation.x, translation.y, translation.z, axis);
}

Transformation* Transformation::translate(double x, double y, double z, dmat3 axis) {
	m_matrix[3].x += axis[0].x * x + axis[1].x * y + axis[2].x * z;
	m_matrix[3].y += axis[0].y * x + axis[1].y * y + axis[2].y * z;
	m_matrix[3].z += axis[0].z * x + axis[1].z * y + axis[2].z * z;

	m_changed = true;
	return this;
}

dquat Transformation::getOrientation() const {
	return quat_cast(dmat3(m_matrix));
}

Transformation* Transformation::setOrientation(dquat orientation, bool normalized) {
	if (!normalized) {
		this->setAxisVectors(mat3_cast(normalize(orientation)), true);
	} else {
		this->setAxisVectors(mat3_cast(orientation), true);
	}
	return this;
}

Transformation* Transformation::rotate(dquat orientation, bool normalized) {
	if (!normalized) {
		this->setOrientation(normalize(orientation) * this->getOrientation(), true);
	} else {
		this->setOrientation(orientation * this->getOrientation(), true);
	}
	return this;
}

Transformation* Transformation::rotate(dvec3 axis, double angle, bool normalized) {
	if (!normalized) {
		this->rotate(angleAxis(angle, normalize(axis)), true);
	} else {
		this->rotate(angleAxis(angle, axis), true);
	}
	return this;
}

dmat3 Transformation::getAxisVectors() const {
	return dmat3(m_matrix);
}

dvec3 Transformation::getXAxis() const {
	return dvec3(m_matrix[0]);
}

dvec3 Transformation::getYAxis() const {
	return dvec3(m_matrix[1]);
}

dvec3 Transformation::getZAxis() const {
	return dvec3(m_matrix[2]);
}

Transformation* Transformation::setAxisVectors(dmat3 axis, bool normalized) {
	return this->setAxisVectors(axis[0], axis[1], axis[2], normalized);
}

Transformation* Transformation::setAxisVectors(dvec3 x, dvec3 y, dvec3 z, bool normalized) {
	if (!normalized) {
		m_matrix[0] = dvec4(normalize(x), m_matrix[0].w);
		m_matrix[1] = dvec4(normalize(y), m_matrix[1].w);
		m_matrix[2] = dvec4(normalize(z), m_matrix[2].w);
	} else {
		m_matrix[0] = dvec4(x, m_matrix[0].w);
		m_matrix[1] = dvec4(y, m_matrix[1].w);
		m_matrix[2] = dvec4(z, m_matrix[2].w);
	}

	m_changed = true;
	return this;
}

dvec3 Transformation::getEuler() const {
	return eulerAngles(quat_cast(this->getAxisVectors()));
}

double Transformation::getPitch() const {
	return pitch(quat_cast(dmat3(this->getAxisVectors())));
}

double Transformation::getYaw() const {
	return yaw(quat_cast(dmat3(this->getAxisVectors())));
}

double Transformation::getRoll() const {
	return roll(quat_cast(dmat3(this->getAxisVectors())));
}

void Transformation::setEuler(dvec3 eulerAngles) {
	this->setAxisVectors(mat3_cast(dquat(mod(eulerAngles, dvec3(M_PI)))));
}

void Transformation::setEuler(double pitch, double yaw, double roll) {
	this->setEuler(dvec3(pitch, yaw, roll));
}

void Transformation::setPitch(double pitch) {
	dquat q = quat_cast(this->getAxisVectors());
	this->setEuler(dvec3(pitch, yaw(q), roll(q)));
}

void Transformation::setYaw(double yaw) {
	dquat q = quat_cast(this->getAxisVectors());
	this->setEuler(dvec3(pitch(q), yaw, roll(q)));
}

void Transformation::setRoll(double roll) {
	dquat q = quat_cast(this->getAxisVectors());
	this->setEuler(dvec3(pitch(q), yaw(q), roll));
}

void Transformation::rotate(double dpitch, double dyaw, double droll) {
	dquat q = quat_cast(this->getAxisVectors());
	this->setEuler(dvec3(pitch(q) + dpitch, yaw(q) + dyaw, roll(q) + droll));
}

dvec3 Transformation::getScale() const {
	return dvec3(m_matrix[0].w, m_matrix[1].w, m_matrix[2].w);
}

Transformation* Transformation::setScale(dvec3 scale) {
	return this->setScale(scale.x, scale.y, scale.z);
}

Transformation* Transformation::setScale(double scale) {
	return this->setScale(scale, scale, scale);
}

Transformation* Transformation::setScale(double x, double y, double z) {
	m_matrix[0].w = x;
	m_matrix[1].w = y;
	m_matrix[2].w = z;

	m_changed = true;
	return this;
}

Transformation* Transformation::scale(dvec3 scale) {
	return this->scale(scale.x, scale.y, scale.z);
}

Transformation* Transformation::scale(double scale) {
	return this->scale(scale, scale, scale);
}

Transformation* Transformation::scale(double x, double y, double z) {
	m_matrix[0].w *= x;
	m_matrix[1].w *= y;
	m_matrix[2].w *= z;

	m_changed = true;
	return this;
}

void Transformation::lookAt(dvec3 eye, dvec3 center, dvec3 up) {
	dvec3 z = normalize(center - eye);
	dvec3 x = normalize(cross(up, z));
	dvec3 y = cross(z, x);
	this->setTranslation(eye);
	this->setAxisVectors(x, y, z);
}

Transformation* Transformation::setFromModelMatrix(dmat4 matrix, bool normalized) {
	m_matrix = matrix;

	double sx = length(dvec3(matrix[0]));
	double sy = length(dvec3(matrix[1]));
	double sz = length(dvec3(matrix[2]));
	double isx = 1.0 / sx;
	double isy = 1.0 / sy;
	double isz = 1.0 / sz;

	// store normalized X/Y/Z splitAxis plus splitAxis lengths (scale) in each columns W component
	m_matrix[0] = dvec4(m_matrix[0].x * isx, m_matrix[0].y * isx, m_matrix[0].z * isx, sx);
	m_matrix[1] = dvec4(m_matrix[1].x * isy, m_matrix[1].y * isy, m_matrix[1].z * isy, sy);
	m_matrix[2] = dvec4(m_matrix[2].x * isz, m_matrix[2].y * isz, m_matrix[2].z * isz, sz);

	m_changed = true;
	return this;
}

dmat4 Transformation::getModelMatrix() const {
	dmat4 m = m_matrix;
	// scale normalized splitAxis vectors by intended scale lengths
	m[0] = dvec4(m[0].x * m[0].w, m[0].y * m[0].w, m[0].z * m[0].w, 0.0);
	m[1] = dvec4(m[1].x * m[1].w, m[1].y * m[1].w, m[1].z * m[1].w, 0.0);
	m[2] = dvec4(m[2].x * m[2].w, m[2].y * m[2].w, m[2].z * m[2].w, 0.0);
	m[3] = dvec4(m[3].x, m[3].y, m[3].z, 1.0);
	return m;
}

Transformation Transformation::operator*(const Transformation& t) const {
	return Transformation(t.getModelMatrix() * this->getModelMatrix());
}

Transformation& Transformation::operator*=(const Transformation& t) {
	*this = *this * t;
	return *this;
}

Transformation Transformation::operator-(const Transformation& t) const {
	dvec3 dtranslation = this->getTranslation() - t.getTranslation(); // translation from t to this
	dquat dorientation = this->getOrientation() * inverse(t.getOrientation()); // rotation from t to this
	dvec3 dscale = this->getScale() / t.getScale(); // scale from t to this - potentially problematic if zero scale.

	return Transformation(dtranslation, dorientation, dscale);
}

Transformation& Transformation::operator-=(const Transformation& t) {
	*this = *this - t;
	return *this;
}

Transformation Transformation::operator+(const Transformation& t) const {
	dvec3 dtranslation = this->getTranslation() + t.getTranslation();
	dquat dorientation = this->getOrientation() * t.getOrientation();
	dvec3 dscale = this->getScale() * t.getScale();

	return Transformation(dtranslation, dorientation, dscale);
}

Transformation& Transformation::operator+=(const Transformation& t) {
	*this = *this + t;
	return *this;
}

bool Transformation::operator==(const Transformation& other) const {
	return this->equals(other);
}

bool Transformation::operator!=(const Transformation& other) const {
	return !this->equals(other);
}

bool Transformation::equals(const Transformation& other, double epsilon) const {
	// check if the matrix represented by this transformation equals that of the other.
	// Checks if each individual raw matrix value is equal within the specified epsilon.
	// Checks the position column first, since that is more likely to differ first.
#if 0
	if (m_matrix == other.m_matrix) return true;
#endif
	if (abs(m_matrix[3][0] - other.m_matrix[3][0]) > epsilon) return false;
	if (abs(m_matrix[3][1] - other.m_matrix[3][1]) > epsilon) return false;
	if (abs(m_matrix[3][2] - other.m_matrix[3][2]) > epsilon) return false;
	if (abs(m_matrix[3][3] - other.m_matrix[3][3]) > epsilon) return false;
	if (abs(m_matrix[0][0] - other.m_matrix[0][0]) > epsilon) return false;
	if (abs(m_matrix[0][1] - other.m_matrix[0][1]) > epsilon) return false;
	if (abs(m_matrix[0][2] - other.m_matrix[0][2]) > epsilon) return false;
	if (abs(m_matrix[0][3] - other.m_matrix[0][3]) > epsilon) return false;
	if (abs(m_matrix[1][0] - other.m_matrix[1][0]) > epsilon) return false;
	if (abs(m_matrix[1][1] - other.m_matrix[1][1]) > epsilon) return false;
	if (abs(m_matrix[1][2] - other.m_matrix[1][2]) > epsilon) return false;
	if (abs(m_matrix[1][3] - other.m_matrix[1][3]) > epsilon) return false;
	if (abs(m_matrix[2][0] - other.m_matrix[2][0]) > epsilon) return false;
	if (abs(m_matrix[2][1] - other.m_matrix[2][1]) > epsilon) return false;
	if (abs(m_matrix[2][2] - other.m_matrix[2][2]) > epsilon) return false;
	if (abs(m_matrix[2][3] - other.m_matrix[2][3]) > epsilon) return false;
	return true;
}

bool Transformation::didChange() const {
	return m_changed;
}

void Transformation::setChanged(bool changed) {
	m_changed = changed;
}
