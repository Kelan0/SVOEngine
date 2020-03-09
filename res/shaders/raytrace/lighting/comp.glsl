#include "globals.glsl"
#include "lighting.glsl"
#include "UE4BRDF.glsl"
#include "raytrace/raytrace.glsl"
#include "raytrace/intersection.glsl"

#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 84
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 8
#endif

// CAMERA UNIFORMS
uniform mat4x3 cameraRays;
uniform mat4 viewMatrix;
uniform mat4 invViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 invProjectionMatrix;
uniform mat4 viewProjectionMatrix;
uniform mat4 invViewProjectionMatrix;
uniform vec3 cameraPosition;
uniform float nearPlane;
uniform float farPlane;
uniform bool cameraMoved;

// SCENE UNIFORMS
uniform ivec2 screenSize;
uniform float aspectRatio;
uniform bool lightingEnabled;
uniform bool imageBasedLightingEnabled;
uniform bool transparentRenderPass;
uniform int materialCount;

// GBUFFER UNIFORMS
uniform sampler2D albedoTexture;
uniform sampler2D emissionTexture;
uniform sampler2D normalTexture;
uniform sampler2D roughnessTexture;
uniform sampler2D metalnessTexture;
uniform sampler2D ambientOcclusionTexture;
uniform sampler2D irradianceTexture;
uniform sampler2D reflectionTexture;
uniform sampler2D depthTexture;

// ENVIRONMENT TEXTURES
uniform samplerCube skyboxEnvironmentTexture;
uniform samplerCube skyboxDiffuseIrradianceTexture;
uniform samplerCube skyboxPrefilteredEnvironmentTexture;
uniform sampler2D BRDFIntegrationMap;

// SCENE LIGHTING
uniform PointLight pointLights[MAX_LIGHTS];
uniform DirectionLight directionLights[MAX_LIGHTS];
uniform SpotLight spotLights[MAX_LIGHTS];
uniform int pointLightCount;
uniform int directionLightCount;
uniform int spotLightCount;

// RAYTRACING
uniform int maxRenderScale;
uniform bool lowResolutionFramePass;
uniform sampler2D lowResolutionFrame;

layout(binding = 0, rgba32f) uniform image2D frameTexture;

SurfacePoint readSurfacePoint(vec2 coord) {
    Fragment frag;
    frag.depth = texture(depthTexture, coord).x;
    frag.albedo = texture(albedoTexture, coord).rgb;
    frag.emission = texture(emissionTexture, coord).rgb;
    frag.normal = decodeNormal(texture(normalTexture, coord).rg);
    frag.roughness = texture(roughnessTexture, coord).r;
    frag.metalness = texture(metalnessTexture, coord).r;
    frag.ambientOcclusion = texture(ambientOcclusionTexture, coord).r;
    frag.irradiance = texture(irradianceTexture, coord).rgb;
    frag.reflection = texture(reflectionTexture, coord).rgb;
    frag.transmission = vec3(0.0);

    return fragmentToSurfacePoint(frag, coord, invViewProjectionMatrix);
}

vec3 uniformSphereSample(inout vec2 seed) {
    vec3 v = nextRandomVec3(seed);
    v *= 2.0;
    v -= 1.0;
    return normalize(v);
}

vec4 uniformHemisphereSample(in vec3 normal, inout vec2 seed) {
    vec4 v; // x,y,z,cosTheta
    v.xyz = uniformSphereSample(seed);
    v.w = dot(v.xyz, normal);
    return v * sign(v.w);
}

void calculateNextSurfacePoint(in Ray ray, in IntersectionInfo intersection, inout SurfacePoint surface) {
    Fragment fragment = getInterpolatedIntersectionFragment(intersection);
    
    surface.exists = true;
    surface.transparent = false;
    surface.invalidNormal = false;

    surface.position = ray.origin + ray.direction * (intersection.dist - 1e-6);
    surface.normal = fragment.normal;
    surface.depth = 0.5;

    surface.albedo = fragment.albedo;
    surface.transmission = fragment.transmission;
    surface.emission = fragment.emission;
    surface.metalness = fragment.metalness;
    surface.roughness = fragment.roughness;
    surface.ambientOcclusion = fragment.ambientOcclusion;
    surface.irradiance = fragment.irradiance;
    surface.reflection = fragment.reflection;
}



vec3 calculateDirectLight(in vec3 wo, in vec3 wi, in SurfacePoint surface) {
    vec3 radiance = vec3(0.0);

    // Ray sampleRay = createRay(surface.position + wi * 1e-5, wi);
    // float sampleDistance = INFINITY;
    // if (!occlusionRayIntersectsBVH(sampleRay, true, sampleDistance)) {
    //     // ray reaches sky, add environment radiance
    //     radiance += texture(skyboxEnvironmentTexture, wi).rgb * PI;
    // }

    // analytic light sources

    return radiance;
}

void calculatePathTracedLighting(inout vec3 finalColour, in SurfacePoint surface, in vec2 seed) {
    vec3 radiance = vec3(0.0);

    vec3 wo, wi;
    vec3 Li, Le, Fr;
    vec3 energy = vec3(1.0);
    Ray sampleRay;
    IntersectionInfo intersection;
    SurfacePoint currentSurface = surface;

    intersection.dist = INFINITY;

    float offsetEPS = 1e-2;

    for (int i = 0; i < 2; i++) {
        if (!currentSurface.exists || (energy.r < 0.05 && energy.g < 0.05 && energy.b < 0.05)) {
            break;
        }
        
        wo = normalize(cameraPosition - currentSurface.position);
        wi = sampleBRDF(wo, currentSurface, seed);

        sampleRay = createRay(currentSurface.position + wi * offsetEPS, wi);
        intersection.dist = INFINITY;
        if (!sampleRayIntersectsBVH(sampleRay, false, intersection)) {
            radiance += texture(skyboxEnvironmentTexture, wi).rgb * PI * energy;
            break;
        }

        intersection.dist -= offsetEPS;

        if (i != 0) { // skip first bounce
            Le = currentSurface.emission * PI;
            Fr = surface.albedo;//evaluateBRDF(wo, wi, currentSurface);
            Li = calculateDirectLight(wo, wi, currentSurface);
            radiance += (Le + Fr * Li) * energy;
            energy *= Fr;
        }

        calculateNextSurfacePoint(sampleRay, intersection, currentSurface);
    }

    finalColour = radiance;// * vec3(0.5, 1.0, 0.8);
}

bool pathTracePrimaryRays(inout vec3 finalColour, vec2 screenPos) {
    Ray ray = createRay(screenPos, cameraPosition, cameraRays);

    IntersectionInfo intersection;
    intersection.dist = INFINITY;

    if (!sampleRayIntersectsBVH(ray, false, intersection)) {
        return false;
    }

    Fragment frag = getInterpolatedIntersectionFragment(intersection);
    
    finalColour = frag.albedo + frag.emission;
    return true;
}

float getEdgeMagnitude(vec2 coord, vec2 pixelSize, float edgeRange) {

    float NDotV = clamp(dot(
        decodeNormal(texture(sampler2D(normalTexture), coord).rg), 
        normalize(cameraPosition - depthToWorldPosition(texture2D(sampler2D(depthTexture), coord).x, coord, invViewProjectionMatrix))
    ), 0.0, 1.0);

    float NDotV2 = NDotV * NDotV;

    vec2 offset00 = vec2(-0.5, -0.5) * vec2(pixelSize) * edgeRange;
    vec2 offset10 = vec2(+0.5, -0.5) * vec2(pixelSize) * edgeRange;
    vec2 offset01 = vec2(-0.5, +0.5) * vec2(pixelSize) * edgeRange;
    vec2 offset11 = vec2(+0.5, +0.5) * vec2(pixelSize) * edgeRange;

    vec3 normal00 = decodeNormal(texture2D(sampler2D(normalTexture), coord + offset00).rg);
    float depth00 = texture2D(sampler2D(depthTexture), coord + offset00).x;
    float dist00 = getLinearDepth(depth00, nearPlane, farPlane);

    vec3 normal10 = decodeNormal(texture2D(sampler2D(normalTexture), coord + offset10).rg);
    float depth10 = texture2D(sampler2D(depthTexture), coord + offset10).x;
    float dist10 = getLinearDepth(depth10, nearPlane, farPlane);

    vec3 normal01 = decodeNormal(texture2D(sampler2D(normalTexture), coord + offset01).rg);
    float depth01 = texture2D(sampler2D(depthTexture), coord + offset01).x;
    float dist01 = getLinearDepth(depth01, nearPlane, farPlane);

    vec3 normal11 = decodeNormal(texture2D(sampler2D(normalTexture), coord + offset11).rg);
    float depth11 = texture2D(sampler2D(depthTexture), coord + offset11).x;
    float dist11 = getLinearDepth(depth11, nearPlane, farPlane);

    vec3 normalGradient = (abs(normal10 - normal00) + abs(normal11 - normal01) + abs(normal01 - normal00) + abs(normal11 - normal10));
    float depthGradient = (abs(dist10 - dist00) + abs(dist11 - dist01) + abs(dist01 - dist00) + abs(dist11 - dist10));

    float f0 = dot(normalGradient, normalGradient);// * 0.1;
    float f1 = depthGradient * depthGradient * NDotV2 * NDotV2;
    return f0 + f1;
}

void calculateLowResolutionFrame() {
    const ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 frameSize = imageSize(frameTexture);

    if (pixelCoord.x >= frameSize.x || pixelCoord.y >= frameSize.y) {
        return;
    }

    const vec2 invFrameSize = 1.0 / vec2(frameSize);
    vec2 pixelPos = vec2(pixelCoord) * invFrameSize;

    vec2 seed = pixelPos;
    pixelPos += nextRandomVec2(seed) * invFrameSize; // jitter sample

    vec4 finalColour = vec4(0.0);

    SurfacePoint surface = readSurfacePoint(pixelPos);
    
    if (surface.exists) {
        vec3 colour;
        calculatePathTracedLighting(colour, surface, seed);

        if (!cameraMoved)
            finalColour = imageLoad(frameTexture, pixelCoord).rgba;

        finalColour += vec4(colour, 1.0);
    }

    imageStore(frameTexture, pixelCoord, finalColour);
}

void calculateFullResolutionFrame() {
    const ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 frameSize = imageSize(frameTexture);

    if (pixelCoord.x >= frameSize.x || pixelCoord.y >= frameSize.y) {
        return;
    }
    
    const vec2 invFrameSize = 1.0 / vec2(frameSize);
    vec2 pixelPos = vec2(pixelCoord) * invFrameSize;

    float edgeMagnitude = getEdgeMagnitude(pixelPos, invFrameSize, 2.0);
    
    vec4 finalColour = vec4(0.0);

    if (!cameraMoved)
        finalColour = imageLoad(frameTexture, pixelCoord).rgba;

    if (edgeMagnitude > 0.0225) {

        vec2 seed = pixelPos;
        pixelPos += nextRandomVec2(seed) * invFrameSize; // jitter sample
        SurfacePoint surface = readSurfacePoint(pixelPos);

        if (surface.exists) {
            vec3 colour;
            calculatePathTracedLighting(colour, surface, seed);

            finalColour += vec4(colour, 1.0);
        }
    } else {
        finalColour += texture2D(lowResolutionFrame, pixelPos);
    }

    imageStore(frameTexture, pixelCoord, finalColour);
}

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;
void main(void) {
    if (lowResolutionFramePass) {
        calculateLowResolutionFrame();
    } else{
        calculateFullResolutionFrame();
    }
}


// void writePixel(ivec2 coord, vec3 colour, bool surfaceExists) {
//     vec4 finalColour = vec4(0.0);

//     if (surfaceExists) {
//         if (!cameraMoved)
//             finalColour = imageLoad(frame, coord).rgba;

//         finalColour += vec4(colour, 1.0);
//     }

//     imageStore(frame, coord, finalColour);
// }


// layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;
// void main() {
//     const ivec2 workgroupCoord = ivec2(gl_WorkGroupID.xy);
//     const ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy) * maxRenderScale;
//     const ivec2 frameSize = imageSize(frame);

//     if (pixelCoord.x >= frameSize.x || pixelCoord.y >= frameSize.y) return;
//     const vec2 invFrameSize = 1.0 / vec2(frameSize);
//     // float edgeMagnitude = clamp(getEdgeMagnitude(vec2(pixelCoord) * invFrameSize, vec2(maxRenderScale * 2) * invFrameSize) * 0.1, 0.0, 1.0);

//     int sampleSize = 1;//1 << int(log2(mix(float(maxRenderScale), 1.0, edgeMagnitude)));
//     int sampleCount = maxRenderScale / sampleSize;
    
//     if (lowResolutionFramePass) {
//         for (int i = 0; i < sampleCount; ++i) {
//             for (int j = 0; j < sampleCount; ++j) {
//                 ivec2 sampleCoord = pixelCoord + ivec2(i, j) * sampleSize;
//                 vec2 samplePos = sampleCoord * invFrameSize;
//                 vec2 seed = samplePos;
//                 samplePos += nextRandomVec2(seed) * invFrameSize * sampleSize; // jitter sample

//                 vec3 colour;

//                 SurfacePoint surface = readSurfacePoint(samplePos);

//                 if (surface.exists) {
//                     calculatePathTracedLighting(colour, surface, seed);
//                 }

//                 for (int u = 0; u < sampleSize; ++u) {
//                     for (int v = 0; v < sampleSize; ++v) {
//                         writePixel(sampleCoord + ivec2(u, v), colour, surface.exists);
//                     }
//                 }
//             }
//         }
//     } else {
//         imageStore(frame, )
//     }
// }