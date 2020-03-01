#include "globals.glsl"
#include "lighting.glsl"
#include "UE4BRDF.glsl"
#include "raytrace/raytrace.glsl"
#include "raytrace/intersection.glsl"

#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 32
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 32
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

layout(binding = 0, rgba32f) uniform image2D frame;

SurfacePoint readSurfacePoint(vec2 coord) {
    Fragment frag;
    frag.depth = texture(depthTexture, coord).x;
    frag.albedo = texture(albedoTexture, coord).rgb;
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

void calculateNextSurfacePoint(in Ray ray, in float sampleDistance, in vec3 sampleBarycentric, in uint sampleTriangleIndex, inout SurfacePoint surface) {
    vec3 hitPoint = ray.origin + ray.direction * sampleDistance;

    Vertex v0, v1, v2;
    getTriangleVertices(sampleTriangleIndex, v0, v1, v2);

    vec3 position = mat3(v0.position, v1.position, v2.position) * sampleBarycentric;
    vec3 normal = normalize(mat3(v0.normal, v1.normal, v2.normal) * sampleBarycentric);

    surface.depth = sampleDistance;
    surface.normal = normal;
    surface.position = position;

    // surface.albedo = fragment.albedo;
    // surface.transmission = fragment.transmission;
    // surface.metalness = fragment.metalness;
    // surface.roughness = fragment.roughness;
    // surface.irradiance = fragment.irradiance;
    // surface.reflection = fragment.reflection;

    // const float eps = 1.0 / 255.0;
    surface.exists = true;
    surface.transparent = false;//surface.transmission.x > eps || surface.transmission.y > eps || surface.transmission.z > eps;
    surface.invalidNormal = false;//isinf(surface.normal.x + surface.normal.y + surface.normal.z) || isnan(surface.normal.x + surface.normal.y + surface.normal.z);
    surface.ambientOcclusion = 1.0;
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

    for (int i = 0; i < 1; i++) {
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

        Le = vec3(0.0);//currentSurface.emission;
        Fr = evaluateBRDF(wo, wi, currentSurface);
        Li = calculateDirectLight(wo, wi, currentSurface);

        radiance += (Le + Fr * Li) * energy;
        energy *= Fr;

        // calculateNextSurfacePoint(sampleRay, intersection.dist, intersection.barycentric, intersection.triangleIndex, currentSurface);
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

    finalColour = frag.albedo;
    return true;
}

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;
void main() {
    const ivec2 workgroupCoord = ivec2(gl_WorkGroupID.xy);
    const ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 frameSize = imageSize(frame);

    if (pixelCoord.x >= frameSize.x || pixelCoord.y >= frameSize.y) return;

    vec2 screenPos = vec2(pixelCoord) / vec2(frameSize);
    vec2 seed = screenPos;

    // SurfacePoint surface = readSurfacePoint(screenPos);

    vec4 finalColour = imageLoad(frame, pixelCoord).rgba;

    if (pathTracePrimaryRays(finalColour.rgb, screenPos)) {
        finalColour.a = 1.0;
    } else {
        finalColour.a = 0.0;
    }

    // if (surface.exists) {
    //     vec4 currColour = vec4(0.0);
    //     // calculateRaytracedLighting(finalColour.rgb, surface, pointLights, pointLightCount, BRDFIntegrationMap, cameraPosition, imageBasedLightingEnabled);
    //     calculatePathTracedLighting(currColour.rgb, surface, seed);
    //     if (cameraMoved) {
    //         finalColour = vec4(0.0);
    //     }

    //     finalColour.rgb += currColour.rgb;
    //     finalColour.a += 1.0;
    // } else {
    //     finalColour = vec4(0.0);
    // }

    imageStore(frame, pixelCoord, vec4(finalColour));
}