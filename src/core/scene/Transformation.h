#pragma once

#include "core/pch.h"

class Transformation {
public:
	Transformation(dmat4 matrix = dmat4(1.0));

	Transformation(dvec3 translation, dquat orientation = dquat(), dvec3 scale = dvec3(1.0));

	~Transformation();

	dvec3 getTranslation() const;

	Transformation* setTranslation(dvec3 translation);

	Transformation* setTranslation(double x, double y, double z);

	Transformation* translate(dvec3 translation, dmat3 axis = dmat3(1.0));

	Transformation* translate(double x, double y, double z, dmat3 axis = dmat3(1.0));

	dquat getOrientation() const;

	Transformation* setOrientation(dquat orientation, bool normalized = false);

	Transformation* rotate(dquat orientation, bool normalized = false);

	Transformation* rotate(dvec3 axis, double angle, bool normalized = false);

	dmat3 getAxisVectors() const;

	dvec3 getXAxis() const;
	
	dvec3 getYAxis() const;

	dvec3 getZAxis() const;

	Transformation* setAxisVectors(dmat3 axis = dmat3(1.0), bool normalized = false);

	Transformation* setAxisVectors(dvec3 x, dvec3 y, dvec3 z, bool normalized = false);

	dvec3 getEuler() const;

	double getPitch() const;

	double getYaw() const;

	double getRoll() const;

	void setEuler(dvec3 eulerAngles);

	void setEuler(double pitch, double yaw, double roll);

	void setPitch(double pitch);

	void setYaw(double yaw);

	void setRoll(double roll);

	void rotate(double dpitch, double dyaw, double droll);

	dvec3 getScale() const;

	Transformation* setScale(dvec3 scale);

	Transformation* setScale(double scale);

	Transformation* setScale(double x, double y, double z);

	Transformation* scale(dvec3 scale);

	Transformation* scale(double scale);

	Transformation* scale(double x, double y, double z);

	void lookAt(dvec3 eye, dvec3 center, dvec3 up = dvec3(0.0, 1.0, 0.0));

	Transformation* setFromModelMatrix(dmat4 matrix, bool normalized = false);

	dmat4 getModelMatrix() const;

	Transformation operator*(const Transformation& t) const;

	Transformation& operator*=(const Transformation& t);

	Transformation operator-(const Transformation& t) const;

	Transformation& operator-=(const Transformation& t);

	Transformation operator+(const Transformation& t) const;

	Transformation& operator+=(const Transformation& t);

	bool operator==(const Transformation& other) const;

	bool operator!=(const Transformation& other) const;

	bool equals(const Transformation& other, double epsilon = 1e-6) const;

	bool didChange() const;

	void setChanged(bool changed);
private:
	dmat4 m_matrix;
	bool m_changed;
};