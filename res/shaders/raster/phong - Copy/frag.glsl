struct Material {
    vec3 albedo;
    float roughness;
    float metalness;

    sampler2D albedoMap;
    bool hasAlbedoMap;

    sampler2D normalMap;
    bool hasNormalMap;

    sampler2D roughnessMap;
    bool hasRoughnessMap;
    bool roughnessInverted;

    sampler2D metalnessMap;
    bool hasMetalnessMap;

    sampler2D ambientOcclusionMap;
    bool hasAmbientOcclusionMap;
};

in vec3 fs_worldPosition;
in vec3 fs_worldNormal;
in vec3 fs_worldTangent;
in vec2 fs_vertexTexture;
in flat int fs_hasTangent;

uniform bool hasMaterial;
uniform Material material;

uniform bool lightingEnabled;
uniform bool imageBasedLightingEnabled;
uniform bool hasLightProbe;
uniform samplerCube diffuseIrradianceTexture;
uniform samplerCube specularReflectionTexture;
uniform vec3 cameraPosition;

out vec3 outAlbedo;
out vec3 outNormal;
out float outRoughness;
out float outMetalness;
out float outAmbientOcclusion;
out vec3 outIrradiance;
out vec3 outReflection;

vec3 calculateNormalMap(sampler2D normalMap, vec3 surfaceTangent, vec3 surfaceNormal) {
    if (fs_hasTangent == 0) {
        return surfaceNormal;
    }
    
    vec3 mappedNormal = texture2D(material.normalMap, fs_vertexTexture).xyz * 2.0 - 1.0;

    bool grayscale = abs(mappedNormal.r - mappedNormal.g) < 1e-4 && abs(mappedNormal.r - mappedNormal.b) < 1e-4; // r g b all same value.
    if (grayscale) { // normal is grayscale bump map
        return surfaceNormal;
    }

    vec3 surfaceBitangent = cross(surfaceTangent, surfaceNormal);
    mat3 tbn = mat3(surfaceTangent, surfaceBitangent, surfaceNormal);

    return (tbn * mappedNormal);
}

void main() {
    const float maxReflectionMip = 5.0;

    vec3 surfaceAlbedo = vec3(1.0);
    vec3 surfaceNormal = normalize(fs_worldNormal);
    vec3 surfaceTangent = normalize(fs_worldTangent);
    float surfaceRoughness = 1.0;
    float surfaceMetalness = 0.0;
    float surfaceAmbientOcclusion = 1.0;
    vec3 surfaceIrradiance = vec3(0.0);
    vec3 surfaceReflection = vec3(0.0);

    if (hasMaterial) {
        surfaceAlbedo = material.albedo;
        if (material.hasAlbedoMap) {
            vec4 albedoColour = texture2D(material.albedoMap, fs_vertexTexture).rgba;
            if (albedoColour.a < 0.5) discard;
            surfaceAlbedo *= albedoColour.rgb;
        }

        //if (lightingEnabled) {
            surfaceRoughness = material.roughness;
            surfaceMetalness = material.metalness;
            if (material.hasNormalMap) surfaceNormal = calculateNormalMap(material.normalMap, surfaceTangent, surfaceNormal);
            if (material.hasRoughnessMap) surfaceRoughness = texture2D(material.roughnessMap, fs_vertexTexture).r;
            if (material.hasMetalnessMap) surfaceMetalness = texture2D(material.metalnessMap, fs_vertexTexture).r;
            if (material.hasAmbientOcclusionMap) surfaceAmbientOcclusion = texture2D(material.ambientOcclusionMap, fs_vertexTexture).r;

            if (material.roughnessInverted) surfaceRoughness = 1.0 - surfaceRoughness;
        //}
    }

    if (/*lightingEnabled && */imageBasedLightingEnabled && hasLightProbe) {
        vec3 surfaceToCamera = cameraPosition - fs_worldPosition;
        vec3 viewReflectionDir = reflect(-surfaceToCamera, surfaceNormal); 
        surfaceIrradiance = texture(diffuseIrradianceTexture, surfaceNormal).rgb;
        surfaceReflection = textureLod(specularReflectionTexture, viewReflectionDir, surfaceRoughness * maxReflectionMip).rgb;
    }

    outAlbedo = surfaceAlbedo;
    outNormal = surfaceNormal;
    outRoughness = surfaceRoughness;
    outMetalness = surfaceMetalness;
    outAmbientOcclusion = surfaceAmbientOcclusion;
    outIrradiance = surfaceIrradiance;
    outReflection = surfaceReflection;
}