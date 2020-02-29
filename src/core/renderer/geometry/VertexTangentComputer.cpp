//#include "core/renderer/geometry/Mesh.h"
//#include "core/renderer/ShaderProgram.h"
//
//VertexTangentComputer::VertexTangentComputer() {
//	m_tangentComputeShader = new ShaderProgram();
//	m_tangentComputeShader->addShader(GL_COMPUTE_SHADER, "shaders/compute/vertexTangents/compute.glsl");
//}
//
//VertexTangentComputer::~VertexTangentComputer() {
//
//}
//
//std::vector<dvec3> VertexTangentComputer::calculateVertexTangents(std::vector<dvec3> vertexPositions, std::vector<dvec2> vertexTextures, std::vector<uint32_t> vertexIndices) {
//	std::vector<dvec3> tangents;
//	tangents.reserve(vertexPositions.size());
//
//	for (int i = 0; i < vertexIndices.size(); i += 3) {
//		glm::dvec3 v0 = vertexPositions[vertexIndices[i + 0]];
//		glm::dvec3 v1 = vertexPositions[vertexIndices[i + 1]];
//		glm::dvec3 v2 = vertexPositions[vertexIndices[i + 2]];
//
//		dvec3 e0 = v1 - v0;
//		dvec3 e1 = v2 - v0;
//		double du0 = v1.m_tex.x - v0.m_tex.x;
//		double dv0 = v1.m_tex.y - v0.m_tex.y;
//		double du1 = v2.m_tex.x - v0.m_tex.x;
//		double dv1 = v2.m_tex.y - v0.m_tex.y;
//
//		double f = 1.0f / (du0 * dv1 - du1 * dv0);
//
//		Vector3f Tangent, Bitangent;
//
//		Tangent.x = f * (dv1 * Edge1.x - dv0 * Edge2.x);
//		Tangent.y = f * (dv1 * Edge1.y - dv0 * Edge2.y);
//		Tangent.z = f * (dv1 * Edge1.z - dv0 * Edge2.z);
//
//		Bitangent.x = f * (-du1 * Edge1.x - du0 * Edge2.x);
//		Bitangent.y = f * (-du1 * Edge1.y - du0 * Edge2.y);
//		Bitangent.z = f * (-du1 * Edge1.z - du0 * Edge2.z);
//
//		v0.m_tangent += Tangent;
//		v1.m_tangent += Tangent;
//		v2.m_tangent += Tangent;
//	}
//
//	return tangents;
//}
//
//VertexTangentComputer* VertexTangentComputer::instance() {
//	static VertexTangentComputer* singelton = new VertexTangentComputer();
//	return singelton;
//}
