#pragma once
#include "core/pch.h"
#include "core/Engine.h"
#include "core/util/FileUtils.h"
#include "core/renderer/ShaderProgram.h"
#include "core/renderer/Material.h"
//#include "core/renderer/geometry/MeshLoader.h"
//#include "core/renderer/geometry/TriAABBIntersectionTest.h"

//class ShaderProgram;
//
//class VertexTangentComputer : public NotCopyable {
//public:
//	static VertexTangentComputer* instance();
//
//	~VertexTangentComputer();
//
//	std::vector<dvec3> calculateVertexTangents(std::vector<dvec3> vertexPositions, std::vector<dvec2> vertexTextures, std::vector<uint32_t> vertexIndices);
//private:
//	VertexTangentComputer();
//
//	ShaderProgram* m_tangentComputeShader;
//};

template <typename V, typename I, qualifier Q>
class _Mesh {
public:
	typedef V value;
	typedef I index;
	typedef vec<3, V, Q> vec3;
	typedef vec<2, V, Q> vec2;

	static const int32_t VERTEX_SIZE = sizeof(value) * 11 + sizeof(uint32_t) * 1; // 11 values + 1 uint per vertex
	static const int32_t TRIANGLE_SIZE = sizeof(index) * 3; // 3 indices per triangle

	struct vertex {
		union {
			struct {
				union {
					vec3 position;
					struct { value px, py, pz; };
				};
				union {
					vec3 normal;
					struct { value nx, ny, nz; };
				};
				union {
					vec3 tangent;
					struct { value bx, by, bz; };
				};
				union {
					vec2 texture;
					struct { value tx, ty; };
				};
				int32_t material;
			};
			uint8_t bytes[VERTEX_SIZE];
		};

		bool equalsPosition(const vertex& other, double epsilon = 1e-6) const;
		
		bool equalsNormal(const vertex& other, double epsilon = 1e-6) const;

		bool equalsTangent(const vertex& other, double epsilon = 1e-6) const;

		bool equalsTexture(const vertex& other, double epsilon = 1e-6) const;

		bool equalsMaterial(const vertex& other, double epsilon = 1e-6) const;

		bool equals(const vertex& other, double epsilon = 1e-6) const;

		vertex operator*(const mat4& other) const;

		vertex& operator*=(const mat4& other);

		bool operator==(const vertex& other) const;

		bool operator!=(const vertex& other) const;
	};

	struct triangle {
		union {
			struct { index i0, i1, i2; };
			index indices[3];
			uint8_t bytes[TRIANGLE_SIZE];
		};

		bool hasIndex(index index) const;

		bool isDegenerate(_Mesh<V, I, Q>* mesh) const;

		bool isDegenerate(std::vector<vertex> vertices) const;

		vertex& getVertex(_Mesh<V, I, Q>* mesh, int32_t index) const;

		vertex& getVertex(std::vector<vertex>& vertices, int32_t index) const;

		vec3 getNormal(_Mesh<V, I, Q>* mesh) const;

		vec3 getNormal(std::vector<vertex> vertices) const;

		bool operator==(const triangle& other) const;

		bool operator!=(const triangle& other) const;
	};

	class Builder {
	public:
		Builder(int32_t reservedVertices = 0, int32_t reservedTriangles = 0);

		Builder(vertex* vertices, triangle* triangles, int32_t vertexCount, int32_t triangleCount);

		Builder(std::vector<vertex>& vertices, std::vector<triangle>& triangles);

		~Builder();

		bool build(_Mesh<V, I, Q>** meshPtr, bool uploadGL = false);
		
		void optimise(const double epsilon = 1e-6);

		//void removedEnclosedGeometry();

		//void computeBoundingSphere() const;

		//void computeAxisAlignedBoundingBox() const;

		//void computeOrientedBoundingBox() const;

		//void computeConvexHull() const;

		void createCuboid(dvec3 halfExtent = dvec3(1.0), dvec3 position = dvec3(0.0));

		void createUVSpheroid(dvec3 radius = dvec3(1.0), dvec3 position = dvec3(0.0), uint32_t uDivisions = 32, uint32_t vDivisions = 32);

		void createIcoSpheroid(dvec3 radius = dvec3(1.0), dvec3 position = dvec3(0.0), uint32_t isoDivisions = 4);

		bool parseOBJ(std::string file, int32_t expectedVertexCount = 1000);

		bool loadModel(std::string file);

		bool loadBinaryFile(std::string file);

		bool writeBinaryFile(std::string file);

		void calculateFaceNormals();

		void calculateVertexNormals();

		void calculateVertexTangents();

		int32_t getVertexCount() const;

		int32_t getTriangleCount() const;

		vertex getVertex(index idx);

		triangle getTriangle(index idx);
		
		void setVertices(std::vector<vertex>& vertices);

		void setTriangles(std::vector<triangle>& triangles);

		index addVertex(vertex vertex);
		
		index addVertex(vec3 position, vec3 normal = vec3(0), vec3 tangent = vec3(0), vec2 texture = vec2(0), int32_t material = -1);
		
		index addVertex(value px, value py, value pz, value nx = 0, value ny = 0, value nz = 0, value bx = 0, value by = 0, value bz = 0, value tx = 0, value ty = 0, int32_t material = -1);
		
		index addTriangle(triangle triangle);
		
		index addTriangle(index i0, index i1, index i2);
		
		index addTriangle(vertex v0, vertex v1, vertex v2);
		
		index addTriangle(vec3 p0, vec3 p1, vec3 p2, vec3 n0 = vec3(0), vec3 n1 = vec3(0), vec3 n2 = vec3(0), vec3 b0 = vec3(0), vec3 b1 = vec3(0), vec3 b2 = vec3(0), vec2 t0 = vec2(0), vec2 t1 = vec2(0), vec2 t2 = vec2(0), int32_t material = -1);
		
		index addTriangle(value px0, value py0, value pz0, value px1, value py1, value pz1, value px2, value py2, value pz2, value nx0 = 0, value ny0 = 0, value nz0 = 0, value nx1 = 0, value ny1 = 0, value nz1 = 0, value nx2 = 0, value ny2 = 0, value nz2 = 0,  value bx0 = 0, value by0 = 0, value bz0 = 0, value bx1 = 0, value by1 = 0, value bz1 = 0, value bx2 = 0, value by2 = 0, value bz2 = 0, value tx0 = 0, value ty0 = 0, value tx1 = 0, value ty1 = 0, value tx2 = 0, value ty2 = 0, int32_t material = -1);

		void removeVertex(index index); // This will invalidate indices returned from previous calls to addVertex

		void removeTriangle(index index); // This will invalidate indices returned from previous calls to addTriangle

		void clear();
	private:
		std::vector<vertex> m_vertices;
		std::vector<triangle> m_triangles;
		vec3 m_largestAxis[2];
	};

	class VoxelMesh {
	public:
		class Node {
		public:
			Node* m_children; // Pointer to first child
			uint8_t m_childMask; // Bitmask of existing children

			Node();

			~Node();
		private:

		};

		VoxelMesh(_Mesh<V, I, Q>* mesh, double voxelSize = 0.025);

		~VoxelMesh();
	private:
		void divide(Node* node, double boxExtent, dvec3 boxCenter, std::vector<triangle> triangles, std::vector<vertex>& vertices, double voxelSize);
	};


	//static_assert(value_type == V, "Mismatched mesh value type");

	_Mesh();

	_Mesh(std::vector<vertex>& vertices, std::vector<triangle>& triangles);

	_Mesh(vertex* vertices, triangle* triangles, uint32_t vertexCount, uint32_t triangleCount, uint32_t vertexOffset = 0, uint32_t triangleOffset = 0);

	~_Mesh();

	vertex getVertex(index index) const;

	triangle getTriangle(index index) const;

	uint32_t getVertexCount() const;

	uint32_t getTriangleCount() const;

	uint32_t getIndexCount() const;

	uint32_t getVertexBufferSize() const;

	uint32_t getIndexBufferSize() const;

	const std::vector<vertex>& getVertices() const;

	const std::vector<triangle>& getTriangles() const;

	bool fillVertexBuffer(void** vertexBufferPtr) const;

	bool fillIndexBuffer(void** indexBufferPtr) const;

	bool isRenderable() const;

	void allocateGPU();

	void deallocateGPU();

	void deallocateCPU();

	bool uploadGPU();

	bool downloadGPU();

	void draw(uint32_t offset = 0, uint32_t count = 0);

	bool getRayIntersection(dvec3 rayOrigin, dvec3 rayDirection, double& closestHitDistance, dvec3& closestHitBarycentric, index& closestHitTriangleIndex, bool anyHit = false) const;

	static void addVertexInputs(ShaderProgram* shaderProgram);

	static void enableVertexAttributes();
private:
	std::vector<triangle> m_triangles;
	std::vector<vertex> m_vertices;
	std::vector<Material> m_materials;

	uint32_t m_vertexArray;
	uint32_t m_vertexBuffer;
	uint32_t m_indexBuffer;
	uint32_t m_allocatedVertices;
	uint32_t m_allocatedIndices;
	bool m_renderable;
};

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::vertex::equalsPosition(const vertex& other, double epsilon) const {
	if (position == other.position) return true;

	dvec3 d = dvec3(position) - dvec3(other.position);
	return dot(d, d) < epsilon * epsilon;
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::vertex::equalsNormal(const vertex& other, double epsilon) const {
	if (normal == other.normal) return true;

	dvec3 d = dvec3(normal) - dvec3(other.normal);
	return dot(d, d) < epsilon * epsilon;
}

template<typename V, typename I, qualifier Q>
inline bool _Mesh<V, I, Q>::vertex::equalsTangent(const vertex& other, double epsilon) const {
	if (tangent == other.tangent) return true;

	dvec3 d = dvec3(tangent) - dvec3(other.tangent);
	return dot(d, d) < epsilon * epsilon;
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::vertex::equalsTexture(const vertex& other, double epsilon) const {
	if (texture == other.texture) return true;
	dvec2 d = dvec2(texture) - dvec2(other.texture);
	return abs(d.x) < epsilon && abs(d.y) < epsilon;
}

template<typename V, typename I, qualifier Q>
inline bool _Mesh<V, I, Q>::vertex::equalsMaterial(const vertex& other, double epsilon) const {
	return material == other.material;
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::vertex::equals(const vertex& other, double epsilon) const {
	return this->equalsPosition(other, epsilon) && this->equalsNormal(other, epsilon) && this->equalsTangent(other, epsilon) && this->equalsTexture(other, epsilon) && this->equalsMaterial(other, epsilon);
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::vertex _Mesh<V, I, Q>::vertex::operator*(const mat4& transform) const {
	dmat4 normalTransform = inverse(transpose(transform));
	vertex v;
	v.position = vec3(transform * vec4(position, 1));
	v.normal = normalize(normalTransform * vec4(normal, 0));
	v.tangent = normalize(normalTransform * vec4(tangent, 0));
	v.texture = vec2(texture);
	v.material = material;
	return v;
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::vertex& _Mesh<V, I, Q>::vertex::operator*=(const mat4& transform) {
	*this = *this * transform;
	return *this;
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::vertex::operator==(const vertex& other) const {
	return this->equals(other);
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::vertex::operator!=(const vertex& other) const {
	return !this->equals(other);
}


template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::triangle::hasIndex(index idx) const {
	return i0 == idx || i1 == idx || i2 == idx;
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::triangle::isDegenerate(_Mesh<V, I, Q>* mesh) const {
	return this->isDegenerate(mesh->m_vertices);
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::triangle::isDegenerate(std::vector<vertex> vertices) const {
	if (i0 == i1 || i1 == i2 || i2 == i0) {
		return true; // One or more identical indices, so triangle forms a line or a point.
	}

	vertex v0 = this->getVertex(vertices, 0);
	vertex v1 = this->getVertex(vertices, 1);
	vertex v2 = this->getVertex(vertices, 2);
	if (v0.equalsPosition(v1) || v1.equalsPosition(v2) || v2.equalsPosition(v0)) {
		return true; // Unique indices, but one or more referenced vertices are at the same position.
	}

	return false;
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::vertex& _Mesh<V, I, Q>::triangle::getVertex(_Mesh<V, I, Q>* mesh, int32_t index) const {
	return this->getVertex(mesh->m_vertices, index);
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::vertex& _Mesh<V, I, Q>::triangle::getVertex(std::vector<vertex>& vertices, int32_t index) const {
	assert(index >= 0 && index < 3);
	assert(vertices.size() > i0 && vertices.size() > i1 && vertices.size() > i2);

	return vertices[indices[index]];
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::vec3 _Mesh<V, I, Q>::triangle::getNormal(_Mesh<V, I, Q>* mesh) const {
	return this->getNormal(mesh->m_vertices);
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::vec3 _Mesh<V, I, Q>::triangle::getNormal(std::vector<vertex> vertices) const {
	// assert not degenerate ?
	vec3 v0 = this->getVertex(vertices, 0).position;
	vec3 v1 = this->getVertex(vertices, 1).position;
	vec3 v2 = this->getVertex(vertices, 2).position;

	return normalize(cross(v1 - v0, v2 - v0));
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::triangle::operator==(const triangle& other) const {
	return i0 == other.i0 && i1 == other.i1 && i2 == other.i2;
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::triangle::operator!=(const triangle& other) const {
	return !(*this == other);
}



template<typename V, typename I, qualifier Q>
_Mesh<V, I, Q>::Builder::Builder(int32_t reservedVertices, int32_t reservedTriangles) {
	m_vertices.reserve(reservedVertices);
	m_triangles.reserve(reservedTriangles);
}

template<typename V, typename I, qualifier Q>
_Mesh<V, I, Q>::Builder::Builder(vertex* vertices, triangle* triangles, int32_t vertexCount, int32_t triangleCount) {
	m_vertices.resize(vertexCount);
	m_triangles.resize(triangleCount);

	std::memcpy(&m_vertices[0], vertices, sizeof(vertex) * vertexCount);
	std::memcpy(&m_triangles[0], triangles, sizeof(triangle) * triangleCount);
}

template<typename V, typename I, qualifier Q>
_Mesh<V, I, Q>::Builder::Builder(std::vector<vertex>& vertices, std::vector<triangle>& triangles) {
	// no copy, modifications should be reflected in both vectors.
	m_vertices = vertices;
	m_triangles = triangles;
}

template<typename V, typename I, qualifier Q>
_Mesh<V, I, Q>::Builder::~Builder() {

}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::Builder::build(_Mesh<V, I, Q>** meshPtr, bool uploadGL) {
	if (meshPtr == NULL) {
		return false;
	}

	if (*meshPtr != NULL) {
		delete* meshPtr;
	}

	this->calculateVertexTangents();

	_Mesh<V, I, Q>* mesh = new _Mesh<V, I, Q>();
	mesh->m_vertices.resize(this->getVertexCount());
	mesh->m_triangles.resize(this->getTriangleCount());
	std::memcpy(&mesh->m_vertices[0], &m_vertices[0], this->getVertexCount() * sizeof(vertex));
	std::memcpy(&mesh->m_triangles[0], &m_triangles[0], this->getTriangleCount() * sizeof(triangle));
	
	if (uploadGL) {
		mesh->allocateGPU();
		mesh->uploadGPU();
	}

	*meshPtr = mesh;

	return true;
}

template<typename V, typename I, qualifier Q>
void _Mesh<V, I, Q>::Builder::optimise(const double epsilon) {

	std::vector<vertex> newVertices;
	std::vector<triangle> newTriangles;
	newVertices.reserve(this->getVertexCount());
	newTriangles.reserve(this->getTriangleCount());

	for (int i = 0; i < this->getTriangleCount(); i++) {
		triangle ot = this->getTriangle(i);
		// TODO: might be faster to remove degenerate triangles after removing duplicated vertices, since 
		// it would be more likely to only include reference comparasons in the triangle
		if (ot.isDegenerate(m_vertices)) {
			continue; // ignore this triangle.
		}

		vertex ov0 = ot.getVertex(m_vertices, 0);
		vertex ov1 = ot.getVertex(m_vertices, 1);
		vertex ov2 = ot.getVertex(m_vertices, 2);

		bool f0 = false;
		bool f1 = false;
		bool f2 = false;
		triangle nt;

		for (int j = 0; j < newVertices.size(); j++) {
			vertex nv = newVertices[j];

			if (ov0.equals(nv, epsilon)) nt.i0 = j, f0 = true;
			if (ov1.equals(nv, epsilon)) nt.i1 = j, f1 = true;
			if (ov2.equals(nv, epsilon)) nt.i2 = j, f2 = true;
		}

		if (!f0) nt.i0 = newVertices.size(), newVertices.push_back(ov0);
		if (!f1) nt.i1 = newVertices.size(), newVertices.push_back(ov1);
		if (!f2) nt.i2 = newVertices.size(), newVertices.push_back(ov2);

		newTriangles.push_back(nt);

		if (i % 100 == 0) {
			info("%d / %d triangles optimised\n", i, this->getTriangleCount());
		}
	}

	m_vertices.swap(newVertices);
	m_triangles.swap(newTriangles);
}

template<typename V, typename I, qualifier Q>
inline void _Mesh<V, I, Q>::Builder::createCuboid(dvec3 halfExtent, dvec3 position) {
	dvec3 v0 = position - halfExtent;
	dvec3 v1 = position + halfExtent;

	index i00 = this->addVertex(v0.x, v0.y, v0.z, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
	index i01 = this->addVertex(v0.x, v0.y, v1.z, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0);
	index i02 = this->addVertex(v0.x, v1.y, v1.z, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	index i03 = this->addVertex(v0.x, v1.y, v0.z, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	
	index i04 = this->addVertex(v1.x, v0.y, v0.z, +1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0);
	index i05 = this->addVertex(v1.x, v1.y, v0.z, +1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	index i06 = this->addVertex(v1.x, v1.y, v1.z, +1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	index i07 = this->addVertex(v1.x, v0.y, v1.z, +1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
	
	index i08 = this->addVertex(v0.x, v0.y, v0.z, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
	index i09 = this->addVertex(v1.x, v0.y, v0.z, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0);
	index i10 = this->addVertex(v1.x, v0.y, v1.z, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	index i11 = this->addVertex(v0.x, v0.y, v1.z, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	
	index i12 = this->addVertex(v0.x, v1.y, v0.z, 0.0, +1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	index i13 = this->addVertex(v0.x, v1.y, v1.z, 0.0, +1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
	index i14 = this->addVertex(v1.x, v1.y, v1.z, 0.0, +1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0);
	index i15 = this->addVertex(v1.x, v1.y, v0.z, 0.0, +1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	
	index i16 = this->addVertex(v0.x, v0.y, v0.z, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 1.0, 1.0);
	index i17 = this->addVertex(v0.x, v1.y, v0.z, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	index i18 = this->addVertex(v1.x, v1.y, v0.z, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	index i19 = this->addVertex(v1.x, v0.y, v0.z, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0);

	index i20 = this->addVertex(v0.x, v0.y, v1.z, 0.0, 0.0, +1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
	index i21 = this->addVertex(v1.x, v0.y, v1.z, 0.0, 0.0, +1.0, 0.0, 0.0, 0.0, 1.0, 1.0);
	index i22 = this->addVertex(v1.x, v1.y, v1.z, 0.0, 0.0, +1.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	index i23 = this->addVertex(v0.x, v1.y, v1.z, 0.0, 0.0, +1.0, 0.0, 0.0, 0.0, 0.0, 0.0);

	this->addTriangle(i00, i01, i02);
	this->addTriangle(i00, i02, i03);
	
	this->addTriangle(i04, i05, i06);
	this->addTriangle(i04, i06, i07);
	
	this->addTriangle(i08, i09, i10);
	this->addTriangle(i08, i10, i11);

	this->addTriangle(i12, i13, i14);
	this->addTriangle(i12, i14, i15);

	this->addTriangle(i16, i17, i18);
	this->addTriangle(i16, i18, i19);

	this->addTriangle(i20, i21, i22);
	this->addTriangle(i20, i22, i23);
}

template<typename V, typename I, qualifier Q>
inline void _Mesh<V, I, Q>::Builder::createUVSpheroid(dvec3 radius, dvec3 position, uint32_t uDivisions, uint32_t vDivisions) {
	uDivisions++;
	vDivisions++;
	double du = 1.0 / (double)(uDivisions - 1);
	double dv = 1.0 / (double)(vDivisions - 1);
	uint32_t c = 0;

	index* indices = new index[uDivisions * vDivisions];
	for (int u = 0; u < uDivisions; u++) {
		for (int v = 0; v < vDivisions; v++) {
			double x = cos(2.0 * glm::pi<double>() * u * du) * sin(glm::pi<double>() * v * dv);
			double y = sin(-glm::half_pi<double>() + glm::pi<double>() * v * dv);
			double z = sin(2.0 * glm::pi<double>() * u * du) * sin(glm::pi<double>() * v * dv);

			double px = x * radius.x;
			double py = y * radius.y;
			double pz = z * radius.z;
			double nx = x / (radius.x); // ?
			double ny = y / (radius.y);
			double nz = z / (radius.z);

			indices[c++] = this->addVertex(px, py, pz, nx, ny, nz, 0.0, 0.0, 0.0, 1.0 - u * du, 1.0 - v * dv);
		}
	}

	for (int u0 = 0, u1 = 1; u0 < uDivisions - 1; u0++, u1++) {
		for (int v0 = 0, v1 = 1; v0 < vDivisions - 1; v0++, v1++) {
			index i0 = indices[vDivisions * u0 + v0]; // 0
			index i1 = indices[vDivisions * u0 + v1]; // 3
			index i2 = indices[vDivisions * u1 + v1]; // 2
			index i3 = indices[vDivisions * u1 + v0]; // 1
			this->addTriangle(i0, i1, i2);
			this->addTriangle(i0, i2, i3);
		}
	}

	delete[] indices;
}

template<typename V, typename I, qualifier Q>
inline void _Mesh<V, I, Q>::Builder::createIcoSpheroid(dvec3 radius, dvec3 position, uint32_t isoDivisions) {

}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::Builder::parseOBJ(std::string objData, int32_t expectedVertexCount) {
	if (objData.empty()) {
		return false;
	}

	// save old data and swap back if error.

	std::stringstream file(objData);
	std::istringstream parser;

	std::string line;
	int32_t lineCount = 0;

	struct face {
		uvec3 v[3];
		int32_t i[3] { -1, -1, -1 };
	};

	struct vertexdata {
		std::vector<uint32_t> i; // list of face indices that reference this vertex
		//vertexdata(): i(0) {}
	};

	struct position : public vertexdata {
		vec3 v;
	};

	struct normal : public vertexdata {
		vec3 v;
	};

	struct texture : public vertexdata {
		vec2 v;
	};

	//typedef vec<3, uvec3, highp> face;
	std::vector<position> positions;
	std::vector<normal> normals;
	std::vector<texture> textures;
	std::vector<face> faces;
	positions.reserve(expectedVertexCount);
	normals.reserve(expectedVertexCount);
	textures.reserve(expectedVertexCount);
	faces.reserve(expectedVertexCount);

	while (std::getline(file, line, '\n')) {
		//info("%s\n", line.c_str());
		lineCount++;

		parser.clear();
		if (line.find("#", 0) == 0) { // line is a comment, ignore it.
			continue;
		} else if (line.find("v ", 0) == 0) { // line is a vertex position
			position p;
			parser.str(line.substr(2));
			parser >> p.v.x >> p.v.y >> p.v.z;
			positions.push_back(p);
		} else if (line.find("vn ", 0) == 0) { // line is a vertex normal
			normal n;
			parser.str(line.substr(3));
			parser >> n.v.x >> n.v.y >> n.v.z;
			normals.push_back(n);
		} else if (line.find("vt ", 0) == 0) { // line is a vertex texture
			texture t;
			parser.str(line.substr(3));
			parser >> t.v.x >> t.v.y;
			textures.push_back(t);
		} else if (line.find("f ", 0) == 0) { // line connects face indices
			parser.str(line.substr(2));
			face f;
			for (int i = 0; i < 3; i++) {
				parser >> f.v[i][0] >> std::ws; // position reference
				positions[f.v[i][0] - 1].i.push_back(faces.size());
				if (parser.peek() == '/') {
					parser.get();
					if (parser.peek() == '/') {
						parser.get();
						parser >> f.v[i][2] >> std::ws; // normal reference
						normals[f.v[i][2] - 1].i.push_back(faces.size());
					} else {
						parser >> f.v[i][1] >> std::ws; // texture reference
						textures[f.v[i][1] - 1].i.push_back(faces.size());
						if (parser.peek() == '/') {
							parser.get();
							parser >> f.v[i][2] >> std::ws; // normal reference
							normals[f.v[i][2] - 1].i.push_back(faces.size());
						}
					}
				}
			}
			if (f.v[0][0] == 0 || f.v[1][0] == 0 || f.v[2][0] == 0) { // No vertex reference was specified. Error
				error("Error parsing OBJ file (line %d) invalid face indices - ignoring\n", lineCount - 1, line.c_str());
				continue;
			}

			//info("%u/%u/%u %u/%u/%u %u/%u/%u\n", f[0][0], f[0][1], f[0][2], f[1][0], f[1][1], f[1][2], f[2][0], f[2][1], f[2][2]);
			faces.push_back(f);
		} else {
			//error("Error parsing OBJ file (line %d) \"%s\" - ignoring\n", lineCount - 1, line.c_str());
		}
	}

	info("%d positions, %d textures, %d normals\n", positions.size(), textures.size(), normals.size());
	//for (int i = 0; i < faces.size(); i++) {
	//	this->addTriangle(
	//		positions[faces[i][0][0] - 1], positions[faces[i][1][0] - 1], positions[faces[i][2][0] - 1],
	//		normals[faces[i][0][2] - 1], normals[faces[i][1][2] - 1], normals[faces[i][2][2] - 1],
	//		textures[faces[i][0][1] - 1], textures[faces[i][1][1] - 1], textures[faces[i][2][1] - 1]
	//	);
	//}

	uint32_t vertexCount = max(max(positions.size(), normals.size()), textures.size());
	uint32_t triangleCount = faces.size();

	std::vector<vertex> vertices(vertexCount);
	std::vector<triangle> triangles(triangleCount);
	//vertices.reserve(positions.size());
	//triangles.reserve(faces.size());

	int vertexPtr = 0;
	for (int i = 0; i < triangleCount; i++) {
		face& f = faces[i];
		triangle& t = triangles[i];

		for (int j = 0; j < 3; j++) {
			if (f.i[j] == -1) { // face vertex has not previously been indexed
				position& vp = positions[f.v[j][0] - 1];
				normal& vn = normals[f.v[j][2] - 1];
				texture& vt = textures[f.v[j][1] - 1];


				// go through all triangles sharing this vertex position
				for (int k = 0; k < vp.i.size(); k++) {
					face& f1 = faces[vp.i[k]];
					if (f1.v[0][0] == f.v[j][0]) { // If other triangle vertex 0 matches this vertex
						f1.i[0] = vertexPtr; // Record the vertex pointer into the other triangle vertex 0
					} else if (f1.v[1][0] == f.v[j][0]) { // If other triangle vertex 1 matches this vertex
						f1.i[1] = vertexPtr; // Record the vertex pointer into the other triangle vertex 1
					} else if (f1.v[2][0] == f.v[j][0]) { // If other triangle vertex 2 matches this vertex
						f1.i[2] = vertexPtr; // Record the vertex pointer into the other triangle vertex 2
					}
				}

				vertices[vertexPtr].position = vp.v;
				vertices[vertexPtr].normal = vn.v;
				vertices[vertexPtr].texture = vt.v;
				f.i[j] = vertexPtr++;
			}

			t.indices[j] = f.i[j];
		}
	}

	m_vertices.swap(vertices);
	m_triangles.swap(triangles);
	this->calculateVertexTangents();
	return true;
}

template<typename V, typename I, qualifier Q>
inline bool _Mesh<V, I, Q>::Builder::loadModel(std::string file) {
	info("Loading model \"%s\"\n", file.c_str());

	file = file.substr(0, file.find_last_of('.'));

	if (this->loadBinaryFile(file + ".bin")) {
		return true;
	}

	this->clear();
	std::string objData;
	if (FileUtils::loadFile(RESOURCE_PATH(file + ".obj").c_str(), objData)) {
		if (this->parseOBJ(objData)) {
			if (!this->writeBinaryFile(file + ".bin")) {
				warn("Loaded OBJ model but failed to write binary file\n");
			}
			return true;
		}
	}

	return false;
}

template<typename V, typename I, qualifier Q>
inline bool _Mesh<V, I, Q>::Builder::loadBinaryFile(std::string file) {
	std::ifstream stream(RESOURCE_PATH(file).c_str(), std::ifstream::in | std::ofstream::binary);
	if (!stream.is_open()) {
		return false;
	}

	uint32_t vertexCount, triangleCount;
	stream.read(reinterpret_cast<char*>(&vertexCount), sizeof(uint32_t));
	stream.read(reinterpret_cast<char*>(&triangleCount), sizeof(uint32_t));
	
	m_vertices.resize(vertexCount);
	m_triangles.resize(triangleCount);

	stream.read(reinterpret_cast<char*>(&m_vertices[0]), sizeof(vertex) * vertexCount);
	stream.read(reinterpret_cast<char*>(&m_triangles[0]), sizeof(triangle) * triangleCount);

	return true;
}

template<typename V, typename I, qualifier Q>
inline bool _Mesh<V, I, Q>::Builder::writeBinaryFile(std::string file) {
	info("Writing binary model file \"%s\" with %d vertices, %d triangles\n", file.c_str(), m_vertices.size(), m_triangles.size());;
	std::ofstream stream(RESOURCE_PATH(file).c_str(), std::ofstream::out | std::ofstream::binary);
	if (!stream.is_open()) {
		info("Failed to write binary file");
		return false;
	}

	uint32_t vertexCount = m_vertices.size();
	uint32_t triangleCount = m_triangles.size();
	stream.write(reinterpret_cast<char*>(&vertexCount), sizeof(uint32_t));
	stream.write(reinterpret_cast<char*>(&triangleCount), sizeof(uint32_t));
	stream.write(reinterpret_cast<char*>(&m_vertices[0]), sizeof(vertex) * vertexCount);
	stream.write(reinterpret_cast<char*>(&m_triangles[0]), sizeof(triangle) * triangleCount);

	stream.close();

	return true;
}

template<typename V, typename I, qualifier Q>
void _Mesh<V, I, Q>::Builder::calculateFaceNormals() {
	std::vector<vertex> newVertices;
	std::vector<triangle> newTriangles;
	newVertices.reserve(this->getVertexCount());
	newTriangles.reserve(this->getTriangleCount());

	for (int i = 0; i < this->getTriangleCount(); i++) {
		triangle tri = this->getTriangle(i);

		newTriangles.push_back(tri);

		// TODO unfinished
	}

	m_vertices.swap(newVertices);
	m_triangles.swap(newTriangles);
}

template<typename V, typename I, qualifier Q>
inline void _Mesh<V, I, Q>::Builder::calculateVertexNormals() {

}

template<typename V, typename I, qualifier Q>
void _Mesh<V, I, Q>::Builder::calculateVertexTangents() {
	for (int i = 0; i < m_triangles.size(); i++) {
		triangle tri = m_triangles[i];
		vertex& v0 = m_vertices[tri.i0];
		vertex& v1 = m_vertices[tri.i1];
		vertex& v2 = m_vertices[tri.i2];

		vec3 e0 = v1.position - v0.position;
		vec3 e1 = v2.position - v0.position;

		double du0 = v1.texture.x - v0.texture.x;
		double dv0 = v1.texture.y - v0.texture.y;
		double du1 = v2.texture.x - v0.texture.x;
		double dv1 = v2.texture.y - v0.texture.y;

		double f = 1.0 / (du0 * dv1 - du1 * dv0);

		vec3 tangent;

		tangent.x = f * (dv1 * e0.x - dv0 * e1.x);
		tangent.y = f * (dv1 * e0.y - dv0 * e1.y);
		tangent.z = f * (dv1 * e0.z - dv0 * e1.z);

		v0.tangent += tangent;
		v1.tangent += tangent;
		v2.tangent += tangent;
	}

	for (int i = 0; i < m_vertices.size(); i++) {
		m_vertices[i].tangent = normalize(m_vertices[i].tangent);
	}
}

template<typename V, typename I, qualifier Q>
int32_t _Mesh<V, I, Q>::Builder::getVertexCount() const {
	return m_vertices.size();
}

template<typename V, typename I, qualifier Q>
int32_t _Mesh<V, I, Q>::Builder::getTriangleCount() const {
	return m_triangles.size();
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::vertex _Mesh<V, I, Q>::Builder::getVertex(index idx) {
	assert(idx >= 0 && idx < m_vertices.size());
	return m_vertices[idx];
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::triangle _Mesh<V, I, Q>::Builder::getTriangle(index idx) {
	assert(idx >= 0 && idx < m_triangles.size());
	return m_triangles[idx];
}

template<typename V, typename I, qualifier Q>
inline void _Mesh<V, I, Q>::Builder::setVertices(std::vector<vertex>& vertices) {
	m_vertices = vertices;
}

template<typename V, typename I, qualifier Q>
inline void _Mesh<V, I, Q>::Builder::setTriangles(std::vector<triangle>& triangles) {
	m_triangles = triangles;
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::index _Mesh<V, I, Q>::Builder::addVertex(vertex vertex) {
	index i = index(m_vertices.size());
	m_vertices.push_back(vertex);
	return i;
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::index _Mesh<V, I, Q>::Builder::addVertex(vec3 position, vec3 normal, vec3 tangent, vec2 texture, int32_t material) {
	return this->addVertex(vertex { position, normal, tangent, texture, material });
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::index _Mesh<V, I, Q>::Builder::addVertex(value px, value py, value pz, value nx, value ny, value nz, value bx, value by, value bz, value tx, value ty, int32_t material) {
	return this->addVertex(vec3(px, py, pz), vec3(nx, ny, nz), vec3(bx, by, bz), vec2(tx, ty), material);
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::index _Mesh<V, I, Q>::Builder::addTriangle(triangle triangle) {
	assert(triangle.i0 >= 0 && triangle.i0 < m_vertices.size());
	assert(triangle.i1 >= 0 && triangle.i1 < m_vertices.size());
	assert(triangle.i2 >= 0 && triangle.i2 < m_vertices.size());

	index i = index(m_triangles.size());
	m_triangles.push_back(triangle);
	return i;
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::index _Mesh<V, I, Q>::Builder::addTriangle(index i0, index i1, index i2) {
	return this->addTriangle({ i0, i1, i2 });
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::index _Mesh<V, I, Q>::Builder::addTriangle(vertex v0, vertex v1, vertex v2) {
	index i0 = this->addVertex(v0);
	index i1 = this->addVertex(v1);
	index i2 = this->addVertex(v2);

	return this->addTriangle(i0, i1, i2);
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::index _Mesh<V, I, Q>::Builder::addTriangle(vec3 p0, vec3 p1, vec3 p2, vec3 n0, vec3 n1, vec3 n2, vec3 b0, vec3 b1, vec3 b2, vec2 t0, vec2 t1, vec2 t2, int32_t material) {
	return this->addTriangle(vertex { p0, n0, b0, t0 }, vertex { p1, n1, b1, t1 }, vertex { p2, n2, b2, t2 }, material);
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::index _Mesh<V, I, Q>::Builder::addTriangle(value px0, value py0, value pz0, value px1, value py1, value pz1, value px2, value py2, value pz2, value nx0, value ny0, value nz0, value nx1, value ny1, value nz1, value nx2, value ny2, value nz2, value bx0, value by0, value bz0, value bx1, value by1, value bz1, value bx2, value by2, value bz2, value tx0, value ty0, value tx1, value ty1, value tx2, value ty2, int32_t material) {
	return this->addTriangle(vec3(px0, py0, pz0), vec3(px1, py1, pz1), vec3(px2, py2, pz2), vec3(nx0, ny0, nz0), vec3(nx1, ny1, nz1), vec3(nx2, ny2, nz2), vec3(bx0, by0, bz0), vec3(bx1, by1, bz1), vec3(bx2, by2, bz2), vec2(tx0, ty0), vec2(tx1, ty1), vec2(tx2, ty2), material);
}

template<typename V, typename I, qualifier Q>
void _Mesh<V, I, Q>::Builder::removeVertex(index idx) {
	assert(idx >= 0 && idx < m_vertices.size());
	m_vertices.erase(m_vertices.begin() + idx);
	// All indices greater than the removed one need to be decremented by one.
	for (auto it = m_triangles.begin(); it != m_triangles.end(); it++) {
		if (it->hasIndex(idx)) { // The removed vertex was part of this triangle. Remove the triangle.
			it = m_triangles.erase(it);
		}

		if (it->i0 > idx) it->i0--;
		if (it->i1 > idx) it->i1--;
		if (it->i2 > idx) it->i2--;
	}
}

template<typename V, typename I, qualifier Q>
void _Mesh<V, I, Q>::Builder::removeTriangle(index index) {
	assert(index >= 0 && index < m_triangles.size());
	m_triangles.erase(m_triangles.begin() + index);
}

template<typename V, typename I, qualifier Q>
void _Mesh<V, I, Q>::Builder::clear() {
	m_triangles.clear();
	m_vertices.clear();
	m_largestAxis[0] = vec3(0);
	m_largestAxis[1] = vec3(0);
}





template<typename V, typename I, qualifier Q>
_Mesh<V, I, Q>::VoxelMesh::Node::Node() {

}

template<typename V, typename I, qualifier Q>
_Mesh<V, I, Q>::VoxelMesh::Node::~Node() {

}

template<typename V, typename I, qualifier Q>
_Mesh<V, I, Q>::VoxelMesh::VoxelMesh(_Mesh<V, I, Q>* mesh, double voxelSize) {
	Node* root = new Node();
	dvec3 meshCenter = dvec3(0.0);
	double meshMaxExtent = -INFINITY;
	double meshMinExtent = +INFINITY;

	for (int i = 0; i < mesh->getVertexCount(); i++) {
		dvec3 pos = dvec3(mesh->getVertex(index(i)).position);

		meshCenter += pos;
		meshMaxExtent = max(meshMaxExtent, pos.x);
		meshMaxExtent = max(meshMaxExtent, pos.y);
		meshMaxExtent = max(meshMaxExtent, pos.z);
		meshMinExtent = min(meshMinExtent, pos.x);
		meshMinExtent = min(meshMinExtent, pos.y);
		meshMinExtent = min(meshMinExtent, pos.z);
	}

	meshCenter /= mesh->getVertexCount();

	this->divide(root, meshCenter, meshMaxExtent - meshMinExtent, mesh->m_triangles, mesh->m_vertices, voxelSize);
}

template<typename V, typename I, qualifier Q>
_Mesh<V, I, Q>::VoxelMesh::~VoxelMesh() {

}

template<typename V, typename I, qualifier Q>
void _Mesh<V, I, Q>::VoxelMesh::divide(Node* node, double boxExtent, dvec3 boxCenter, std::vector<triangle> triangles, std::vector<vertex>& vertices, double voxelSize) {
	//for (int i = 0; i < triangles.size(); i++) {
	//	dvec3 v0 = dvec3(triangles[i].getVertex(vertices, 0).position);
	//	dvec3 v1 = dvec3(triangles[i].getVertex(vertices, 1).position);
	//	dvec3 v2 = dvec3(triangles[i].getVertex(vertices, 2).position);
	//
	//	float boxCenterArr[3] { boxCenter.x, boxCenter.y, boxCenter.z };
	//	float boxExtentArr[3] { boxExtent, boxExtent, boxExtent };
	//	float triangleVertsArr[3][3] {
	//		{ v0.x, v0.y, v0.z },
	//		{ v1.x, v1.y, v1.z },
	//		{ v2.x, v2.y, v2.z }
	//	};
	//
	//	bool intersectsBox = triBoxOverlap(boxCenterArr, boxExtentArr, triangleVertsArr);
	//
	//
	//}
}




template<typename V, typename I, qualifier Q>
_Mesh<V, I, Q>::_Mesh() {
}

template<typename V, typename I, qualifier Q>
inline _Mesh<V, I, Q>::_Mesh(std::vector<vertex>& vertices, std::vector<triangle>& triangles):
	m_vertices(vertices),
	m_triangles(triangles) {
}

template<typename V, typename I, qualifier Q>
inline _Mesh<V, I, Q>::_Mesh(vertex* vertices, triangle* triangles, uint32_t vertexCount, uint32_t triangleCount, uint32_t vertexOffset, uint32_t triangleOffset):
	m_vertices(vertices + vertexOffset, vertices + vertexOffset + vertexCount),
	m_triangles(triangles + triangleOffset, triangles + triangleOffset + triangleCount) {
}

template<typename V, typename I, qualifier Q>
_Mesh<V, I, Q>::~_Mesh() {
	this->deallocateCPU();
	this->deallocateGPU();
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::vertex _Mesh<V, I, Q>::getVertex(index index) const {
	assert(index >= 0 && index < m_vertices.size());
	return m_vertices[index];
}

template<typename V, typename I, qualifier Q>
typename _Mesh<V, I, Q>::triangle _Mesh<V, I, Q>::getTriangle(index index) const {
	assert(index >= 0 && index < m_triangles.size());
	return m_triangles[index];
}

template<typename V, typename I, qualifier Q>
uint32_t _Mesh<V, I, Q>::getVertexCount() const {
	return m_vertices.size();
}

template<typename V, typename I, qualifier Q>
uint32_t _Mesh<V, I, Q>::getTriangleCount() const {
	return m_triangles.size();
}

template<typename V, typename I, qualifier Q>
uint32_t _Mesh<V, I, Q>::getIndexCount() const {
	return m_triangles.size() * 3;
}

template<typename V, typename I, qualifier Q>
uint32_t _Mesh<V, I, Q>::getVertexBufferSize() const {
	return this->getVertexCount() * sizeof(vertex);
}

template<typename V, typename I, qualifier Q>
uint32_t _Mesh<V, I, Q>::getIndexBufferSize() const {
	return this->getIndexCount() * sizeof(index);
}

template<typename V, typename I, qualifier Q>
inline const std::vector<typename _Mesh<V, I, Q>::vertex>& _Mesh<V, I, Q>::getVertices() const {
	return m_vertices;
}

template<typename V, typename I, qualifier Q>
inline const std::vector<typename _Mesh<V, I, Q>::triangle>& _Mesh<V, I, Q>::getTriangles() const {
	return m_triangles;
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::fillVertexBuffer(void** vertexBufferPtr) const {
	assert(vertexBufferPtr != NULL);

	uint32_t bufferSize = this->getVertexBufferSize();
	bool allocated = false;
	if (*vertexBufferPtr == NULL) {
		*vertexBufferPtr = new uint8_t[bufferSize]; // malloc(bufferSize);
		allocated = true;
	} // else check buffer is large enough ?

	memcpy(*vertexBufferPtr, &m_vertices[0], bufferSize);
	return allocated;
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::fillIndexBuffer(void** indexBufferPtr) const {
	assert(indexBufferPtr != NULL);

	uint32_t bufferSize = this->getIndexBufferSize();
	bool allocated = false;
	if (*indexBufferPtr == NULL) {
		*indexBufferPtr = new uint8_t[bufferSize];
		allocated = true;
	} // else check buffer is large enough ?

	memcpy(*indexBufferPtr, &m_triangles[0], bufferSize);
	return allocated;
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::isRenderable() const {
	return m_renderable;
}

template<typename V, typename I, qualifier Q>
void _Mesh<V, I, Q>::allocateGPU() {
	if (m_renderable) {
		this->deallocateGPU();
	}

	m_renderable = true;
	m_allocatedVertices = this->getVertexCount();
	m_allocatedIndices = this->getIndexCount();
	glGenVertexArrays(1, &m_vertexArray);
	glBindVertexArray(m_vertexArray);
	glGenBuffers(1, &m_vertexBuffer);
	glGenBuffers(1, &m_indexBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, m_allocatedVertices * sizeof(vertex), NULL, GL_STATIC_DRAW);
	//glBufferStorage(GL_ARRAY_BUFFER, m_allocatedVertices * sizeof(vertex), NULL, 0);
	Mesh::enableVertexAttributes();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_allocatedIndices * sizeof(index), NULL, GL_STATIC_DRAW);
	//glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, m_allocatedIndices * sizeof(reference), NULL, 0);
	glBindVertexArray(0);
}

template<typename V, typename I, qualifier Q>
void _Mesh<V, I, Q>::deallocateGPU() {
	if (m_renderable) {
		glDeleteBuffers(1, &m_vertexBuffer);
		glDeleteBuffers(1, &m_indexBuffer);
		glDeleteVertexArrays(1, &m_vertexArray);
		m_vertexBuffer = 0;
		m_indexBuffer = 0;
		m_vertexArray = 0;
		m_allocatedVertices = 0;
		m_allocatedIndices = 0;
		m_renderable = false;
	}
}

template<typename V, typename I, qualifier Q>
inline void _Mesh<V, I, Q>::deallocateCPU() {
	m_vertices.clear();
	m_triangles.clear();
}

template<typename V, typename I, qualifier Q>
bool _Mesh<V, I, Q>::uploadGPU() {
	if (m_renderable) {
		void *vertexData = NULL, *indexData = NULL;
		this->fillVertexBuffer(&vertexData);
		this->fillIndexBuffer(&indexData);

		uint32_t vertexCount = this->getVertexCount();
		uint32_t indexCount = this->getIndexCount();

		glBindVertexArray(m_vertexArray);

		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
		if (m_allocatedVertices < vertexCount) {
			m_allocatedVertices = vertexCount;
			glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(vertex), NULL, GL_STATIC_DRAW);
			//glBufferStorage(GL_ARRAY_BUFFER, vertexCount * sizeof(vertex), vertexData, 0);
		} else {
			glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(vertex), vertexData);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
		if (m_allocatedIndices < indexCount) {
			m_allocatedIndices = indexCount;
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(index), NULL, GL_STATIC_DRAW);
			//glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(reference), indexData, 0);
		} else {
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexCount * sizeof(index), indexData);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		delete[] vertexData;
		delete[] indexData;
		return true;
	}

	return false;
}

template<typename V, typename I, qualifier Q>
inline bool _Mesh<V, I, Q>::downloadGPU() {
	// TODO
	return false;
}

template<typename V, typename I, qualifier Q>
void _Mesh<V, I, Q>::draw(uint32_t offset, uint32_t count) {
	if (m_renderable) {
		if (count == 0 || count > m_allocatedIndices)
			count = m_allocatedIndices;

		glBindVertexArray(m_vertexArray);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
		glDrawRangeElementsBaseVertex(GL_TRIANGLES, offset, offset + count, count, GL_UNSIGNED_INT, (void*)(offset * sizeof(uint32_t)), 0);
		//glDrawRangeElements(GL_TRIANGLES, offset, offset + count, count, GL_UNSIGNED_INT, (void*) (offset * sizeof(uint32_t)));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
}

// Moller Trumbore triangle intersection test
inline bool rayTriangleIntersection(dvec3 rayOrigin, dvec3 rayDirection, dvec3 v0, dvec3 v1, dvec3 v2, double& distance, dvec3& barycentric) {
	const double eps = 1e-6;
	dvec3 e1, e2, h, s, q;
	double a, f, u, v;
	e1 = v1 - v0;
	e2 = v2 - v0;
	h = cross(rayDirection, e2);
	a = dot(e1, h);
	//if (cullBackface && a < 0.0)
	//	return false;
	if (abs(a) < eps)
		return false;
	f = 1.0 / a;
	s = rayOrigin - v0;
	u = f * dot(s, h);
	if (u < 0.0 || u > 1.0)
		return false;
	q = cross(s, e1);
	v = f * dot(rayDirection, q);
	if (v < 0.0 || u + v > 1.0)
		return false;
	double t = f * dot(e2, q);
	if (t > eps && t < distance) {
		distance = t;
		barycentric = vec3(1.0 - u - v, u, v);
		return true;
	} else {
		return false;
	}
}

template<typename V, typename I, qualifier Q>
inline bool _Mesh<V, I, Q>::getRayIntersection(dvec3 rayOrigin, dvec3 rayDirection, double& closestHitDistance, dvec3& closestHitBarycentric, index& closestHitTriangleIndex, bool anyHit) const {
	bool found = false;

	for (index i = 0; i < m_triangles.size(); ++i) {
		const Mesh::triangle& tri = m_triangles[i];

		const Mesh::vertex& v0 = m_vertices[tri.indices[0]];
		const Mesh::vertex& v1 = m_vertices[tri.indices[1]];
		const Mesh::vertex& v2 = m_vertices[tri.indices[2]];

		dvec4 p0 = dvec4(v0.position, 1.0);
		dvec4 p1 = dvec4(v1.position, 1.0);
		dvec4 p2 = dvec4(v2.position, 1.0);

		double intersectionDistance = closestHitDistance;
		dvec3 intersectionBarycentric;
		if (rayTriangleIntersection(rayOrigin, rayDirection, p0, p1, p2, closestHitDistance, closestHitBarycentric)) {
			closestHitTriangleIndex = i;
			if (anyHit) return true;
			found = true;
		}
	}

	return found;
}

template<typename V, typename I, qualifier Q>
inline void _Mesh<V, I, Q>::addVertexInputs(ShaderProgram* shaderProgram) {
	shaderProgram->addAttribute(0, "vs_vertexPosition");
	shaderProgram->addAttribute(1, "vs_vertexNormal");
	shaderProgram->addAttribute(2, "vs_vertexTangent");
	shaderProgram->addAttribute(3, "vs_vertexTexture");
	shaderProgram->addAttribute(4, "vs_vertexMaterial");
}

template<typename V, typename I, qualifier Q>
inline void _Mesh<V, I, Q>::enableVertexAttributes() {
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Mesh::VERTEX_SIZE, (void*)offsetof(Mesh::vertex, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, Mesh::VERTEX_SIZE, (void*)offsetof(Mesh::vertex, normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Mesh::VERTEX_SIZE, (void*)offsetof(Mesh::vertex, tangent));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, Mesh::VERTEX_SIZE, (void*)offsetof(Mesh::vertex, texture));

	glEnableVertexAttribArray(4);
	glVertexAttribIPointer(4, 1, GL_INT, Mesh::VERTEX_SIZE, (void*)offsetof(Mesh::vertex, material));
}

typedef Mesh::vertex Vertex;
typedef Mesh::index Index;