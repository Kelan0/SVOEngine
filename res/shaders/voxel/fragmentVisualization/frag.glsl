#include "globals.glsl"

in VertexData {
    flat vec4 voxelAlbedo;
    flat vec3 voxelNormal;
    flat vec3 faceNormal;
} fs_in;

out vec3 outAlbedo;
out vec2 outNormal;
out float outRoughness;
out float outMetalness;
out float outAmbientOcclusion;
out vec3 outIrradiance;
out vec3 outReflection;

uniform float aspectRatio;

void main() {
    outAlbedo = fs_in.voxelAlbedo.rgb;
    outNormal = encodeNormal(fs_in.voxelNormal);
    outRoughness = 1.0;
    outMetalness = 0.0;
    outAmbientOcclusion = 0.0;
    outIrradiance = vec3(0.0);
    outReflection = vec3(0.0);
}