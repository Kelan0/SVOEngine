#pragma once

#include "core/pch.h"
#include "core/renderer/geometry/Mesh.h"
#include "core/scene/BVH.h"

// class BoundingVolumeHierarchy;
// struct PrimitiveInfo;
// struct PrimitiveReference;

class ShaderProgram;

struct GeometryRegion {
	uint64_t vertexOffset;
	uint64_t triangleOffset;
	uint64_t triangleCount;
};

class GeometryBuffer {
public:
	GeometryBuffer();

	~GeometryBuffer();

	void bindVertexBuffer(uint32_t index);

	void bindTriangleBuffer(uint32_t index);

	void bindBVHNodeBuffer(uint32_t index);
	
	void bindBVHReferenceBuffer(uint32_t index);

	void bindEmissiveTriangleBuffer(uint32_t index);

	void reset();

	void initializeBuffers();

	void draw(GeometryRegion geometryRegion);

	void allocate(uint64_t vertexCount, uint64_t triangleCount);

	void upload(const std::vector<Mesh::vertex>& vertices, const std::vector<Mesh::triangle>& triangles, GeometryRegion* geometryRegion, glm::dmat4 transformation = dmat4(1.0));

	void buildBVH();

	void applyUniforms(ShaderProgram* shaderProgram);

	uint64_t getAllocatedVertexCount() const;

	uint64_t getAllocatedTriangleCount() const;

	BVH* getBVH() const;

private:
	// The amount counted through the scene tree
	uint64_t m_vertexAllocCount = 0;
	uint64_t m_triangleAllocCount = 0;

	// The allocated size of the buffer in GPU memory
	uint32_t m_vertexBufferSize = 0;
	uint32_t m_triangleBufferSize = 0;
	uint32_t m_bvhNodeBufferSize = 0;
	uint32_t m_bvhReferenceBufferSize = 0;

	// The buffer handles
	uint32_t m_vertexBuffer; // buffer containing linear list of all scene vertices
	uint32_t m_triangleBuffer; // buffer containing linear list of all scene triangles
	uint32_t m_bvhNodeBuffer; // buffer containing linear list of BVH nodes
	uint32_t m_bvhReferenceBuffer; // buffer containing linear list of BVH primitive references.
	uint32_t m_emissiveTriangleBuffer; // buffer containing a list of indices to all triangles that have an emissive material

	uint32_t m_vao;

	std::vector<Mesh::vertex> m_vertices;
	std::vector<Mesh::triangle> m_triangles;
	std::vector<uint32_t> m_emissiveTriangles;
	BVH* m_bvh;
};

