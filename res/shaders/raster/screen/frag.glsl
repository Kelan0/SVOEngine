#include "globals.glsl"
#include "lighting.glsl"

//#define VERTEX_BUFFER_BINDING 3
//#define TRIANGLE_BUFFER_BINDING 4
//#define BVH_NODE_BUFFER_BINDING 5
//#define BVH_REFERENCE_BUFFER_BINDING 6 
//
//#include "raytrace/raytrace.glsl"
//#include "raytrace/intersection.glsl"

//#define BASIC_SHADING
//#define BLINN_PHONG

#define MAX_FRAGMENT_LIST_SIZE 1
#define MAX_TRANSPARENT_FRAGMENTS 1

#define PRIM_COST 1.5
#define NODE_COST 1.0

in vec2 fs_vertexPosition;
in vec2 fs_vertexTexture;

// CAMERA UNIFORMS
uniform mat4x3 cameraRays;
uniform mat4 viewMatrix;
uniform mat4 invViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 invProjectionMatrix;
uniform mat4 viewProjectionMatrix;
uniform mat4 invViewProjectionMatrix;
uniform mat4 prevViewProjectionMatrix;
uniform vec3 cameraPosition;
uniform float nearPlane;
uniform float farPlane;

// SCENE UNIFORMS
uniform ivec2 screenSize;
uniform float aspectRatio;
uniform bool lightingEnabled;
uniform bool imageBasedLightingEnabled;
uniform bool transparentRenderPass;

// SCREEN RENDERER UNIFORMS
uniform int renderGBufferMode;
uniform vec2 screenResolution;
uniform uvec2 albedoTexture;
uniform uvec2 normalTexture;
uniform uvec2 roughnessTexture;
uniform uvec2 metalnessTexture;
uniform uvec2 ambientOcclusionTexture;
uniform uvec2 irradianceTexture;
uniform uvec2 reflectionTexture;
uniform uvec2 depthTexture;
uniform uvec2 prevDepthTexture;
uniform uvec2 skyboxEnvironmentTexture;
uniform uvec2 BRDFIntegrationMap;
uniform uvec2 pointLightIcon;

layout(binding = 6, rgba32f) uniform image2D raytracedFrame;

layout (binding = 0, r32ui) uniform uimage2D nodeHeads;
layout (binding = 1, std430) buffer NodeBuffer {
    PackedFragment nodeBuffer[];
};
layout (binding = 2) uniform atomic_uint nodeCounter;
uniform int allocatedFragmentNodes;

layout (binding = 3, r16ui) uniform uimage2D reprojectionHistoryTexture;


// TODO: investigate if texture storage (TBO/regular texture) would be faster for BVH or geometry storage

uniform samplerCube testDepthCubeTexture;

uniform PointLight pointLights[MAX_LIGHTS];
uniform DirectionLight directionLights[MAX_LIGHTS];
uniform SpotLight spotLights[MAX_LIGHTS];
uniform int pointLightCount;
uniform int directionLightCount;
uniform int spotLightCount;

out vec4 outColour;

int readSurfacePoints(vec2 coord, inout SurfacePoint points[MAX_TRANSPARENT_FRAGMENTS]) {
    float opaqueDepth = texture(sampler2D(depthTexture), coord).x;
    int count = 0;

    if (opaqueDepth > 0.0 && opaqueDepth < 1.0) {
        Fragment opaqueFragment;
        opaqueFragment.depth = opaqueDepth;
        opaqueFragment.albedo = texture(sampler2D(albedoTexture), coord).rgb;
        opaqueFragment.normal = decodeNormal(texture(sampler2D(normalTexture), coord).rg);
        opaqueFragment.roughness = texture(sampler2D(roughnessTexture), coord).r;
        opaqueFragment.metalness = texture(sampler2D(metalnessTexture), coord).r;
        opaqueFragment.ambientOcclusion = texture(sampler2D(ambientOcclusionTexture), coord).r;
        opaqueFragment.irradiance = texture(sampler2D(irradianceTexture), coord).rgb;
        opaqueFragment.reflection = texture(sampler2D(reflectionTexture), coord).rgb;
        opaqueFragment.transmission = vec3(0.0);

        points[0] = fragmentToSurfacePoint(opaqueFragment, coord, invViewProjectionMatrix);
        count = 1;
    } else {
        opaqueDepth = 1.0;
    }

    uint index = imageLoad(nodeHeads, ivec2(coord * screenSize)).r;
    if (index != 0xFFFFFFFF) {
        for (int i = 0; i < MAX_FRAGMENT_LIST_SIZE && count < MAX_FRAGMENT_LIST_SIZE && index != 0xFFFFFFFF; i++) {
            PackedFragment packedFragment = nodeBuffer[index];

            points[count++] = packedFragmentToSurfacePoint(packedFragment, coord, invViewProjectionMatrix);

            index = packedFragment.next;
        }

        // insertion sort
        for (int j = 1; j < count; ++j) {
            SurfacePoint key = points[j];
            int i;
            for (i = j - 1; i >= 0 && points[i].depth > key.depth; --i) {
                points[i + 1] = points[i];
            }
            points[i + 1] = key;
        }
    }

    return count;
}

vec4 projectPoint(vec3 point) {
    vec4 p = viewProjectionMatrix * vec4(point, 1.0);
    p.xyz = (p.xyz / p.w) * 0.5 + 0.5;
    p.z *= p.w;
    return p;
}

void calculateLightBillboards(ivec2 pixel, float surfaceDepth, inout vec3 finalColour) {
    for (int i = 0; i < pointLightCount; i++) {
        vec4 projectedLight = projectPoint(pointLights[i].position);

        vec2 d = (vec2(pixel) - projectedLight.xy * screenSize);
        float r = 60.0 / projectedLight.z;
        if (abs(d.x) < r && abs(d.y) < r && projectedLight.z / projectedLight.w < surfaceDepth) { // TODO: projected depth test
            vec3 spriteColour = texture(sampler2D(pointLightIcon), (d.xy + r) / (2.0 * r)).rgb;
            finalColour = mix(finalColour, spriteColour * pointLights[i].colour, spriteColour.r);
        }
    }
}


// vec3 renderGBuffer() {
//     Fragment fragments[MAX_TRANSPARENT_FRAGMENTS];
//     int count = readFragments(coord, fragments);
//     Fragment fragment = fragment[0];

//     switch (renderGBufferMode) {
//         case 1: { // render quadrants of same screen with different gbuffers
//             if (fs_vertexTexture.x < 0.5) {
//                 if (fs_vertexTexture.y < 0.5) { // 00
//                     return fragment.albedo;// readAlbedo(fs_vertexTexture);
//                 } else { // 01
//                     return fragment.normal * 0.5 + 0.5;// readNormal(fs_vertexTexture) * 0.5 + 0.5;
//                 }
//             } else {
//                 if (fs_vertexTexture.y < 0.5) { // 10
//                     return vec3(fragment.roughness);// vec3(readRoughness(fs_vertexTexture));
//                 } else { // 11
//                     return vec3(fragment.metalness);// vec3(readMetalness(fs_vertexTexture));
//                 }
//             }
//         } case 2: { // render the same screen multiple times with different gbuffers
//             if (fs_vertexTexture.x < 0.5) {
//                 if (fs_vertexTexture.y < 0.5) { // 00
//                     return readFragments((fs_vertexTexture - vec2(0.0, 0.0)) / 0.5).albedo;// readAlbedo((fs_vertexTexture - vec2(0.0, 0.0)) / 0.5);
//                 } else { // 01
//                     return readFragments((fs_vertexTexture - vec2(0.0, 0.5)) / 0.5).normal * 0.5 + 0.5;// readNormal((fs_vertexTexture - vec2(0.0, 0.5)) / 0.5) * 0.5 + 0.5;
//                 }
//             } else {
//                 if (fs_vertexTexture.y < 0.5) { // 10
//                     return vec3(readFragments((fs_vertexTexture - vec2(0.5, 0.0)) / 0.5).roughness);// vec3(readRoughness((fs_vertexTexture - vec2(0.5, 0.0)) / 0.5));
//                 } else { // 11
//                     return vec3(readFragments((fs_vertexTexture - vec2(0.5, 0.5)) / 0.5).metalness);// vec3(readMetalness((fs_vertexTexture - vec2(0.5, 0.5)) / 0.5));
//                 }
//             }
//         } case 3: { // display albedo only
//             return fragment.albedo;// readAlbedo(fs_vertexTexture);
//         } case 4: { // display normals only
//             return fragment.normal * 0.5 + 0.5;// readNormal(fs_vertexTexture) * 0.5 + 0.5;
//         } case 5: { // display roughness only
//             return vec3(fragment.roughness);// vec3(readRoughness(fs_vertexTexture));
//         } case 6: { // display metalness only
//             return vec3(fragment.metalness);// vec3(readMetalness(fs_vertexTexture));
//         } case 7: { // display irradiance only
//             return vec3(fragment.irradiance);// vec3(readIrradiance(fs_vertexTexture));
//         } case 8: { // display reflection only
//             return vec3(fragment.reflection);// vec3(readReflection(fs_vertexTexture));
//         } default: {
//             return vec3(0.0);
//         }
//     }
// }

vec4 renderEquirectangularCubemap(in samplerCube cubeTexture, ivec2 pixel, ivec2 position, ivec2 size) {
    vec2 coord = vec2(pixel - position) / size;
    if (coord.x >= 0.0 && coord.y >= 0.0 && coord.x < 1.0 && coord.y < 1.0) {
        float theta = ((coord.x) * 2.0 * PI) + PI / 2.0;
        float phi = (coord.y) * PI - 0.5 * PI;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);
        vec3 dir = vec3(cosPhi * cosTheta, sinPhi, cosPhi * sinTheta);
        return texture(cubeTexture, dir);
    }
    return vec4(0.0);
}


void main() {
    ivec2 pixel = ivec2(fs_vertexTexture * screenSize);
    SurfacePoint surfaces[MAX_TRANSPARENT_FRAGMENTS];
    int count = readSurfacePoints(fs_vertexTexture, surfaces);

    vec3 pixelDirection = createRay(fs_vertexTexture, cameraPosition, cameraRays).direction;
    vec3 finalColour = texture(samplerCube(skyboxEnvironmentTexture), pixelDirection.xyz).rgb;

    // vec4 raytracedFrameColour = imageLoad(raytracedFrame, pixel).rgba;

    // if (raytracedFrameColour.a > 0.0) {
    //     finalColour = raytracedFrameColour.rgb / raytracedFrameColour.a;
    // }

    for (int i = count - 1; i >= 0; --i) { // blend from far to near
        vec3 surfaceColour = vec3(0.0);
        // surfaces[i].ambientOcclusion *= raytracedFrameColour.r;
        calculateLighting(surfaceColour, surfaces[i], pointLights, pointLightCount, sampler2D(BRDFIntegrationMap), cameraPosition, imageBasedLightingEnabled);
        finalColour = mix(surfaceColour, finalColour, surfaces[i].transmission);
    }

    //calculateLightBillboards(pixel, surfaces[0].depth, finalColour);
    
    // vec4 reprojectedPosition = prevViewProjectionMatrix * vec4(surfaces[0].position, 1.0);
    // reprojectedPosition.xyz = reprojectedPosition.xyz / reprojectedPosition.w;
    // reprojectedPosition.xyz = reprojectedPosition.xyz * 0.5 + 0.5;
    // ivec2 reprojectedPixel = ivec2(reprojectedPosition.xy * screenSize);

    // float prevDepth = texture(sampler2D(prevDepthTexture), reprojectedPosition.xy).x;
    // float currDepth = texture(sampler2D(depthTexture), fs_vertexTexture).x;
    // vec3 prevCoord = depthToWorldPosition(prevDepth, reprojectedPosition.xy, inverse(prevViewProjectionMatrix));
    // vec3 currCoord = depthToWorldPosition(currDepth, fs_vertexTexture.xy, inverse(viewProjectionMatrix));

    // // float depthSimilarity = clamp(pow(prevDepth / currDepth, 4.0) + 0.2, 0.0, 1.0);
    // vec3 deltaCoord = currCoord - prevCoord;
    // float deltaDistance = dot(deltaCoord, deltaCoord);

    // // if (deltaDistance > 0.1 * 0.1) {
    // //     imageStore(reprojectionHistoryTexture, pixel, uvec4(0u));
    // // } else {
    // //     uint reprojectionHistory = imageLoad(reprojectionHistoryTexture, pixel).r + 1;

    // //     imageStore(reprojectionHistoryTexture, pixel, uvec4(reprojectionHistory));

    // //     finalColour = heatmap(float(reprojectionHistory) / 65536.0);   
    // // }

    // finalColour = vec3(1.0 / (1.0 + deltaDistance));


    finalColour = pow(finalColour / (finalColour + vec3(1.0)), vec3(1.0/2.2)); 

    outColour = vec4(finalColour, 1.0);
}