#include "globals.glsl"
#include "lighting.glsl"

#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 8
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 8
#endif

#ifndef LOCAL_SIZE_Z
#define LOCAL_SIZE_Z 8
#endif

layout (binding = 0, r32ui) uniform coherent volatile uimage3D voxelAlbedoStorage;
layout (binding = 1, r32ui) uniform coherent volatile uimage3D voxelNormalStorage;
layout (binding = 2, r32ui) uniform coherent volatile uimage3D voxelEmissiveStorage;

uniform float voxelGridScale;
uniform vec3 voxelGridCenter;
uniform int voxelGridSize;

uniform vec3 cameraPosition;

uniform PointLight pointLights[MAX_LIGHTS];
uniform DirectionLight directionLights[MAX_LIGHTS];
uniform SpotLight spotLights[MAX_LIGHTS];
uniform int pointLightCount;
uniform int directionLightCount;
uniform int spotLightCount;

uniform sampler2D BRDFIntegrationMap;

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;
void main(void) {
    const ivec3 voxelCoord = ivec3(gl_GlobalInvocationID.xyz);

    if (voxelCoord.x >= voxelGridSize || voxelCoord.y >= voxelGridSize || voxelCoord.z >= voxelGridSize) {
        return;
    }

    vec4 voxelAlbedo = unpackUnorm4x8(imageLoad(voxelAlbedoStorage, voxelCoord).r);
    if (voxelAlbedo.a < 0.001) return;
    
    vec3 voxelNormal = normalize(unpackUnorm4x8((imageLoad(voxelNormalStorage, voxelCoord).r)).xyz * 2.0 - 1.0);
    //vec4 voxelEmissive = unpackUnorm4x8(imageLoad(voxelEmissiveStorage, voxelCoord).r);
    
    SurfacePoint surface;
    surface.depth = 0.0;
    surface.normal = voxelNormal;
    surface.position = voxelGridCenter + (voxelCoord - ivec3(voxelGridSize / 2)) * voxelGridScale;
    surface.exists = true;
    surface.albedo = voxelAlbedo.rgb;
    surface.metalness = 0.0;
    surface.roughness = 1.0;
    surface.ambientOcclusion = 0.0;
    surface.irradiance = vec3(0.0);
    surface.reflection = vec3(0.0);

    // TODO: fix missing shadows
    vec3 finalColour = vec3(0.0);
    calculateLighting(finalColour, surface, pointLights, pointLightCount, BRDFIntegrationMap, cameraPosition, false);

    uint packedAlbedo = packUnorm4x8(vec4(finalColour, voxelAlbedo.a));
    imageStore(voxelAlbedoStorage, voxelCoord, uvec4(packedAlbedo, 0u, 0u, 0u));
}