#ifndef RAYTRACE_GLSL
#define RAYTRACE_GLSL

#include "globals.glsl"

#ifndef VERTEX_BUFFER_BINDING
#define VERTEX_BUFFER_BINDING 0
#endif

#ifndef TRIANGLE_BUFFER_BINDING
#define TRIANGLE_BUFFER_BINDING 1
#endif

#ifndef BVH_NODE_BUFFER_BINDING
#define BVH_NODE_BUFFER_BINDING 2
#endif

#ifndef BVH_REFERENCE_BUFFER_BINDING
#define BVH_REFERENCE_BUFFER_BINDING 3
#endif

// The flat array of nodes is depth-first - The left child immediately follows the current node. 
// If the node is a leaf, dataOffset is the primitive pointer, else it is the right child pointer
struct BVHNode {
    float xmin, ymin, zmin;
    float xmax, ymax, zmax;
    uint parentIndex;
	uint dataOffset;
	uint primitiveCount_splitAxis_flags;
};

layout (binding = VERTEX_BUFFER_BINDING, std430) buffer VertexBuffer {
   RawVertex vertices[];
};
layout (binding = TRIANGLE_BUFFER_BINDING, std430) buffer TriangleBuffer {
   Triangle triangles[];
};
layout (binding = BVH_NODE_BUFFER_BINDING, std430) buffer BVHNodeBuffer {
    BVHNode bvhNodes[];
};
layout (binding = BVH_REFERENCE_BUFFER_BINDING, std430) buffer BVHReferenceBuffer {
    uint bvhReferences[];
};

Vertex getVertex(uint vertexIndex) {
    RawVertex v = vertices[vertexIndex];

    Vertex vertex;
    vertex.position = vec3(v.px, v.py, v.pz);
    vertex.normal = vec3(v.nx, v.ny, v.nz);
    vertex.tangent = vec3(v.bx, v.by, v.bz);
    vertex.texture = vec2(v.tx, v.ty);
    return vertex;

    // Vertex vertex;
    // vertex.position.x = vertices[vertexIndex * 11 + 0];
    // vertex.position.y = vertices[vertexIndex * 11 + 1];
    // vertex.position.z = vertices[vertexIndex * 11 + 2];
    // vertex.normal.x = vertices[vertexIndex * 11 + 3];
    // vertex.normal.y = vertices[vertexIndex * 11 + 4];
    // vertex.normal.z = vertices[vertexIndex * 11 + 5];
    // vertex.tangent.x = vertices[vertexIndex * 11 + 6];
    // vertex.tangent.y = vertices[vertexIndex * 11 + 7];
    // vertex.tangent.z = vertices[vertexIndex * 11 + 8];
    // vertex.texture.x = vertices[vertexIndex * 11 + 9];
    // vertex.texture.y = vertices[vertexIndex * 11 + 10];
    // return vertex;
}

Triangle getTriangle(in uint triangleIndex) {
    return triangles[triangleIndex];

    // Triangle triangle;
    // triangle.i0 = triangles[triangleIndex * 3 + 0];
    // triangle.i1 = triangles[triangleIndex * 3 + 1];
    // triangle.i2 = triangles[triangleIndex * 3 + 2];
    // return triangle;
}

void getTriangleVertices(in uint triangleIndex, out Vertex v0, out Vertex v1, out Vertex v2) {
    Triangle triangle = getTriangle(triangleIndex);
    v0 = getVertex(triangle.i0);
    v1 = getVertex(triangle.i1);
    v2 = getVertex(triangle.i2);
}

#endif