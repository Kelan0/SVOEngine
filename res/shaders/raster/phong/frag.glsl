#include "globals.glsl"

layout(early_fragment_tests) in; // Needed for correct transparent pixel occlusion

const float maxReflectionMip = 5.0;

in VertexData {
    vec3 worldPosition;
    vec3 worldNormal;
    vec3 worldTangent;
    vec2 vertexTexture;
    vec4 projectedPosition;
    vec4 prevProjectedPosition;
    flat int hasTangent;
    flat int materialIndex;
} fs_in;


layout (binding = 0, r32ui) uniform uimage2D nodeHeads;
layout (binding = 1, std430) buffer NodeBuffer {
    PackedFragment nodeBuffer[];
};
layout (binding = 2) uniform atomic_uint nodeCounter;

layout (binding = 3, std430) buffer MaterialBuffer {
    PackedMaterial materialBuffer[];
};

uniform int allocatedFragmentNodes;

// uniform bool hasMaterial;
// uniform Material material;

uniform bool lightingEnabled;
uniform bool imageBasedLightingEnabled;
uniform bool hasLightProbe;
uniform samplerCube diffuseIrradianceTexture;
uniform samplerCube specularReflectionTexture;
uniform vec3 cameraPosition;
uniform ivec2 screenSize;
uniform float aspectRatio;

uniform bool transparentRenderPass;

out vec2 outNormal;
out vec2 outTangent;
out vec2 outVelocity;
out vec2 outTextureCoord;
out int outMaterialIndex;
out float outLinearDepth;

// out vec4 outAlbedo;
// out vec3 outEmission;
// out vec2 outNormal;
// out float outRoughness;
// out float outMetalness;
// out float outAmbientOcclusion;
// out vec3 outIrradiance;
// out vec3 outReflection;
// out vec3 outTransmission;

void insertNode(in Fragment fragment) {
    uint x = uint(gl_FragCoord.x);
    uint y = uint(gl_FragCoord.y);

    uint nodeIndex = atomicCounterIncrement(nodeCounter);
    if (nodeIndex < allocatedFragmentNodes) {
        PackedFragment packedFragment = packFragment(fragment);
        packedFragment.next = imageAtomicExchange(nodeHeads, ivec2(x, y), nodeIndex);
        nodeBuffer[nodeIndex] = packedFragment;
    }
}

void main() {
    vec2 invScreenSize = 1.0 / vec2(screenSize);
    vec2 currProjectedPixelPos = fs_in.projectedPosition.xy / fs_in.projectedPosition.w * 0.5 + 0.5;
    vec2 prevProjectedPixelPos = fs_in.prevProjectedPosition.xy / fs_in.prevProjectedPosition.w * 0.5 + 0.5;
    // currProjectedPixelPos = floor(currProjectedPixelPos * screenSize) * invScreenSize;
    // prevProjectedPixelPos = floor(prevProjectedPixelPos * screenSize) * invScreenSize;

    outNormal = encodeNormal(normalize(fs_in.worldNormal));
    outTangent = fs_in.hasTangent != 0 ? encodeNormal(normalize(fs_in.worldTangent)) : vec2(0.0);
    outVelocity = currProjectedPixelPos - prevProjectedPixelPos;
    outTextureCoord = fs_in.vertexTexture;
    outMaterialIndex = fs_in.materialIndex;
    outLinearDepth = length(fs_in.worldPosition - cameraPosition);

//     Material material;
//     bool hasMaterial = false;

//     if (fs_in.materialIndex >= 0) {
//         material = unpackMaterial(materialBuffer[fs_in.materialIndex]);
//         hasMaterial = true;
//     }

//     Fragment fragment = calculateFragment(material, fs_in.worldPosition, fs_in.worldNormal, fs_in.worldTangent, fs_in.vertexTexture, gl_FragCoord.z, fs_in.hasTangent != 0, hasMaterial);

//     if (lightingEnabled && imageBasedLightingEnabled && hasLightProbe) {
//         vec3 surfaceToCamera = cameraPosition - fs_in.worldPosition;
//         vec3 viewReflectionDir = reflect(-surfaceToCamera, fragment.normal); 
//         fragment.irradiance = texture(diffuseIrradianceTexture, fragment.normal).rgb;// * occ;
//         fragment.reflection = textureLod(specularReflectionTexture, viewReflectionDir, fragment.roughness * maxReflectionMip).rgb;// * occ;
//     }

//     // const float eps = 1.0 / 255.0;
//     // const bool transparent = fragment.transmission.r > eps || fragment.transmission.g > eps || fragment.transmission.b > eps;
    
//     // if (transparent) {
//     //     discard;
//     // }
    
// //  if (transparent) {
//         if (transparentRenderPass) {
//             insertNode(fragment);
// //      }
//     } else {
//         outAlbedo = vec4(fragment.transmission, 1.0);
//         outEmission = fragment.emission;
//         outNormal = encodeNormal(fragment.normal);
//         outRoughness = fragment.roughness;
//         outMetalness = fragment.metalness;
//         outAmbientOcclusion = fragment.ambientOcclusion;
//         outIrradiance = fragment.irradiance;
//         outReflection = fragment.reflection;
//         outTransmission = fragment.transmission;

//         // gl_FragDepth = gl_FragCoord.z * 0.5 + 0.5;
//     }
}