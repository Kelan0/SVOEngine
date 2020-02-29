in vec3 fs_worldPosition;

uniform samplerCube environmentMapTexture;

out vec3 outAlbedo;
out vec3 outNormal;
out float outRoughness;
out float outMetalness;
out float outAmbientOcclusion;
out vec3 outIrradiance;
out vec3 outReflection;

void main() {
    vec3 mapDirection = normalize(fs_worldPosition);
    vec3 mapColour = texture(environmentMapTexture, mapDirection).rgb;
    outAlbedo = mapColour;
    outNormal = vec3(0.0);
    outRoughness = 0.0;
    outMetalness = 0.0;
    outAmbientOcclusion = 0.0;
    outIrradiance = vec3(0.0);
    outReflection = vec3(0.0);
}