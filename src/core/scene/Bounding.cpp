#include "core/scene/Bounding.h"


AxisAlignedBB::AxisAlignedBB() :
	m_min(+INFINITY), m_max(-INFINITY) { // initialize to infinity, this box can be extended
}

AxisAlignedBB::AxisAlignedBB(dvec3 min, dvec3 max) : // Bounding box from min and max points
	m_min(min), m_max(max) {
	this->validate();
}

AxisAlignedBB::AxisAlignedBB(double xMin, double yMin, double zMin, double xMax, double yMax, double zMax) : // bounding box from min and max points
	m_min(xMin, yMin, zMin), m_max(xMax, yMax, zMax) {
	this->validate();
}

AxisAlignedBB::AxisAlignedBB(std::vector<dvec3>& pointCloud, dmat4 transform) { // minimum enclosing bounding box for a cloud of points.
	if (pointCloud.empty()) {
		throw std::invalid_argument("Cannot construct bounding box from empty point cloud");
	}

	AxisAlignedBB::minMaxPointCloud(pointCloud, transform, m_min, m_max);
}

AxisAlignedBB::AxisAlignedBB(std::vector<dvec3>& pointCloud) {
	if (pointCloud.empty()) {
		throw std::invalid_argument("Cannot construct bounding box from empty point cloud");
	}

	AxisAlignedBB::minMaxPointCloud(pointCloud, m_min, m_max);
}

AxisAlignedBB AxisAlignedBB::transformed(dmat4 transform) {
	// Return a new AABB enclosing the 8 vertices of this with a transform applied.
	std::vector<dvec3> boxCorners = this->getBoxCorners();
	return AxisAlignedBB(boxCorners, transform);
}

AxisAlignedBB AxisAlignedBB::extend(AxisAlignedBB& other) const {
	AxisAlignedBB extended;
	extended.m_min.x = min(m_min.x, other.m_min.x);
	extended.m_min.y = min(m_min.y, other.m_min.y);
	extended.m_min.z = min(m_min.z, other.m_min.z);
	extended.m_max.x = max(m_max.x, other.m_max.x);
	extended.m_max.y = max(m_max.y, other.m_max.y);
	extended.m_max.z = max(m_max.z, other.m_max.z);
	return extended;
}

AxisAlignedBB AxisAlignedBB::extend(AxisAlignedBB& other, int axis) const {
	AxisAlignedBB extended;
	extended.m_min = dvec3(m_min);
	extended.m_max = dvec3(m_max);
	extended.m_min[axis] = min(extended.m_min[axis], other.m_min[axis]);
	extended.m_max[axis] = max(extended.m_max[axis], other.m_max[axis]);
	return extended;
}

AxisAlignedBB AxisAlignedBB::extend(dvec3 point) const {
	AxisAlignedBB extended;
	extended.m_min.x = min(m_min.x, point.x);
	extended.m_min.y = min(m_min.y, point.y);
	extended.m_min.z = min(m_min.z, point.z);
	extended.m_max.x = max(m_max.x, point.x);
	extended.m_max.y = max(m_max.y, point.y);
	extended.m_max.z = max(m_max.z, point.z);
	return extended;
}

AxisAlignedBB AxisAlignedBB::extend(dvec3 point, int axis) const {
	AxisAlignedBB extended;
	extended.m_min = dvec3(m_min);
	extended.m_max = dvec3(m_max);
	extended.m_min[axis] = min(extended.m_min[axis], point[axis]);
	extended.m_max[axis] = max(extended.m_max[axis], point[axis]);
	return extended;
}

AxisAlignedBB AxisAlignedBB::extend(std::vector<dvec3>& pointCloud) const {
	AxisAlignedBB extended;
	AxisAlignedBB::minMaxPointCloud(pointCloud, extended.m_min, extended.m_max);
	return extended;
}

std::vector<dvec3> AxisAlignedBB::getBoxCorners() const {
	return {
		dvec3(m_min.x, m_min.y, m_min.z),
		dvec3(m_min.x, m_min.y, m_max.z),
		dvec3(m_min.x, m_max.y, m_min.z),
		dvec3(m_min.x, m_max.y, m_max.z),
		dvec3(m_max.x, m_min.y, m_min.z),
		dvec3(m_max.x, m_min.y, m_max.z),
		dvec3(m_max.x, m_max.y, m_min.z),
		dvec3(m_max.x, m_max.y, m_max.z),
	};
}

dvec3 AxisAlignedBB::getMin() const {
	return m_min;
}

double AxisAlignedBB::getMin(int axis) const {
	return m_min[axis];
}

dvec3 AxisAlignedBB::getMax() const {
	return m_max;
}

double AxisAlignedBB::getMax(int axis) const {
	return m_max[axis];
}

dvec3 AxisAlignedBB::getFullExtent() const {
	return (m_max - m_min);
}

double AxisAlignedBB::getFullExtent(int axis) const {
	return (m_max[axis] - m_min[axis]);
}

dvec3 AxisAlignedBB::getHalfExtent() const {
	return (m_max - m_min) * 0.5;
}

double AxisAlignedBB::getHalfExtent(int axis) const {
	return (m_max[axis] - m_min[axis]) * 0.5;
}

dvec3 AxisAlignedBB::getCenter() const {
	return (m_max + m_min) * 0.5;
}

double AxisAlignedBB::getCenter(int axis) const {
	return (m_max[axis] + m_min[axis]) * 0.5;
}

dvec3 AxisAlignedBB::getUnitCoordinate(dvec3 worldCoord) const {
	return (worldCoord - m_min) / (m_max - m_min);
}

double AxisAlignedBB::getUnitCoordinate(dvec3 worldCoord, int axis) const {
	return (worldCoord[axis] - m_min[axis]) / (m_max[axis] - m_min[axis]);
}

dvec3 AxisAlignedBB::getWorldCoordinate(dvec3 unitCoord) const {
	return m_min + (m_max - m_min) * unitCoord;
}

double AxisAlignedBB::getWorldCoordinate(dvec3 unitCoord, int axis) const {
	return m_min[axis] + (m_max[axis] - m_min[axis]) * unitCoord[axis];
}

double AxisAlignedBB::getWorldCoordinate(double unitCoord, int axis) const {
	return m_min[axis] + (m_max[axis] - m_min[axis]) * unitCoord;
}

int AxisAlignedBB::getLargestAxis() const {
	dvec3 extent = this->getFullExtent();
	if (extent.x > extent.y && extent.x > extent.z) { // x axis
		return 0;
	} else if (extent.y > extent.z) { // y axis
		return 1;
	} else { // z axis
		return 2;
	}
}

int AxisAlignedBB::getSmallestAxis() const {
	dvec3 extent = this->getFullExtent();
	if (extent.x < extent.y && extent.x < extent.z) { // x axis
		return 0;
	} else if (extent.y < extent.z) { // y axis
		return 1;
	} else { // z axis
		return 2;
	}
}

double AxisAlignedBB::getVolume() const {
	dvec3 extent = this->getFullExtent();
	return extent.x * extent.y * extent.z;
}

double AxisAlignedBB::getSurfaceArea() const {
	dvec3 extent = this->getFullExtent();
	return 2.0 * (extent.x * extent.y + extent.y * extent.z + extent.z * extent.x); // use dot product?
}

bool AxisAlignedBB::isDegenerate(bool ignoreZeroVolume) const {
	const double volume = this->getVolume();
	if (!isnormal(volume)) // volume is either infinite or NaN in one or more dimensions
		return true;
	if (!ignoreZeroVolume && volume <= 0.0)
		return true;
	return false;
}

bool AxisAlignedBB::intersectsPoint(dvec3 point) {
	return point.x >= m_min.x && point.x < m_max.x &&
		point.y >= m_min.y && point.y < m_max.y &&
		point.z >= m_min.z && point.z < m_max.z;
}

bool AxisAlignedBB::intersectsRay(dvec3 origin, dvec3 direction, double* t0, double* t1) {
	dvec3 invDir = 1.0 / direction;

	int sx = invDir.x < 0;
	int sy = invDir.y < 0;
	int sz = invDir.z < 0;

	double tmin = (m_bounds[0 + sx].x - origin.x) * invDir.x;
	double tmax = (m_bounds[1 - sx].x - origin.x) * invDir.x;
	double tymin = (m_bounds[0 + sy].y - origin.y) * invDir.y;
	double tymax = (m_bounds[1 - sy].y - origin.y) * invDir.y;

	if ((tmin > tymax) || (tymin > tmax)) return false;
	tmin = std::max(tmin, tymin); //if (tymin > tmin) tmin = tymin;
	tmax = std::min(tmax, tymax); //if (tymax < tmax) tmax = tymax;

	double tzmin = (m_bounds[0 + sz].z - origin.z) * invDir.z;
	double tzmax = (m_bounds[1 - sz].z - origin.z) * invDir.z;

	if ((tmin > tzmax) || (tzmin > tmax)) return false;
	tmin = std::max(tmin, tzmin); //if (tzmin > tmin) tmin = tzmin; 
	tmax = std::min(tmax, tzmax); //if (tzmax < tmax) tmax = tzmax; 

	if (t0 != NULL)* t0 = tmin;
	if (t1 != NULL)* t1 = tmax;

	return tmin > 0.0; // intersection is in front of the ray.
}

bool AxisAlignedBB::intersectsSegment(dvec3 a, dvec3 b, double* t0, double* t1) {
	// TODO: quick checks at both ends, and for zero-length line
	dvec3 d = b - a;
	double l = dot(d, d);

	if (l < 1e-10) { // (effectively) zero length segment, treat as point
		if (!intersectsPoint(a)) {
			return false;
		}

		// whole segment intersects the box.
		if (t0 != NULL)* t0 = 0.0;
		if (t1 != NULL)* t1 = 1.0;
		return true;
	}

	d /= (l = sqrt(l));

	double rt0, rt1;
	if (!intersectsRay(a, d, &rt0, &rt1)) {
		return false;
	}

	if (rt0 >= l) { // nearest intersection is further away than the length of the line
		return false;
	}

	if (t0 != NULL)* t0 = rt0 / l;
	if (t1 != NULL)* t1 = std::min(rt1 / l, 1.0);

	return true;
}

AxisAlignedBB AxisAlignedBB::combine(AxisAlignedBB a, AxisAlignedBB b) {
	AxisAlignedBB bound;
	bound.m_min.x = min(a.getMin().x, b.getMin().x);
	bound.m_min.y = min(a.getMin().y, b.getMin().y);
	bound.m_min.z = min(a.getMin().z, b.getMin().z);
	bound.m_max.x = max(a.getMax().x, b.getMax().x);
	bound.m_max.y = max(a.getMax().y, b.getMax().y);
	bound.m_max.z = max(a.getMax().z, b.getMax().z);
	return bound;
}

AxisAlignedBB AxisAlignedBB::combine(AxisAlignedBB a, dvec3 b) {
	AxisAlignedBB bound;
	bound.m_min.x = min(a.getMin().x, b.x);
	bound.m_min.y = min(a.getMin().y, b.y);
	bound.m_min.z = min(a.getMin().z, b.z);
	bound.m_max.x = max(a.getMax().x, b.x);
	bound.m_max.y = max(a.getMax().y, b.y);
	bound.m_max.z = max(a.getMax().z, b.z);
	return bound;
}

AxisAlignedBB AxisAlignedBB::overlap(AxisAlignedBB a, AxisAlignedBB b) {
	AxisAlignedBB bound;
	bound.m_min.x = max(a.getMin().x, b.getMin().x);
	bound.m_min.y = max(a.getMin().y, b.getMin().y);
	bound.m_min.z = max(a.getMin().z, b.getMin().z);
	bound.m_max.x = min(a.getMax().x, b.getMax().x);
	bound.m_max.y = min(a.getMax().y, b.getMax().y);
	bound.m_max.z = min(a.getMax().z, b.getMax().z);
	return bound;
}

void AxisAlignedBB::minMaxPointCloud(std::vector<dvec3>& pointCloud, dvec3& outMin, dvec3& outMax) {
	outMin = dvec3(+INFINITY);
	outMax = dvec3(-INFINITY);

	for (int i = 0; i < pointCloud.size(); i++) {
		minMax(pointCloud[i], outMin, outMax);
	}
}

void AxisAlignedBB::minMaxPointCloud(std::vector<dvec3>& pointCloud, dmat4 transform, dvec3& outMin, dvec3& outMax) {
	outMin = dvec3(+INFINITY);
	outMax = dvec3(-INFINITY);

	for (int i = 0; i < pointCloud.size(); i++) {
		dvec4 transformedPoint = transform * dvec4(pointCloud[i], 1.0);
		minMax(transformedPoint, outMin, outMax);
	}
}

void AxisAlignedBB::minMax(dvec3 point, dvec3& outMin, dvec3& outMax) {
	outMin.x = min(outMin.x, point.x);
	outMin.y = min(outMin.y, point.y);
	outMin.z = min(outMin.z, point.z);
	outMax.x = max(outMax.x, point.x);
	outMax.y = max(outMax.y, point.y);
	outMax.z = max(outMax.z, point.z);
}

void AxisAlignedBB::validate() {
	if (m_min.x > m_max.x) std::swap(m_min.x, m_max.x);
	if (m_min.y > m_max.y) std::swap(m_min.y, m_max.y);
	if (m_min.z > m_max.z) std::swap(m_min.z, m_max.z);
}