#include "core/renderer/geometry/GeometryBuffer.h"
#include "core/renderer/MaterialManager.h"
#include "core/renderer/ShaderProgram.h"
#include "core/scene/Scene.h"

GeometryBuffer::GeometryBuffer() {
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vertexBuffer);
	glGenBuffers(1, &m_triangleBuffer);
	glGenBuffers(1, &m_bvhNodeBuffer);
	glGenBuffers(1, &m_bvhReferenceBuffer);
	glGenBuffers(1, &m_emissiveTriangleBuffer);
}

GeometryBuffer::~GeometryBuffer() {
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vertexBuffer);
	glDeleteBuffers(1, &m_triangleBuffer);
	glDeleteBuffers(1, &m_bvhNodeBuffer);
	glDeleteBuffers(1, &m_bvhReferenceBuffer);
	glDeleteBuffers(1, &m_emissiveTriangleBuffer);
}

void GeometryBuffer::bindVertexBuffer(uint32_t index) {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_vertexBuffer);
}

void GeometryBuffer::bindTriangleBuffer(uint32_t index) {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_triangleBuffer);
}

void GeometryBuffer::bindBVHNodeBuffer(uint32_t index) {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_bvhNodeBuffer);
}

void GeometryBuffer::bindBVHReferenceBuffer(uint32_t index) {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_bvhReferenceBuffer);
}

void GeometryBuffer::bindEmissiveTriangleBuffer(uint32_t index) {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_emissiveTriangleBuffer);
}

void GeometryBuffer::reset() {
	m_vertexAllocCount = 0;
	m_triangleAllocCount = 0;
	m_vertices.clear();
	m_triangles.clear();
}

void GeometryBuffer::initializeBuffers() {
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triangleBuffer);

	if (m_vertexBufferSize <= m_vertexAllocCount) { // current buffer is too small
		m_vertexBufferSize = m_vertexAllocCount;
		glBufferData(GL_ARRAY_BUFFER, m_vertexBufferSize * sizeof(Mesh::vertex), NULL, GL_STATIC_DRAW);
	}

	if (m_triangleBufferSize <= m_triangleAllocCount) { // current buffer is too small
		m_triangleBufferSize = m_triangleAllocCount;
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_triangleBufferSize * sizeof(Mesh::triangle), NULL, GL_STATIC_DRAW);
	}

	Mesh::enableVertexAttributes();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}

void GeometryBuffer::draw(GeometryRegion geometryRegion) {
	uint64_t baseVertex = 0;// geometryRegion.vertexOffset;
	uint64_t offset = geometryRegion.triangleOffset * 3;
	uint64_t count = geometryRegion.triangleCount * 3;

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triangleBuffer);
	glDrawRangeElementsBaseVertex(GL_TRIANGLES, offset, offset + count, count, GL_UNSIGNED_INT, (void*)(offset * sizeof(uint32_t)), baseVertex);
	//glDrawRangeElements(GL_TRIANGLES, offset, offset + count, count, GL_UNSIGNED_INT, (void*) (offset * sizeof(uint32_t)));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GeometryBuffer::allocate(uint64_t vertexCount, uint64_t triangleCount) {
	m_vertexAllocCount += vertexCount;
	m_triangleAllocCount += triangleCount;
}

void GeometryBuffer::upload(const std::vector<Mesh::vertex>& vertices, const std::vector<Mesh::triangle>& triangles, GeometryRegion* geometryRegion, glm::dmat4 transformation) {
	if (geometryRegion != NULL) {
		uint64_t vertexOffset = m_vertices.size();
		m_vertices.resize(vertexOffset + vertices.size());
		std::memcpy(&m_vertices[vertexOffset], &vertices[0], sizeof(Mesh::vertex) * vertices.size());

		uint64_t triangleOffset = m_triangles.size();
		m_triangles.resize(triangleOffset + triangles.size());
		for (int i = 0; i < triangles.size(); i++) {
			uint32_t triangleIndex = triangleOffset + i;
			m_triangles[triangleIndex].i0 = vertexOffset + triangles[i].i0;
			m_triangles[triangleIndex].i1 = vertexOffset + triangles[i].i1;
			m_triangles[triangleIndex].i2 = vertexOffset + triangles[i].i2;

			int32_t materialIndex = m_vertices[m_triangles[triangleIndex].i0].material;
			if (materialIndex >= 0) {
				Material* material = Engine::scene()->getMaterialManager()->getMaterial(materialIndex);
				if (material->isEmissive()) {
					m_emissiveTriangles.push_back(triangleIndex);
				}
			}
		}

		//std::memcpy(&m_triangles[triangleOffset], &triangles[0], sizeof(Mesh::triangle) * triangles.size());

		if (transformation != dmat4(1)) {
			for (int i = 0; i < vertices.size(); ++i) {
				m_vertices[vertexOffset + i] *= transformation;
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
		//glBufferSubData(GL_ARRAY_BUFFER, vertexOffset * sizeof(Mesh::vertex), vertices.size() * sizeof(Mesh::vertex), &m_vertices[vertexOffset]);
		glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Mesh::vertex), &m_vertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triangleBuffer);
		//glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, triangleOffset * sizeof(Mesh::triangle), triangles.size() * sizeof(Mesh::triangle), &m_triangles[triangleOffset]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_triangles.size() * sizeof(Mesh::triangle), &m_triangles[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_emissiveTriangleBuffer);
		//glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, triangleOffset * sizeof(Mesh::triangle), triangles.size() * sizeof(Mesh::triangle), &m_triangles[triangleOffset]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, m_emissiveTriangles.size() * sizeof(uint32_t), &m_emissiveTriangles[0], GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		geometryRegion->vertexOffset = vertexOffset;
		geometryRegion->triangleOffset = triangleOffset;
		geometryRegion->triangleCount = triangles.size();
	}

	//m_bvh->build()
}

void GeometryBuffer::buildBVH() {
	m_bvh = BVH::build(m_vertices, m_triangles);

	if (m_bvh != NULL) {
		const std::vector<BVHBinaryNode>& linearNodes = m_bvh->createLinearNodes();
		const std::vector<BVH::PrimitiveReference>& primitiveReferences = m_bvh->getPrimitiveReferences();

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_bvhNodeBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BVHBinaryNode) * linearNodes.size(), &linearNodes[0], GL_STATIC_DRAW);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_bvhReferenceBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BVH::PrimitiveReference) * primitiveReferences.size(), &primitiveReferences[0], GL_STATIC_DRAW);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}

void GeometryBuffer::applyUniforms(ShaderProgram* shaderProgram) {
	shaderProgram->setUniform("emissiveTriangleCount", (uint32_t) m_emissiveTriangles.size());
}

uint64_t GeometryBuffer::getAllocatedVertexCount() const {
	return m_vertexAllocCount;
}

uint64_t GeometryBuffer::getAllocatedTriangleCount() const {
	return m_triangleAllocCount;
}

BVH* GeometryBuffer::getBVH() const {
	return m_bvh;
}
