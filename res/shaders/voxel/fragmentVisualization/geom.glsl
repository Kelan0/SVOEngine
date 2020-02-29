#define PI 3.1415926535897932384626433832795028841971

layout(points) in;
layout(triangle_strip, max_vertices = 36) out;

const int elements[] = int[] (
    0,1,2, 2,1,3, // x-
    5,4,7, 7,4,6, // x+
    5,1,4, 4,1,0, // y-
    3,7,2, 2,7,6, // y+
    4,0,6, 6,0,2, // z-
    1,5,3, 3,5,7  // z+
);
const vec3 normals[] = vec3[] (
    vec3(-1.0, 0.0, 0.0),
    vec3(+1.0, 0.0, 0.0),
    vec3(0.0, -1.0, 0.0),
    vec3(0.0, +1.0, 0.0),
    vec3(0.0, 0.0, -1.0),
    vec3(0.0, 0.0, +1.0)
);
const vec3 vertices[] = vec3[] (
    vec3(-1.0, -1.0, -1.0), // 0
    vec3(-1.0, -1.0, +1.0), // 1
    vec3(-1.0, +1.0, -1.0), // 2
    vec3(-1.0, +1.0, +1.0), // 3
    vec3(+1.0, -1.0, -1.0), // 4
    vec3(+1.0, -1.0, +1.0), // 5
    vec3(+1.0, +1.0, -1.0), // 6
    vec3(+1.0, +1.0, +1.0)  // 7
);

// layout (binding = 0, rgba16f) uniform image3D voxelVisualizationTexture;

in VertexData {
    ivec3 gridCoord;
    vec3 voxelPosition;
    vec4 voxelAlbedo;
    vec3 voxelNormal;
    vec3 voxelEmissive;
} gs_in[];

out VertexData {
    flat vec4 voxelAlbedo;
    flat vec3 voxelNormal;
    flat vec3 faceNormal;
} gs_out;

uniform float voxelGridScale;
uniform vec3 voxelGridCenter;
uniform mat4 viewProjectionMatrix;
uniform vec3 cameraPosition;
uniform int octreeLevel;
uniform int octreeDepth;

// vec4 decodeRGBA8(uint rgba) {
//     return vec4(
//         float((rgba & 0x000000FF)),
//         float((rgba & 0x0000FF00) >> 8u),
//         float((rgba & 0x00FF0000) >> 16u),
//         float((rgba & 0xFF000000) >> 24u)
//     );
// }

vec2 encodeNormal(vec3 normal) {
    return (vec2(atan(normal.y, normal.x) / PI, normal.z) + 1.0) * 0.5;
}

vec3 decodeNormal(vec2 enc) {
    vec2 ang = enc * 2.0 - 1.0;
    float rad = ang.x * PI;
    float sinTheta = sin(rad);
    float cosTheta = cos(rad);
    float sinPhi = sqrt(1.0 - ang.y * ang.y);
    float cosPhi = ang.y;
    return vec3(cosTheta * sinPhi, sinTheta * sinPhi, cosPhi);
}

void main() {
    // vec4 voxelData = imageLoad(voxelVisualizationTexture, gs_in[0].gridCoord).rgba;

    // vec4 neighbours[] = vec4[] (
    //     vec4(0.0),//unpackUnorm4x8(imageLoad(voxelAlbedoStorage, gs_in[0].gridCoord - ivec3(1,0,0)).r),
    //     vec4(0.0),//unpackUnorm4x8(imageLoad(voxelAlbedoStorage, gs_in[0].gridCoord + ivec3(1,0,0)).r),
    //     vec4(0.0),//unpackUnorm4x8(imageLoad(voxelAlbedoStorage, gs_in[0].gridCoord - ivec3(0,1,0)).r),
    //     vec4(0.0),//unpackUnorm4x8(imageLoad(voxelAlbedoStorage, gs_in[0].gridCoord + ivec3(0,1,0)).r),
    //     vec4(0.0),//unpackUnorm4x8(imageLoad(voxelAlbedoStorage, gs_in[0].gridCoord - ivec3(0,0,1)).r),
    //     vec4(0.0) //unpackUnorm4x8(imageLoad(voxelAlbedoStorage, gs_in[0].gridCoord + ivec3(0,0,1)).r)
    // );

    if (gs_in[0].voxelAlbedo.a > 0.0) {
        float voxelSize = voxelGridScale * 0.5 * (1 << (octreeDepth - (octreeLevel + 1)));
        vec3 voxelPosition = gs_in[0].voxelPosition;
        // vec3 voxelToCamera = cameraPosition - voxelPosition;

        uint idx = 0;
        for(uint i = 0; i < 12; i++) { // tri
            uint face = i / 2;
            // if (neighbours[face].a <= 0.0) {
            vec3 n = normals[face];
            // if (dot(n, voxelToCamera) > 0.0) {
                for (int k = 0; k < 3; k++) { // vert
                    gs_out.voxelAlbedo = gs_in[0].voxelAlbedo;
                    gs_out.voxelNormal = gs_in[0].voxelNormal;
                    gs_out.faceNormal = n;
                    gl_Position = viewProjectionMatrix * vec4(voxelPosition + vertices[elements[idx++]] * voxelSize, 1.0);
                    EmitVertex();
                }
                EndPrimitive();
            // }
            // }
        }
    }
}

