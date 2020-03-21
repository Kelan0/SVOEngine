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
uniform mat4x3 prevCameraRays;
uniform mat4 viewMatrix;
uniform mat4 invViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 invProjectionMatrix;
uniform mat4 viewProjectionMatrix;
uniform mat4 invViewProjectionMatrix;
uniform mat4 prevViewProjectionMatrix;
uniform vec3 cameraPosition;
uniform vec3 prevCameraPosition;
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
uniform uvec2 normalTexture;
uniform uvec2 tangentTexture;
uniform uvec2 velocityTexture;
uniform uvec2 textureCoordTexture;
uniform uvec2 materialIndexTexture;
uniform uvec2 linearDepthTexture;
uniform uvec2 depthTexture;

uniform uvec2 prevTextureCoordTexture;
uniform uvec2 prevMaterialIndexTexture;
uniform uvec2 prevLinearDepthTexture;
uniform uvec2 prevDepthTexture;

uniform uvec2 skyboxEnvironmentTexture;
uniform uvec2 BRDFIntegrationMap;
uniform uvec2 pointLightIcon;

uniform uvec2 raytracedFrame;
// layout(binding = 6, rgba32f) uniform image2D raytracedFrame;

layout (binding = 0, r32ui) uniform uimage2D nodeHeads;
layout (binding = 1, std430) buffer NodeBuffer {
    PackedFragment nodeBuffer[];
};
layout (binding = 2) uniform atomic_uint nodeCounter;
uniform int allocatedFragmentNodes;

layout (binding = 3, r16ui) uniform uimage2D prevReprojectionHistoryTexture;
layout (binding = 4, r16ui) uniform uimage2D reprojectionHistoryTexture;

layout (binding = 4, std430) buffer MaterialBuffer {
    PackedMaterial materialBuffer[];
};

// TODO: investigate if texture storage (TBO/regular texture) would be faster for BVH or geometry storage

uniform samplerCube testDepthCubeTexture;

uniform PointLight pointLights[MAX_LIGHTS];
uniform DirectionLight directionLights[MAX_LIGHTS];
uniform SpotLight spotLights[MAX_LIGHTS];
uniform int pointLightCount;
uniform int directionLightCount;
uniform int spotLightCount;

out vec4 outColour;

SurfacePoint readOpaqueSurfacePoint(vec2 coord) {
    vec3 normal = decodeNormal(texture(sampler2D(normalTexture), coord).rg);
    vec3 tangent = decodeNormal(texture(sampler2D(tangentTexture), coord).rg);
    vec2 textureCoord = texture(sampler2D(textureCoordTexture), coord).xy;
    int materialIndex = texture(isampler2D(materialIndexTexture), coord).x;
    float depth = texture(sampler2D(depthTexture), coord).x;
    
    Material material;
    bool hasMaterial = false;
    bool hasTangent = dot(tangent, tangent) > 1e-4; // tangent is not zero-length

    if (materialIndex >= 0) {
        material = unpackMaterial(materialBuffer[materialIndex]);
        hasMaterial = true;
    }

    Fragment fragment = calculateFragment(material, vec3(0.0), normal, tangent, textureCoord, depth, hasTangent, hasMaterial);

    // Fragment fragment;
    // fragment.depth = texture(sampler2D(depthTexture), coord).x;
    // fragment.albedo = texture(sampler2D(albedoTexture), coord).rgb;
    // fragment.emission = texture(sampler2D(emissionTexture), coord).rgb;
    // fragment.normal = decodeNormal(texture(sampler2D(normalTexture), coord).rg);
    // fragment.roughness = texture(sampler2D(roughnessTexture), coord).r;
    // fragment.metalness = texture(sampler2D(metalnessTexture), coord).r;
    // fragment.ambientOcclusion = texture(sampler2D(ambientOcclusionTexture), coord).r;
    // fragment.irradiance = texture(sampler2D(irradianceTexture), coord).rgb;
    // fragment.reflection = texture(sampler2D(reflectionTexture), coord).rgb;
    // fragment.transmission = texture(sampler2D(transmissionTexture), coord).rgb;

    return fragmentToSurfacePoint(fragment, coord, invViewProjectionMatrix);
}

int readSurfacePoints(vec2 coord, inout SurfacePoint points[MAX_TRANSPARENT_FRAGMENTS]) {
    float opaqueDepth = texture(sampler2D(depthTexture), coord).x;
    int count = 0;

    if (opaqueDepth > 0.0 && opaqueDepth < 1.0) {
        points[0] = readOpaqueSurfacePoint(coord);
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



float getEdgeMagnitude(vec2 coord, vec2 pixelSize, float edgeRange) {
    float NDotV = clamp(dot(
        decodeNormal(texture(sampler2D(normalTexture), coord).rg), 
        normalize(cameraPosition - depthToWorldPosition(texture2D(sampler2D(depthTexture), coord).x, coord, invViewProjectionMatrix))
    ), 0.0, 1.0);

    vec2 offset00 = vec2(-0.5, -0.5) * vec2(pixelSize) * edgeRange;
    vec2 offset10 = vec2(+0.5, -0.5) * vec2(pixelSize) * edgeRange;
    vec2 offset01 = vec2(-0.5, +0.5) * vec2(pixelSize) * edgeRange;
    vec2 offset11 = vec2(+0.5, +0.5) * vec2(pixelSize) * edgeRange;

    vec3 normal00 = texture2D(sampler2D(normalTexture), coord + offset00).xyz;
    float depth00 = texture2D(sampler2D(depthTexture), coord + offset00).x;
    float dist00 = getLinearDepth(depth00, nearPlane, farPlane);

    vec3 normal10 = texture2D(sampler2D(normalTexture), coord + offset10).xyz;
    float depth10 = texture2D(sampler2D(depthTexture), coord + offset10).x;
    float dist10 = getLinearDepth(depth10, nearPlane, farPlane);

    vec3 normal01 = texture2D(sampler2D(normalTexture), coord + offset01).xyz;
    float depth01 = texture2D(sampler2D(depthTexture), coord + offset01).x;
    float dist01 = getLinearDepth(depth01, nearPlane, farPlane);

    vec3 normal11 = texture2D(sampler2D(normalTexture), coord + offset11).xyz;
    float depth11 = texture2D(sampler2D(depthTexture), coord + offset11).x;
    float dist11 = getLinearDepth(depth11, nearPlane, farPlane);

    vec3 normalGradient = (abs(normal10 - normal00) + abs(normal11 - normal01) + abs(normal01 - normal00) + abs(normal11 - normal10));
    float depthGradient = (abs(dist10 - dist00) + abs(dist11 - dist01) + abs(dist01 - dist00) + abs(dist11 - dist10));

    return dot(normalGradient, normalGradient) * 0.1 + depthGradient * depthGradient * NDotV * NDotV * NDotV * NDotV > 0.15 * 0.15 ? 1.0 : 0.0;
}

// float getEdgeMagnitude(vec2 coord, vec2 pixelSize) {
//     vec2 coord00 = coord;
//     vec2 coord10 = coord + vec2(pixelSize.x, 0);
//     vec2 coord01 = coord + vec2(0, pixelSize.y);

//     vec3 normal00 = texture2D(sampler2D(normalTexture), coord00).xyz;
//     vec3 normal10 = texture2D(sampler2D(normalTexture), coord10).xyz;
//     vec3 normal01 = texture2D(sampler2D(normalTexture), coord01).xyz;
//     vec3 normalGradient = abs((normal10 - normal00) + (normal01 - normal00));

//     float depth00 = getLinearDepth(texture2D(sampler2D(depthTexture), coord00).x, nearPlane, farPlane);
//     float depth10 = getLinearDepth(texture2D(sampler2D(depthTexture), coord10).x, nearPlane, farPlane);
//     float depth01 = getLinearDepth(texture2D(sampler2D(depthTexture), coord01).x, nearPlane, farPlane);
//     float depthGradient = abs((depth10 - depth00) + (depth01 - depth00));

//     float f0 = dot(normalGradient, normalGradient);
//     float f1 = depthGradient * depthGradient;
//     return f0 + f1;
// }

// float reprojectPixel(in SurfacePoint surface, in ivec2 currPixel, inout ivec2 prevPixel, inout uint reprojectionCount) {
//     vec2 invResolution = 1.0 / vec2(screenResolution);
//     float currDepth = texelFetch(sampler2D(depthTexture), currPixel, 0).x;
//     vec3 currPosition = depthToWorldPosition(currDepth, vec2(currPixel + 0.5) * invResolution, invViewProjectionMatrix);
//     vec3 prevProjection = projectWorldPosition(currPosition, prevViewProjectionMatrix);

//     float diff = 1.0;
//     prevPixel = ivec2(prevProjection.xy * screenResolution);

//     if (prevProjection.x >= 0.0 && prevProjection.y >= 0.0 && prevProjection.x < 1.0 && prevProjection.y < 1.0) {
//         float prevDepth = texture(sampler2D(prevDepthTexture), prevProjection.xy).x;
//         vec3 prevPosition = depthToWorldPosition(prevDepth, prevProjection.xy, inverse(prevViewProjectionMatrix));
//         vec3 delta = abs(prevPosition - currPosition);
//         diff = max(delta.x, max(delta.y, delta.z));
//     }

//     // float NDotV = 1.0 - dot(surface.normal, normalize(cameraPosition - surface.position));
//     // float NDotV2 = NDotV * NDotV;
//     // float NDotV4 = NDotV2 * NDotV2;

//     float NDotV4 = 0.0;

//     if (diff > mix(0.01, 0.1, NDotV4)) {
//         reprojectionCount = 0u;
//     } else {
//         reprojectionCount = imageLoad(prevReprojectionHistoryTexture, prevPixel).x;
//     }

//     ++reprojectionCount;
    
//     imageStore(reprojectionHistoryTexture, currPixel, uvec4(reprojectionCount));
//     return diff;
// }

float reprojectPixel(in vec2 currPixelCoord, inout vec2 prevPixelCoord, inout uint reprojectionCount) {
    vec2 invScreenSize = 1.0 / vec2(screenSize);
    vec2 velocity = texture(sampler2D(velocityTexture), currPixelCoord).xy;
    prevPixelCoord = currPixelCoord - velocity;

    float diff = 1.0;

    if (prevPixelCoord.x >= 0.0 && prevPixelCoord.y >= 0.0 && prevPixelCoord.x <= 1.0 && prevPixelCoord.y <= 1.0) {

        float currDepth = texture(sampler2D(depthTexture), currPixelCoord).x;
        vec3 currPosition = depthToWorldPosition(currDepth, currPixelCoord, invViewProjectionMatrix);

        // vec2 neighbourCoords[8];
        // vec2 r = 30.0 * invScreenSize;
        // neighbourCoords[0] = currPixelCoord + vec2(-r.x, -r.y);
        // neighbourCoords[1] = currPixelCoord + vec2(   0, -r.y);
        // neighbourCoords[2] = currPixelCoord + vec2(+r.x, -r.y);
        // neighbourCoords[3] = currPixelCoord + vec2(-r.x,    0);
        // neighbourCoords[4] = currPixelCoord + vec2(+r.x,    0);
        // neighbourCoords[5] = currPixelCoord + vec2(-r.x, +r.y);
        // neighbourCoords[6] = currPixelCoord + vec2(   0, +r.y);
        // neighbourCoords[7] = currPixelCoord + vec2(+r.x, +r.y);

        // vec3 minNeighbour = currPosition;
        // vec3 maxNeighbour = currPosition;

        // for (int i = 0; i < 8; i++) {
        //     vec3 neighbour = depthToWorldPosition(texture(sampler2D(depthTexture), neighbourCoords[i]).x, neighbourCoords[i], invViewProjectionMatrix);
        //     minNeighbour = min(minNeighbour, neighbour);
        //     maxNeighbour = max(maxNeighbour, neighbour);
        // }

        float prevDepth = texture(sampler2D(prevDepthTexture), prevPixelCoord).x;
        vec3 prevPosition = depthToWorldPosition(prevDepth, prevPixelCoord, inverse(prevViewProjectionMatrix));
        // if (all(greaterThan(prevPosition, minNeighbour)) && all(lessThan(prevPosition, maxNeighbour))) {
            vec3 delta = currPosition - prevPosition;
            diff = length(delta);
            // diff = 1.0 - currDepth / prevDepth;
        // }
    }

    if (diff > 0.01) {
        reprojectionCount = 0u;
    } else {
        reprojectionCount = imageLoad(prevReprojectionHistoryTexture, ivec2(prevPixelCoord * screenResolution)).x;
    }

    ++reprojectionCount;

    imageStore(reprojectionHistoryTexture, ivec2(currPixelCoord * screenResolution), uvec4(reprojectionCount));

    return diff;
}

void main() {
    ivec2 pixel = ivec2(fs_vertexTexture * screenSize);

    SurfacePoint surface = readOpaqueSurfacePoint(fs_vertexTexture);
        
    vec3 pixelDirection = createRay(fs_vertexTexture, cameraPosition, cameraRays).direction;
    vec3 finalColour = texture(samplerCube(skyboxEnvironmentTexture), pixelDirection.xyz).rgb;

    float variance = 0.0;

    if (surface.exists) {
        // calculateLighting(finalColour, surface, pointLights, pointLightCount, sampler2D(BRDFIntegrationMap), cameraPosition, imageBasedLightingEnabled);
        // finalColour += surface.emission;
        finalColour = vec3(1.0);
        vec4 raytracedFrameColour = texture2D(sampler2D(raytracedFrame), fs_vertexTexture);

        finalColour *= raytracedFrameColour.rgb;
        variance = raytracedFrameColour.a;
    }
    

    // for (int i = count - 1; i >= 0; --i) { // blend from far to near
    //     vec3 surfaceColour = vec3(0.0);
    //     // surfaces[i].ambientOcclusion *= raytracedFrameColour.r;
    //     calculateLighting(surfaceColour, surfaces[i], pointLights, pointLightCount, sampler2D(BRDFIntegrationMap), cameraPosition, imageBasedLightingEnabled);
    //     finalColour = mix(surfaceColour, finalColour, surfaces[i].transmission);
    // }

    finalColour = pow(finalColour / (finalColour + vec3(1.0)), vec3(1.0/2.2)); 

    // finalColour = vec3(heatmap(variance));
    // finalColour = vec3(fwidth(finalColour));
    vec2 prevPixel = fs_vertexTexture;
    uint count = 0u;
    float diff = reprojectPixel(fs_vertexTexture, prevPixel, count);
    // finalColour = heatmap(float(count) / 80.0);

    // finalColour = vec3(heatmap(variance));


    float NDotV = dot(surface.normal, normalize(cameraPosition - surface.position));
    NDotV *= NDotV;
    NDotV *= NDotV;

    bool reproj = diff < mix(0.1, 0.01, NDotV);
    // finalColour = vec3(1.0, 0.0, 0.0);
    // if (reproj) {
    //     uint currMaterialIndex = texture(isampler2D(materialIndexTexture), fs_vertexTexture).x;
    //     vec2 currTextureCoord = texture(sampler2D(textureCoordTexture), fs_vertexTexture).xy;

    //     uint prevMaterialIndex = texture(isampler2D(prevMaterialIndexTexture), prevPixel).x;
    //     vec2 prevTextureCoord = texture(sampler2D(prevTextureCoordTexture), prevPixel).xy;

    //     finalColour = vec3(0.0, 1.0, 0.0);
    //     if (currMaterialIndex == prevMaterialIndex) {
    //         ivec2 currTexelCoord = ivec2(fract(currTextureCoord + 0.5) * 4096.0);
    //         ivec2 prevTexelCoord = ivec2(fract(prevTextureCoord + 0.5) * 4096.0);
    //         if (currTexelCoord == prevTexelCoord) {
    //             finalColour = vec3(1.0, 1.0, 1.0);
    //         }
    //     }
    // }

    // if (reproj) {
    //     Ray currRay = createRay(fs_vertexTexture, cameraPosition, cameraRays);
    //     Ray prevRay = createRay(prevPixel, prevCameraPosition, prevCameraRays);
    //     float currDist = texture(sampler2D(linearDepthTexture), fs_vertexTexture).x;
    //     float prevDist = texture(sampler2D(prevLinearDepthTexture), prevPixel).x;
    //     vec3 currPosition = currRay.origin + currRay.direction * currDist;
    //     vec3 prevPosition = prevRay.origin + prevRay.direction * prevDist;
    //     finalColour = abs(currPosition - prevPosition) * 100.0;
    // } else {
    //     finalColour = vec3(1.0, 0.0, 0.0);
    // }

    outColour = vec4(finalColour, 1.0);
}