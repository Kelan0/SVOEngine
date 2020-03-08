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

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;
void main(void) {
    const ivec2 invocation = ivec2(gl_GlobalInvocationID.xy);
    const vec2 textureCoord = (vec2(invocation) + 0.5) / vec2(mapSize);
    const float NDotV = textureCoord.x;
    const float roughness = textureCoord.y;
    const uint sampleCount = 1024u;

    vec3 V;
    V.x = sqrt(1.0 - NDotV * NDotV);
    V.y = 0.0;
    V.z = NDotV;

    vec3 N = vec3(0.0, 0.0, 1.0);

    float a = roughness; // not squared here, the x coordinate is squared when sampling this LUT
    float a2 = a * a;
    float k = a2 * 0.5;

    float scale = 0.0;
    float bias = 0.0;


    for(uint i = 0u; i < sampleCount; ++i) {
        vec2 Xi = Hammersley(i, sampleCount);
        vec3 H = sampleSpecularBRDF(Xi, N, V, a2);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NDotL = max(L.z, 0.0);
        float NDotH = max(H.z, 0.0);
        float VDotH = max(dot(V, H), 0.0);

        if(NDotL > 0.0) {
            float G = GeometrySmith(NDotL, NDotV, k);
            float G_Vis = (G * VDotH) / (NDotH * NDotV);
            float f1 = 1.0 - VDotH;
            float f2 = f1 * f1;
            float Fc = f2 * f2 * f1; // pow(1.0 - VDotH, 5.0);

            scale += (1.0 - Fc) * G_Vis;
            bias += Fc * G_Vis;
        }
    }
    scale /= float(sampleCount);
    bias /= float(sampleCount);

    imageStore(dstImage, invocation, vec4(scale, bias, 0.0, 0.0));
}