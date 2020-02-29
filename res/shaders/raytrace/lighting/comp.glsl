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

SurfacePoint getTriangleSamplePoint(in float dist, in uint triangleIndex, in vec3 barycentric) {
    Vertex v0, v1, v2;
    getTriangleVertices(triangleIndex, v0, v1, v2);

    vec3 position = barycentric[0] * v0.position + barycentric[1] * v1.position + barycentric[2] * v2.position;
    vec3 normal = barycentric[0] * v0.normal + barycentric[1] * v1.normal + barycentric[2] * v2.normal;
    vec3 tangent = barycentric[0] * v0.tangent + barycentric[1] * v1.tangent + barycentric[2] * v2.tangent;
    vec2 texture = barycentric[0] * v0.texture + barycentric[1] * v1.texture + barycentric[2] * v2.texture;
    bool hasTangent = dot(tangent, tangent) > 1e-3;

    Fragment fragment = calculateFragment(position, normal, tangent, texture, 0.0, hasTangent);

    SurfacePoint point;

    point.depth = fragment.depth;
    point.normal = fragment.normal;
    point.position = position;

    point.albedo = fragment.albedo;
    point.transmission = fragment.transmission;
    point.metalness = fragment.metalness;
    point.roughness = fragment.roughness;
    point.irradiance = fragment.irradiance;
    point.reflection = fragment.reflection;

    point.exists = true;
    point.transparent = false;//point.transmission.x > eps || point.transmission.y > eps || point.transmission.z > eps;
    point.invalidNormal = false;//isinf(point.normal.x + point.normal.y + point.normal.z) || isnan(point.normal.x + point.normal.y + point.normal.z);
    point.ambientOcclusion = 1.0;
    
    return point;
}

bool sampleGlobalSurfacePoint(in Ray ray, inout SurfacePoint surface) {
    float dist = INFINITY;
    uint triangleIndex;
    vec3 barycentric;
    if (sampleRayIntersectsBVH(ray, true, dist, barycentric, triangleIndex)) {
        surface = getTriangleSamplePoint(dist, triangleIndex, barycentric);
        return true;
    }
    return false;
}

void calculateIndirectLighting(inout vec3 finalColour, in SurfacePoint surface, in sampler2D BRDFIntegrationMap, in vec3 outgoingRadiance, in vec3 surfaceToCamera, in float nDotV, in vec3 baseReflectivity) {
    const float maxReflectionMip = 5.0;
    vec3 viewReflectionDir = reflect(-surfaceToCamera, surface.normal); 
    // vec3 reflectionColour = raytraceReflection(createRay(surface.position, viewReflectionDir));
    vec3 kS = calculateFresnelSchlickRoughness(nDotV, baseReflectivity, surface.roughness);
    vec3 kD = (1.0 - kS) * (1.0 - surface.metalness);
    vec3 incomingRadiance = surface.irradiance;// texture(skyboxDiffuseIrradianceTexture, surface.normal).rgb; // incoming radiance
    vec3 diffuse = incomingRadiance * surface.albedo;
    vec3 prefilteredColour = surface.reflection;// textureLod(skyboxPrefilteredEnvironmentTexture, viewReflectionDir, surface.roughness * maxReflectionMip).rgb;
    vec2 brdfIntegral = texture2D(BRDFIntegrationMap, vec2(nDotV, surface.roughness)).rg;
    vec3 specular = prefilteredColour * (kS * brdfIntegral.x + brdfIntegral.y);
    vec3 ambient = (kD * diffuse + specular) * surface.ambientOcclusion;
    finalColour = ambient + outgoingRadiance;
}

void calculateRaytracedLighting(inout vec3 finalColour, in SurfacePoint surface, in PointLight pointLights[MAX_LIGHTS], in int pointLightCount, in sampler2D BRDFIntegrationMap, in vec3 cameraPosition, in bool imageBasedLightingEnabled) {
    if (surface.exists) {
        vec3 surfaceToCamera = cameraPosition - surface.position;
        float cameraDist = length(surfaceToCamera);
        surfaceToCamera /= cameraDist;

        float nDotV = clamp(dot(surface.normal, surfaceToCamera), 0.0, 1.0);
        //if (nDotV <= 0.0) return;

        vec3 baseReflectivity = mix(DIELECTRIC_BASE_REFLECTIVITY, surface.albedo, surface.metalness);
        vec3 outgoingRadiance = vec3(0.0);
        for (int i = 0; i < pointLightCount; i++) {
            calculatePointLight(outgoingRadiance, pointLights[i], surface, surfaceToCamera, nDotV, baseReflectivity, imageBasedLightingEnabled);
        }

        calculateIndirectLighting(finalColour, surface, BRDFIntegrationMap, outgoingRadiance, surfaceToCamera, nDotV, baseReflectivity);
    }
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

// vec4 cosineHemisphereSample(in float f0, in float f1) {

// }

// vec3 FresnelSchlick(in float cosTheta, in vec3 f0, in vec3 f90) {
//     float c = 1.0 - cosTheta;
//     float c2 = c * c;
//     return f0 + (f90 - f0) * c2 * c2 * c; // pow(c, 5.0); // is pow faster than 3 multiplications?
// }

// float NormalDistribution_GGX(in float NDotH, in float a, in float a2) {
//     float f = (NDotH * NDotH * (a2 - 1.0) + 1.0);
//     return a2 / (PI * f * f);
// }

// float GeometrySchlick_GGX(in float NDotV, in float k) {
//     return NDotV / (NDotV * (1.0 - k) + k);
// }

// float GeometrySmith(in float NDotV, in float NDotL, in float k) {
//     float gv = GeometrySchlick_GGX(NDotV, k);
//     float gl = GeometrySchlick_GGX(NDotL, k);
//     return gv * gl;
// }

// float pdfBRDF(in Ray ray, in SurfacePoint surface, in vec3 wi) {
//     return 1.0; // TODO
// }

// vec3 sampleBRDF(in vec3 wo, in SurfacePoint surface, inout vec2 seed) {
//     vec3 N = surface.normal;
//     vec3 V = wo;

//     float r1 = nextRandom(seed);
//     float r2 = nextRandom(seed);

//     vec3 Y = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
//     vec3 X = normalize(cross(Y, N));
//     vec3 Z = cross(N, X);

//     if (nextRandom(seed) < 0.5 * (1.0 - surface.metalness)) {
//         vec3 wi = uniformHemisphereSample(surface.normal, seed).xyz;//cosHemisphereSample(r1, r2);
//         return X * wi.x + Y * wi.y + Z * wi.z;
//     } else {
//         float a = surface.roughness;
//         float phi = r1 * TWO_PI;
//         float cosTheta = sqrt((1.0 - r2) / (1.0 + (a * a - 1.0) * r2));
//         float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
//         float sinPhi = sin(phi);
//         float cosPhi = cos(phi);

//         vec3 H = X * sinTheta * cosPhi + Y * sinTheta * sinPhi + Z * cosTheta;
//         return reflect(V, H);
//     }
// }

// vec3 evalBRDF(in vec3 wo, in SurfacePoint surface, in vec3 wi) {
//     vec3 N = surface.normal;
//     vec3 V = wo; // reflected direction towards eye (wo)
//     vec3 L = wi; // incident direction from light (wi)

//     float NDotL = dot(N, L);
//     if (NDotL <= 0.0)
//         return vec3(0.0);
    
//     float NDotV = dot(N, V);
//     if (NDotV <= 0.0)
//         return vec3(0.0);

//     vec3 H = normalize(L + V);
//     float NDotH = dot(N, H);
//     float LDotH = dot(L, H);
    
//     float m = surface.metalness;
//     float m2 = m * m;
//     float a = surface.roughness;
//     float a2 = a * a;
//     float k = a * 0.5 + 0.5;
//     float k2 = k * k;
    
//     // if (directLighting) // illumination by analytic light sourtce
//     //     k = ((a + 1.0) * (a + 1.0)) / 8.0;
//     // else
//     //     k = a2 / 2.0

//     vec3 f0 = mix(DIELECTRIC_BASE_REFLECTIVITY, surface.albedo, surface.metalness);
//     // vec3 f0 = 0.16 * reflectance * reflectance * (1.0 - m) + baseColor * m;
//     vec3 f90 = vec3(1.0 - a); //vec3(1.0);

//     float D = NormalDistribution_GGX(NDotH, a, a2);
//     float G = GeometrySmith(NDotV, NDotL, k2);
//     vec3 F = FresnelSchlick(LDotH, f0, f90);
//     vec3 DFG = (D * G) * F;

//     vec3 kS = F;
//     vec3 kD = (1.0 - kS) * (1.0 - m);

//     vec3 diffuse = surface.albedo * INV_PI;
//     vec3 specular = DFG / (4.0 * NDotV * NDotL);

//     return kD * diffuse + kS * specular;
// }

// void calculatePathTracedLighting(inout vec3 finalColour, in SurfacePoint surface, in vec2 seed) {
//     vec3 surfaceToCamera = cameraPosition - surface.position;
//     float cameraDist = length(surfaceToCamera);
//     surfaceToCamera /= cameraDist;

//     float nDotV = clamp(dot(surface.normal, surfaceToCamera), 0.0, 1.0);
//     vec3 baseReflectivity = mix(DIELECTRIC_BASE_REFLECTIVITY, surface.albedo, surface.metalness);
//     vec3 outgoingRadiance = vec3(0.0);

//     // int pointLightSampleOffset = int(nextRandom(seed) * pointLightCount);
//     // int pointLightSampleCount = min(1, pointLightCount);

//     // for (int i = 0; i < pointLightSampleCount; ++i) {
//     //     vec3 lightOffset = uniformSphereSample(seed) * 0.08;
//     //     int lightIndex = (i + pointLightSampleOffset) % pointLightCount;
//     //     vec3 surfaceToLight = (pointLights[lightIndex].position + lightOffset) - surface.position;
//     //     float lightDist = length(surfaceToLight);
//     //     surfaceToLight /= lightDist;
//     //     float dist = lightDist;
//     //     Ray lightRay = createRay(surface.position, surfaceToLight);
//     //     if (!occlusionRayIntersectsBVH(lightRay, true, dist)) {
//     //         calculatePointLight(outgoingRadiance, pointLights[lightIndex], surface, surfaceToCamera, nDotV, baseReflectivity, imageBasedLightingEnabled);
//     //     }
//     // }

//     vec3 outgoingRadiance = vec3(0.0);

//     float ambientOcclusion = 0.0;
//     uint aoSampleCount = 1;//cameraMoved ? 1 : 16;
//     float aoSampleIncr = 1.0 / float(aoSampleCount);

//     for (int i = 0; i < aoSampleCount; i++) {
//         float dist = INFINITY;
//         Ray sampleRay = createRay(surface.position, uniformHemisphereSample(surface.normal, seed).xyz);
//         if (occlusionRayIntersectsBVH(sampleRay, true, dist)) {
//             ambientOcclusion += aoSampleIncr;
//         }
//     }
    
//     vec4 sampleDirection = uniformHemisphereSample(surface.normal, seed);
//     Ray sampleRay = createRay(surface.position, sampleDirection.xyz);

//     float dist = INFINITY;
//     if (!occlusionRayIntersectsBVH(sampleRay, true, dist)) {
//         // No hit, sample sky
//         outgoingRadiance += texture(skyboxEnvironmentTexture, sampleDirection.xyz).rgb * sampleDirection.w * sin(acos(cosTheta));
//     } else {

//     }


//     // vec3 incomingRadiance = texture(skyboxDiffuseIrradianceTexture, surface.normal).rgb;

//     // vec3 kS = calculateFresnelSchlickRoughness(nDotV, baseReflectivity, surface.roughness);
//     // vec3 kD = (1.0 - kS) * (1.0 - surface.metalness);
//     // vec3 diffuse = incomingRadiance * surface.albedo;
//     // vec3 prefilteredColour = surface.reflection;
//     // vec2 brdfIntegral = texture2D(BRDFIntegrationMap, vec2(nDotV, surface.roughness)).rg;
//     // vec3 specular = prefilteredColour * (kS * brdfIntegral.x + brdfIntegral.y);
//     // vec3 ambient = (kD * diffuse + specular) * (1.0 - ambientOcclusion);
//     // finalColour = ambient + outgoingRadiance;



//     // vec3 wo = normalize(cameraPosition - surface.position);
//     // vec3 wi = sampleBRDF(wo, surface, seed);

//     // Ray r1 = createRay(surface.position, wi);

//     // vec3 incomingRadiance = vec3(0.0);

//     // float dist = INFINITY;
//     // // if (!occlusionRayIntersectsBVH(r1, true, dist)) {
//     //     incomingRadiance = texture(skyboxEnvironmentTexture, r1.direction).rgb * PI;
//     // // }

//     // // vec3 ambient = evalBRDF(wo, surface, wi);

//     // finalColour = incomingRadiance;//ambient * (1.0 - ambientOcclusion);

//     // // calculateIndirectLighting(finalColour, surface, BRDFIntegrationMap, outgoingRadiance, surfaceToCamera, nDotV, baseReflectivity);

// }



void calculatePathTracedLighting(inout vec3 finalColour, in SurfacePoint surface, in vec2 seed) {
    // float ambientOcclusion = 0.0;
    // uint aoSampleCount = 1;//cameraMoved ? 1 : 16;
    // float aoSampleIncr = 1.0 / float(aoSampleCount);

    // for (int i = 0; i < aoSampleCount; i++) {
    //     float dist = INFINITY;
    //     Ray sampleRay = createRay(surface.position, uniformHemisphereSample(surface.normal, seed).xyz);
    //     if (occlusionRayIntersectsBVH(sampleRay, true, dist)) {
    //         ambientOcclusion += aoSampleIncr;
    //     }
    // }

    vec3 surfaceToCamera = cameraPosition - surface.position;
    float cameraDist = length(surfaceToCamera);
    surfaceToCamera /= cameraDist;

    vec3 wo = surfaceToCamera;
    vec3 wi = sampleBRDF(wo, surface, seed);

    Ray sampleRay = createRay(surface.position + wi * 1e-5, wi);
    float sampleDist = INFINITY;
    vec3 barycentric;
    uint triangleIndex;
    // if (!sampleRayIntersectsBVH(sampleRay, false, sampleDist, barycentric, triangleIndex)) {
    if (occlusionRayIntersectsBVH(sampleRay, true, sampleDist)) {
        return;
    }

    vec3 Fr = evaluateBRDF(wo, wi, surface);
    vec3 Li = texture(skyboxEnvironmentTexture, wi).rgb * PI;
    finalColour += Fr * Li * dot(wi, surface.normal);
}

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;
void main() {
    const ivec2 workgroupCoord = ivec2(gl_WorkGroupID.xy);
    const ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 frameSize = imageSize(frame);

    if (pixelCoord.x >= frameSize.x || pixelCoord.y >= frameSize.y) return;

    vec2 screenPos = vec2(pixelCoord) / vec2(frameSize);

    // vec2 seed = cameraMoved ? screenPos : vec2(workgroupCoord.xy) / vec2(gl_NumWorkGroups.xy);
    vec2 seed = screenPos;

    SurfacePoint surface = readSurfacePoint(screenPos);
    // vec2 seed;
    // seed.x = atan(-surface.normal.x, surface.normal.y);
    // seed.x = (seed.x + PI / 2.0) / (PI * 2.0) + PI * (28.670 / 360.0);
    // seed.y = acos(surface.normal.z) / PI;
    // seed += vec2(workgroupCoord.xy) / vec2(gl_NumWorkGroups.xy);
    // if (cameraMoved) seed = screenPos;

    vec4 finalColour = imageLoad(frame, pixelCoord).rgba;

    if (surface.exists) {
        vec4 currColour = vec4(0.0);
        // calculateRaytracedLighting(finalColour.rgb, surface, pointLights, pointLightCount, BRDFIntegrationMap, cameraPosition, imageBasedLightingEnabled);
        calculatePathTracedLighting(currColour.rgb, surface, seed);
        if (cameraMoved) {
            finalColour = vec4(0.0);
        }

        finalColour.rgb += currColour.rgb;
        finalColour.a += 1.0;
    } else {
        finalColour = vec4(0.0);
    }

    imageStore(frame, pixelCoord, vec4(finalColour));
}