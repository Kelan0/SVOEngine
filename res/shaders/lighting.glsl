#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#ifndef DIELECTRIC_BASE_REFLECTIVITY
#define DIELECTRIC_BASE_REFLECTIVITY vec3(0.04)
#endif

#include "globals.glsl"
#include "UE4BRDF.glsl"


struct ShadingContext {
    PointLight pointLights[MAX_LIGHTS];
    int pointLightCount;


};

struct SurfaceLights {
    PointLight pointLights[MAX_LIGHTS];
    int pointLightCount;
};

float getAttenuation(float lightDist, float quadratic = 1.0, float linear = 0.0, float constant = 1.0) {
    return 1.0 / (quadratic * lightDist * lightDist + linear * lightDist + constant);
}

vec3 calculateShadowOcclusion(in PointLight light, in vec3 surfaceToLight, in float lightDist, in float nDotL) {
    vec3 occlusion = vec3(1.0);

    if (light.hasShadowMap) {
        float shadowDepth = texture(light.shadowMapTexture, -surfaceToLight).r;
        float shadowDistance = 1.0 / (1.0 - shadowDepth) - 1.0;
        float bias = clamp(0.005 * tan(acos(nDotL)), 0.0, 0.01);
        if (lightDist - bias > shadowDistance) {
            occlusion = vec3(0.0);
        }
    }

    return occlusion;
}

void calculatePointLight(inout vec3 outgoingRadiance, in PointLight light, in SurfacePoint surface, in vec3 surfaceToCamera, in float nDotV, in vec3 baseReflectivity, in bool imageBasedLightingEnabled) {
    vec3 lightPosition = light.position;
    vec3 lightColour = light.colour;
    vec3 lightAttenuation = light.attenuation;
    float lightRadius = light.radius;

    vec3 surfaceToLight = lightPosition - surface.position;
    float lightDist = dot(surfaceToLight, surfaceToLight);
    if (lightDist < lightRadius * lightRadius) {
        lightDist = sqrt(lightDist);
        surfaceToLight /= lightDist;

        vec3 wo = surfaceToCamera;
        vec3 wi = surfaceToLight;

        float attenuation = getAttenuation(lightDist, lightAttenuation[0], lightAttenuation[1], lightAttenuation[2]);

        vec3 Li = lightColour * attenuation;// * shadowFactor;
        
        // if Li is not zero

        vec3 Fr = evaluateBRDF(wo, wi, surface, true);

        outgoingRadiance += /*Le*/ + Fr * Li * dot(wi, surface.normal);

        // vec3 halfVector = normalize(surfaceToCamera + surfaceToLight);

        // vec3 incomingRadiance = lightColour * attenuation;

        // float nDotL = max(dot(surface.normal, surfaceToLight), 0.0);
        // float nDotH = max(dot(surface.normal, halfVector), 0.0);
        // float hDotV = max(dot(halfVector, surfaceToCamera), 0.0);

        // if (nDotL <= 0.0) return;

        // vec3 shadowOcclusion = calculateShadowOcclusion(light, surfaceToLight, lightDist, nDotL);
        // vec3 contributedColour = shadowOcclusion * incomingRadiance * nDotL;

        // if (contributedColour.r <= 0.001 && contributedColour.g <= 0.001 && contributedColour.b < 0.001) {
        //     // light contributed to surface is too dark to make a difference, don't bother with BRDF
        //     return;
        // }

        // vec3 fresnel = calculateFresnelSchlick(hDotV, baseReflectivity);
        // float normalDistribution = calculateNormalDistributionGGX(nDotH, surface.roughness);
        // float geometryDistribution = calculateGeometrySmith(nDotV, nDotL, surface.roughness, imageBasedLightingEnabled);

        // vec3 DFG = normalDistribution * geometryDistribution * fresnel;
        // vec3 specular = DFG / max(4.0 * nDotV * nDotL, 0.0001);

        // vec3 kS = fresnel;
        // vec3 kD = (1.0 - kS) * (1.0 - surface.metalness);
        // outgoingRadiance += max((kD * surface.albedo / PI + specular) * contributedColour, 0.0);
    }
}

void calculateImageBasedLighting(inout vec3 finalColour, in SurfacePoint surface, in sampler2D BRDFIntegrationMap, in vec3 outgoingRadiance, in vec3 surfaceToCamera, in float nDotV, in vec3 baseReflectivity) {
    const float maxReflectionMip = 5.0;
    float a = surface.roughness * surface.roughness;
    
    vec3 f0 = baseReflectivity;
    vec3 f90 = vec3(1.0 - a);
    vec3 F = FresnelSchlick(nDotV, f0, f90);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - surface.metalness);
    vec2 BRDF = texture2D(BRDFIntegrationMap, vec2(nDotV, a)).rg;

    vec3 diffuse = surface.irradiance * surface.albedo * PI;
    vec3 specular = surface.reflection * (kS * BRDF[0] + BRDF[1]);

    finalColour = (kD * diffuse + specular) + outgoingRadiance;
}

void calculateAmbientLighting(inout vec3 finalColour, in SurfacePoint surface, in vec3 outgoingRadiance) {
    vec3 ambient = vec3(0.001) * surface.albedo * surface.ambientOcclusion;
    finalColour = ambient + outgoingRadiance;
}

void calculateLighting(inout vec3 finalColour, in SurfacePoint surface, in PointLight pointLights[MAX_LIGHTS], in int pointLightCount, in sampler2D BRDFIntegrationMap, in vec3 cameraPosition, in bool imageBasedLightingEnabled) {
    if (surface.exists) {
        vec3 surfaceToCamera = cameraPosition - surface.position;
        float cameraDist = length(surfaceToCamera);
        surfaceToCamera /= cameraDist;

        float nDotV = clamp(dot(surface.normal, surfaceToCamera), 0.0, 1.0);

        vec3 baseReflectivity = mix(DIELECTRIC_BASE_REFLECTIVITY, surface.albedo, surface.metalness);
        vec3 outgoingRadiance = vec3(0.0);
        for (int i = 0; i < pointLightCount; i++) {
            calculatePointLight(outgoingRadiance, pointLights[i], surface, surfaceToCamera, nDotV, baseReflectivity, imageBasedLightingEnabled);
        }

        if (imageBasedLightingEnabled) {
            calculateImageBasedLighting(finalColour, surface, BRDFIntegrationMap, outgoingRadiance, surfaceToCamera, nDotV, baseReflectivity);
        } else {
            calculateAmbientLighting(finalColour, surface, outgoingRadiance);
        }
    }
}

#endif