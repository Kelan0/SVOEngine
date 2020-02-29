#include "globals.glsl"

layout (binding = 0) uniform atomic_uint voxelCount;

layout (binding = 0, rgba16ui) uniform /*coherent*/ volatile uimageBuffer voxelPositionStorage;
layout (binding = 1, rgba32ui) uniform /*coherent*/ volatile uimageBuffer voxelDataStorage;
// layout (binding = 1, rgba8) uniform /*coherent*/ volatile imageBuffer voxelAlbedoStorage;
// layout (binding = 2, rg16f) uniform /*coherent*/ volatile imageBuffer voxelNormalStorage;
// layout (binding = 3, rgb10_a2) uniform /*coherent*/ volatile imageBuffer voxelEmissiveStorage;
layout (binding = 4, r32ui) uniform /*coherent*/ volatile uimageBuffer octreeLeafHeadIndexBuffer;

layout (binding = 5, rgba8) uniform image2D axisProjectionMap;

in VertexData {
    // vec3 projectedPosition;
    vec3 worldPosition;
    vec3 worldNormal;
    vec3 worldTangent;
    vec2 vertexTexture;
    flat int hasTangent;
    flat int dominantAxis;
    // vec4 aabb; // xy=min, zw=max
} fs_in;

uniform int voxelGridSize;
uniform float voxelGridScale;
uniform bool hasMaterial;
uniform Material material;

uniform bool countVoxels;

vec3 calculateNormalMap(sampler2D normalMap, vec3 surfaceTangent, vec3 surfaceNormal) {
    if (fs_in.hasTangent == 0) {
        return surfaceNormal;
    }
    
    vec3 mappedNormal = texture2D(material.normalMap, fs_in.vertexTexture).xyz * 2.0 - 1.0;

    bool grayscale = abs(mappedNormal.r - mappedNormal.g) < 1e-4 && abs(mappedNormal.r - mappedNormal.b) < 1e-4; // r g b all same value.
    if (grayscale) { // normal is grayscale bump map
        return surfaceNormal;
    }

    vec3 surfaceBitangent = cross(surfaceTangent, surfaceNormal);
    mat3 tbn = mat3(surfaceTangent, surfaceBitangent, surfaceNormal);

    return (tbn * mappedNormal);
}

void calculateFragment(out vec3 surfaceAlbedo, out vec3 surfaceNormal, out vec3 surfaceEmissive) {
    vec3 surfaceTangent = normalize(fs_in.worldTangent);
    surfaceNormal = fs_in.worldNormal;
    surfaceAlbedo = vec3(1.0);
    surfaceEmissive = vec3(0.0);
    
    if (hasMaterial) {
        surfaceAlbedo *= material.albedo;
        if (material.hasAlbedoMap) {
            vec4 albedoColour = texture2D(material.albedoMap, fs_in.vertexTexture).rgba;
            if (albedoColour.a < 0.5) discard;
            surfaceAlbedo *= albedoColour.rgb;
        }

        // TODO: investigate, worth calculating normal map? maybe for small voxel sizes only?
        if (material.hasNormalMap) surfaceNormal = calculateNormalMap(material.normalMap, surfaceTangent, surfaceNormal);
    }

    surfaceNormal = normalize(surfaceNormal);
}

ivec3 getVoxelCoord() {
    const int gridSize = voxelGridSize - 1;
    vec3 coord;
    coord.xy = gl_FragCoord.xy;
    coord.z = gl_FragCoord.z * (gridSize);

    switch (fs_in.dominantAxis) {
        case 0: 
            coord.xyz = coord.zyx; 
            break;
        case 1: 
            coord.xyz = coord.xzy; 
            coord.xz = gridSize - coord.xz;
            break;
        case 2: 
            coord.xyz = coord.xyz;
            coord.x = gridSize - coord.x;
            break;
    }

    return ivec3(coord);
}

void imageAtomicAverage(layout(r32ui) coherent volatile uimage3D storage, ivec3 coord, vec4 value) {
    const float scale = 256.0;
    const float invScale = 1.0 / scale;

    value.a = invScale;
    uint newValue = packUnorm4x8(value);
    uint expectedValue = 0;
    uint actualValue;
    vec4 color; 
    int count = 0;
    
    actualValue = imageAtomicCompSwap(storage, coord, expectedValue, newValue);
    
    while (actualValue != expectedValue && count++ < 255) {
        expectedValue = actualValue; 
        color = unpackUnorm4x8(actualValue);

        color.a *= scale;
        color.rgb *= color.a;

        color.rgb += value.rgb;
        color.a += 1.0;

        color.rgb /= color.a;
        color.a *= invScale;

        newValue = packUnorm4x8(color);
        actualValue = imageAtomicCompSwap(storage, coord, expectedValue, newValue);
    }
}

uint packColour(vec4 colour) {
    return packUnorm4x8(colour);
}

vec4 unpackColour(uint colour) {
    return unpackUnorm4x8(colour);
}

uint packNormal(vec3 normal) {
    return packUnorm2x16(encodeNormal(normal));
}

vec3 unpackNormal(uint normal) {
    return decodeNormal(unpackUnorm2x16(normal));
}

uint packEmissive(vec3 emissive) { // pack emissive as 10 bit RGB - max emissivity is 4.0
    emissive = clamp(emissive, 0.0, 4.0) * 255.0;
    return uint(emissive.r) | (uint(emissive.g) << 10) | (uint(emissive.b) << 20);
}

vec3 unpackEmissive(uint emissive) {
    return vec3(emissive & 0x3FF, (emissive >> 10) & 0x3FF, (emissive >> 20) & 0x3FF) / 255.0;
}

void main() {
    // if(fs_in.projectedPosition.x < fs_in.aabb.x || fs_in.projectedPosition.y < fs_in.aabb.y || fs_in.projectedPosition.x > fs_in.aabb.z || fs_in.projectedPosition.y > fs_in.aabb.w)
    //     discard;

    ivec3 voxelCoord = getVoxelCoord();

    uint voxelIndex = atomicCounterIncrement(voxelCount);
    
    if (!countVoxels) {
        vec3 surfaceAlbedo = vec3(0.0);
        vec3 surfaceNormal = vec3(0.0);
        vec3 surfaceEmissive = vec3(0.0);

        calculateFragment(surfaceAlbedo, surfaceNormal, surfaceEmissive);

        uint packedSurfaceAlbedo = packUnorm4x8(vec4(surfaceAlbedo, 1.0));
        uint packedSurfaceNormal = packUnorm2x16(encodeNormal(surfaceNormal));
        uint packedSurfaceEmissive = packUnorm4x8(vec4(surfaceEmissive, 1.0));
        
        imageStore(voxelPositionStorage, int(voxelIndex), uvec4(voxelCoord, 0u)); // TODO make use of unused alpha channel - potential 21-bit coordinate
        imageStore(voxelDataStorage, int(voxelIndex), uvec4(packedSurfaceAlbedo, packedSurfaceNormal, packedSurfaceEmissive, 0u));
        // imageStore(voxelAlbedoStorage, int(voxelIndex), vec4(surfaceAlbedo, 1.0));
        // imageStore(voxelNormalStorage, int(voxelIndex), vec4(encodeNormal(surfaceNormal), 0.0, 0.0));
        // imageStore(voxelEmissiveStorage, int(voxelIndex), vec4(surfaceEmissive, 0.0));
        imageStore(octreeLeafHeadIndexBuffer, int(voxelIndex), uvec4(0xFFFFFFFF, 0u, 0u, 0u));

        #if 0 // debug stuff
        float z = 0.0;

        switch (fs_in.dominantAxis) {
            case 0:
                z = float(voxelCoord.x) / voxelGridSize;
                if (
                    voxelCoord.x < 0 || voxelCoord.x >= voxelGridSize || 
                    voxelCoord.y < 0 || voxelCoord.y >= voxelGridSize || 
                    voxelCoord.z < 0 || voxelCoord.z >= voxelGridSize) 
                    z = 1.0;
                imageStore(axisProjectionMap, voxelCoord.zy, max(imageLoad(axisProjectionMap, voxelCoord.zy), vec4(z, 0.0, 0.0, 1.0)));
                break;
            case 1:
                z = float(voxelCoord.y) / voxelGridSize;
                if (
                    voxelCoord.x < 0 || voxelCoord.x >= voxelGridSize || 
                    voxelCoord.y < 0 || voxelCoord.y >= voxelGridSize || 
                    voxelCoord.z < 0 || voxelCoord.z >= voxelGridSize) 
                    z = 1.0;
                imageStore(axisProjectionMap, voxelCoord.xz, max(imageLoad(axisProjectionMap, voxelCoord.xz), vec4(0.0, z, 0.0, 1.0)));
                break;
            case 2:
                z = float(voxelCoord.z) / voxelGridSize;
                if (
                    voxelCoord.x < 0 || voxelCoord.x >= voxelGridSize || 
                    voxelCoord.y < 0 || voxelCoord.y >= voxelGridSize || 
                    voxelCoord.z < 0 || voxelCoord.z >= voxelGridSize) 
                    z = 1.0;
                imageStore(axisProjectionMap, voxelCoord.xy, max(imageLoad(axisProjectionMap, voxelCoord.xy), vec4(0.0, 0.0, z, 1.0)));
                break;
        }
        #endif
    }
}