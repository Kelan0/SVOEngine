#pragma once

#include "core/pch.h"


class AxisAlignedBB {
public:
	AxisAlignedBB();

	AxisAlignedBB(dvec3 min, dvec3 max); // Bounding box from min and max points

	AxisAlignedBB(double xMin, double yMin, double zMin, double xMax, double yMax, double zMax); // bounding box from min and max points

	AxisAlignedBB(std::vector<dvec3>& pointCloud, dmat4 transform); // minimum enclosing bounding box for a cloud of points.

	AxisAlignedBB(std::vector<dvec3>& pointCloud); // minimum enclosing bounding box for a cloud of points.

	AxisAlignedBB transformed(dmat4 transform);

	AxisAlignedBB extend(AxisAlignedBB& other) const;

	AxisAlignedBB extend(AxisAlignedBB& other, int axis) const;

	AxisAlignedBB extend(dvec3 point) const;

	AxisAlignedBB extend(dvec3 point, int axis) const;

	AxisAlignedBB extend(std::vector<dvec3>& pointCloud) const;

	std::vector<dvec3> getBoxCorners() const;

	dvec3 getMin() const;

	double getMin(int axis) const;

	dvec3 getMax() const;

	double getMax(int axis) const;

	dvec3 getFullExtent() const;

	double getFullExtent(int axis) const;

	dvec3 getHalfExtent() const;

	double getHalfExtent(int axis) const;

	dvec3 getCenter() const;

	double getCenter(int axis) const;

	dvec3 getUnitCoordinate(dvec3 worldCoord) const;

	double getUnitCoordinate(dvec3 worldCoord, int axis) const;

	dvec3 getWorldCoordinate(dvec3 unitCoord) const;

	double getWorldCoordinate(dvec3 unitCoord, int axis) const;

	double getWorldCoordinate(double unitCoord, int axis) const;

	int getLargestAxis() const;

	int getSmallestAxis() const;

	double getVolume() const;

	double getSurfaceArea() const;

	bool isDegenerate(bool ignoreZeroVolume = false) const;

	bool intersectsPoint(dvec3 point);

	bool intersectsRay(dvec3 origin, dvec3 direction, double* t0 = NULL, double* t1 = NULL);

	bool intersectsSegment(dvec3 a, dvec3 b, double* t0 = NULL, double* t1 = NULL);

	static AxisAlignedBB combine(AxisAlignedBB a, AxisAlignedBB b);

	static AxisAlignedBB combine(AxisAlignedBB a, dvec3 b);

	static AxisAlignedBB overlap(AxisAlignedBB a, AxisAlignedBB b);

	static void minMaxPointCloud(std::vector<dvec3>& pointCloud, dvec3& outMin, dvec3& outMax);

	static void minMaxPointCloud(std::vector<dvec3>& pointCloud, dmat4 transform, dvec3& outMin, dvec3& outMax);

	static void minMax(dvec3 point, dvec3& outMin, dvec3& outMax);

private:

	void validate();

	union {
		struct {
			dvec3 m_min;
			dvec3 m_max;
		};
		dvec3 m_bounds[2];
	};
};
