#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#include "globals.glsl"

#ifndef DIELECTRIC_BASE_REFLECTIVITY
#define DIELECTRIC_BASE_REFLECTIVITY vec3(0.04)
#endif

struct SurfaceLights {
    PointLight pointLights[MAX_LIGHTS];
    int pointLightCount;
};

float getAttenuation(float lightDist, float quadratic = 1.0, float linear = 0.0, float constant = 1.0) {
    return 1.0 / (quadratic * lightDist * lightDist + linear * lightDist + constant);
}

#ifdef BASIC_SHADING // SIMPLE PHONG LIGHTING
// TODO ---
vec3 calculatePointLightPhong(in SurfacePoint surface, in PointLight light) {
    vec3 lightDir = light.position - surface.position;
    float lightDist = dot(lightDir, lightDir);

    if (lightDist < light.radius * light.radius) {
        lightDist = sqrt(lightDist);
        lightDir /= lightDist;

        //#ifdef BLINN_PHONG
        //float nDotL = clamp(dot(surface.normal, normalize(surface.normal + lightDir)), 0.0, 1.0); 
        //#else
        float nDotL = max(dot(surface.normal, lightDir), 0.0);
        //#endif

        float attenuation = getAttenuation(lightDist, light.attenuation[0], light.attenuation[1], light.attenuation[2]);
    
        return light.colour * nDotL * attenuation;
    }

    return vec3(0.0);
}

void calculateLighting(in SurfacePoint surface, inout vec3 finalColour) {
    for (int i = 0; i < pointLightCount; i++) {
        finalColour += surface.albedo * calculatePointLightPhong(surface, pointLights[i]);
    }
}



#else // PBR LIGHTING

vec3 calculateFresnelSchlick(float cosTheta, vec3 baseReflectivity) {
    float f = 1.0 - cosTheta;
    float f2 = f * f;
    return baseReflectivity + (1.0 - baseReflectivity) * f2 * f2 * f; //pow(f, 5.0);
}

vec3 calculateFresnelSchlickRoughness(float cosTheta, vec3 baseReflectivity, float roughness) {
    float f = 1.0 - cosTheta;
    float f2 = f * f;
    vec3 t = max(vec3(1.0 - roughness), baseReflectivity);
    return baseReflectivity + (t - baseReflectivity) * f2 * f2 * f; //pow(f, 5.0);
}

float calculateNormalDistributionGGX(float nDotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float f = (nDotH * nDotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * f * f);
}

float calculateGeometrySchlickGGX(float nDotV, float roughness, bool directLighting) {
    float k;

    if (directLighting)
        k = ((roughness + 1.0) * (roughness + 1.0)) / 8.0;
    else
        k = (roughness * roughness) / 2.0;

    return nDotV / (nDotV * (1.0 - k) + k);
}

float calculateGeometrySmith(float nDotV, float nDotL, float roughness, bool directLighting) {
    float ggx2 = calculateGeometrySchlickGGX(nDotV, roughness, directLighting);
    float ggx1 = calculateGeometrySchlickGGX(nDotL, roughness, directLighting);
	
    return ggx1 * ggx2;
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

        vec3 halfVector = normalize(surfaceToCamera + surfaceToLight);

        float attenuation = getAttenuation(lightDist, lightAttenuation[0], lightAttenuation[1], lightAttenuation[2]);
        vec3 incomingRadiance = lightColour * attenuation;

        float nDotL = max(dot(surface.normal, surfaceToLight), 0.0);
        float nDotH = max(dot(surface.normal, halfVector), 0.0);
        float hDotV = max(dot(halfVector, surfaceToCamera), 0.0);

        if (nDotL <= 0.0) return;

        vec3 shadowOcclusion = calculateShadowOcclusion(light, surfaceToLight, lightDist, nDotL);
        vec3 contributedColour = shadowOcclusion * incomingRadiance * nDotL;

        if (contributedColour.r <= 0.001 && contributedColour.g <= 0.001 && contributedColour.b < 0.001) {
            // light contributed to surface is too dark to make a difference, don't bother with BRDF
            return;
        }

        vec3 fresnel = calculateFresnelSchlick(hDotV, baseReflectivity);
        float normalDistribution = calculateNormalDistributionGGX(nDotH, surface.roughness);
        float geometryDistribution = calculateGeometrySmith(nDotV, nDotL, surface.roughness, imageBasedLightingEnabled);

        vec3 DFG = normalDistribution * geometryDistribution * fresnel;
        vec3 specular = DFG / max(4.0 * nDotV * nDotL, 0.0001);

        vec3 kS = fresnel;
        vec3 kD = (1.0 - kS) * (1.0 - surface.metalness);
        outgoingRadiance += max((kD * surface.albedo / PI + specular) * contributedColour, 0.0);
    }
}

void calculateImageBasedLighting(inout vec3 finalColour, in SurfacePoint surface, in sampler2D BRDFIntegrationMap, in vec3 outgoingRadiance, in vec3 surfaceToCamera, in float nDotV, in vec3 baseReflectivity) {
    const float maxReflectionMip = 5.0;
    vec3 viewReflectionDir = reflect(-surfaceToCamera, surface.normal); 
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
        //if (nDotV <= 0.0) return;

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

#endif