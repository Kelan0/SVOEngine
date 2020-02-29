#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 8
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 8
#endif

#ifndef LOCAL_SIZE_Z
#define LOCAL_SIZE_Z 1
#endif

#include "globals.glsl"
#include "lighting.glsl"
#include "UE4BRDF.glsl"

layout(binding = 0, rgba32f) uniform writeonly image2D dstImage;
uniform ivec2 mapSize;

vec3 calculateImportanceSampleGGX(vec2 sequence, vec3 normal, float roughness) {
    float a = roughness * roughness;
	float a2 = a * a;
    float phi = 2.0 * PI * sequence.x;
    const float eps = 1.0 - 1e-3;

    float cosTheta = sqrt((1.0 - sequence.y) / (1.0 + (a2 - 1.0) * sequence.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
    float cosPhi = cos(phi);
    float sinPhi = sin(phi);
	
    vec3 H = vec3(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta); // convert spherical coords to tangent space
    vec3 up = abs(normal.z) < eps ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    mat3 tbn;
    tbn[0] = normalize(cross(up, normal));
    tbn[1] = cross(normal, tbn[0]);
    tbn[2] = normal;
	
    vec3 sampleVec = tbn * H;//tangent * H.x + bitangent * H.y + normal * H.z;
    return normalize(sampleVec);
}

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;
void main(void) {
    const ivec2 invocation = ivec2(gl_GlobalInvocationID.xy);
    const vec2 textureCoord = (vec2(invocation) + 0.5) / vec2(mapSize);
    const float nDotV = textureCoord.x;
    const float roughness = textureCoord.y;
    const uint sampleCount = 1024u;

    const vec3 V = vec3(sqrt(1.0 - nDotV * nDotV), 0.0, nDotV);

    float scale = 0.0;
    float bias = 0.0;

    vec3 normal = vec3(0.0, 0.0, 1.0);

    for(uint i = 0u; i < sampleCount; ++i) {
        vec2 sequence = Hammersley(i, sampleCount);
        vec3 H = calculateImportanceSampleGGX(sequence, normal, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float nDotL = max(L.z, 0.0);
        float nDotH = max(H.z, 0.0);
        float vDotH = max(dot(V, H), 0.0);

        if(nDotL > 0.0) {
            float G = calculateGeometrySmith(nDotV, nDotL, roughness, false);
            float G_Vis = (G * vDotH) / (nDotH * nDotV);
            float Fc = pow(1.0 - vDotH, 5.0);

            scale += (1.0 - Fc) * G_Vis;
            bias += Fc * G_Vis;
        }
    }
    scale /= float(sampleCount);
    bias /= float(sampleCount);

    imageStore(dstImage, invocation, vec4(scale, bias, 0.0, 0.0));
}