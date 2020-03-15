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

#ifndef EMISSIVE_TRIANGLE_BUFFER_BINDING
#define EMISSIVE_TRIANGLE_BUFFER_BINDING 4
#endif

#ifndef MATERIAL_BUFFER_BINDING
#define MATERIAL_BUFFER_BINDING 5
#endif

struct IntersectionInfo {
    float dist;
    uint triangleIndex;
    vec3 barycentric;
    Vertex v0;
    Vertex v1;
    Vertex v2;
};

// The flat array of nodes is depth-first - The left child immediately follows the current node. 
// If the node is a leaf, dataOffset is the primitive pointer, else it is the right child pointer
struct BVHNode {
    float xmin, ymin, zmin;
    float xmax, ymax, zmax;
    uint parentIndex;
	uint dataOffset;
	uint primitiveCount_splitAxis_flags;
};

struct EmissiveSurfaceInfo {
    uint index;
    float area;
};

struct EmissiveSample {
    vec3 position;
    float area;
    Fragment surfaceFragment;
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
layout (binding = EMISSIVE_TRIANGLE_BUFFER_BINDING, std430) buffer EmissiveTriangleBuffer {
    uint emissiveTriangleBuffer[];
};
layout (binding = MATERIAL_BUFFER_BINDING, std430) buffer MaterialBuffer {
    PackedMaterial materialBuffer[];
};

uniform uint emissiveTriangleCount;

Vertex getVertex(uint vertexIndex) {
    RawVertex v = vertices[vertexIndex];

    Vertex vertex;
    vertex.position = vec3(v.px, v.py, v.pz);
    vertex.normal = vec3(v.nx, v.ny, v.nz);
    vertex.tangent = vec3(v.bx, v.by, v.bz);
    vertex.texture = vec2(v.tx, v.ty);
    vertex.material = v.m;
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

Fragment getInterpolatedTriangleFragment(vec3 barycentric, Vertex v0, Vertex v1, Vertex v2, float depth = 0.5) {
    vec3 position = mat3x3(v0.position, v1.position, v2.position) * barycentric;
    vec3 normal = mat3x3(v0.normal, v1.normal, v2.normal) * barycentric;
    vec3 tangent = mat3x3(v0.tangent, v1.tangent, v2.tangent) * barycentric;
    vec2 texture = mat3x2(v0.texture, v1.texture, v2.texture) * barycentric;
    uint materialIndex = v0.material;

    bool hasTangent = dot(tangent, tangent) > 1e-3;
    bool hasMaterial = materialIndex >= 0;
    Material material;

    if (hasMaterial) {
        material = unpackMaterial(materialBuffer[materialIndex]);
    }
    
    return calculateFragment(material, position, normal, tangent, texture, depth, hasTangent, hasMaterial);
}

Fragment getInterpolatedIntersectionFragment(IntersectionInfo intersection) {
    return getInterpolatedTriangleFragment(intersection.barycentric, intersection.v0, intersection.v1, intersection.v2, intersection.dist);
}

vec4 getNextEmissiveSampleDirection(inout vec2 seed, in vec3 origin) {
    // Select random emissive triangle, select uniform random point within triangle, get direction to sample point
    uint sampleTriangleIndex = emissiveTriangleBuffer[uint(nextRandom(seed) * emissiveTriangleCount)];
    
    Vertex v0, v1, v2;
    getTriangleVertices(sampleTriangleIndex, v0, v1, v2);
    
    vec3 samplePosition = getRandomTrianglePoint(seed, v0.position, v1.position, v2.position);
    float sampleSurfaceArea = getTriangleSurfaceArea(v0.position, v1.position, v2.position);
    return vec4(normalize(samplePosition - origin), sampleSurfaceArea);
}

EmissiveSample getNextEmissiveSample(inout vec2 seed) {
    uint sampleTriangleIndex = emissiveTriangleBuffer[uint(nextRandom(seed) * emissiveTriangleCount)];
    
    Vertex v0, v1, v2;
    getTriangleVertices(sampleTriangleIndex, v0, v1, v2);
    
    // vec3 normal = cross(v1.position - v0.position, v2.position - v0.position);
    // float area = length(n);
    // normal /= area; // avoid two square roots.
    // area *= 0.5;
    
    vec3 barycentric = getRandomBarycentricCoord(seed);
    Fragment fragment = getInterpolatedTriangleFragment(barycentric, v0, v1, v2);
    
    EmissiveSample emissiveSample;
    emissiveSample.position = mat3x3(v0.position, v1.position, v2.position) * barycentric;
    emissiveSample.area = getTriangleSurfaceArea(v0.position, v1.position, v2.position);
    emissiveSample.surfaceFragment = fragment;
    return emissiveSample;
}

float powerHeuristic(float a, float b) {
    float a2 = a * a;
    return a2 / (b * b + a2);
}

#endif