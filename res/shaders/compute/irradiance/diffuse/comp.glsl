#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 1
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 1
#endif

#ifndef LOCAL_SIZE_Z
#define LOCAL_SIZE_Z 1
#endif

#include "globals.glsl"
#include "lighting.glsl"

const vec2 invAtan = vec2(0.15915494309, 0.31830988618);
const vec3 cubeCornerVertices[24] = vec3[](
    vec3(+1.0, -1.0, -1.0), vec3(+1.0, +1.0, -1.0), vec3(+1.0, +1.0, +1.0), vec3(+1.0, -1.0, +1.0), // +x
    vec3(-1.0, -1.0, -1.0), vec3(-1.0, -1.0, +1.0), vec3(-1.0, +1.0, +1.0), vec3(-1.0, +1.0, -1.0), // -x
    vec3(-1.0, +1.0, -1.0), vec3(-1.0, +1.0, +1.0), vec3(+1.0, +1.0, +1.0), vec3(+1.0, +1.0, -1.0), // +y
    vec3(-1.0, -1.0, -1.0), vec3(+1.0, -1.0, -1.0), vec3(+1.0, -1.0, +1.0), vec3(-1.0, -1.0, +1.0), // -y
    vec3(-1.0, -1.0, +1.0), vec3(+1.0, -1.0, +1.0), vec3(+1.0, +1.0, +1.0), vec3(-1.0, +1.0, +1.0), // +z
    vec3(-1.0, -1.0, -1.0), vec3(-1.0, +1.0, -1.0), vec3(+1.0, +1.0, -1.0), vec3(+1.0, -1.0, -1.0)  // -z
);

uniform ivec2 srcMapSize;
uniform ivec2 dstMapSize;

layout(binding = 0, rgba32f) uniform readonly imageCube srcImage;
layout(binding = 1, rgba32f) uniform writeonly imageCube dstImage;

vec3 getCubemapCoordinate(vec3 ray) {
    vec3 absRay = abs(ray);
    float sc, tc, ma;
    int layer;

    if (absRay.x > absRay.y && absRay.x > absRay.z) { // major axis = x
        ma = absRay.x;
        if (ray.x > 0) 
            sc = -ray.z, tc = -ray.y, layer = 0; // positive x
        else 
            sc = ray.z, tc = -ray.y, layer = 1; // negative x
    } else if (absRay.y > absRay.z) { // major axis = y
        ma = absRay.y;
        if (ray.y > 0) 
            sc = ray.x, tc = ray.z, layer = 2; // positive y
        else 
            sc = ray.x, tc = -ray.z, layer = 3; // negative y
    } else { // major axis = z
        ma = absRay.z;
        if (ray.z > 0) 
            sc = ray.x, tc = -ray.y, layer = 4; // positive z
        else 
            sc = -ray.x, tc = -ray.y, layer = 5; // negative z
    }

    float s = (sc / ma) * 0.5 + 0.5;
    float t = (tc / ma) * 0.5 + 0.5;

    return vec3(s, t, layer);
}

vec3 calculateRay(vec2 textureCoord, int face) {
    vec3 v00 = cubeCornerVertices[face * 4 + 0];
    vec3 v10 = cubeCornerVertices[face * 4 + 1];
    vec3 v11 = cubeCornerVertices[face * 4 + 2];
    vec3 v01 = cubeCornerVertices[face * 4 + 3];
    vec3 vy0 = mix(v00, v10, textureCoord.x);
    vec3 vy1 = mix(v01, v11, textureCoord.x);
    return normalize(mix(vy0, vy1, textureCoord.y)); // normalized interpolated coordinate on unit cube
}

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;
void main(void) {
    const ivec3 invocation = ivec3(gl_GlobalInvocationID.xyz);
    vec2 textureCoord = (vec2(invocation.xy + 0.5)) / vec2(dstMapSize);
    
    const vec3 srcSize = vec3(srcMapSize, 1.0);
    const vec3 dstSize = vec3(dstMapSize, 1.0);
    const float dt = (1.0 / 250.0);// * (2.0 * PI);

    const vec3 normal = calculateRay(textureCoord, invocation.z);
    const vec3 coord = getCubemapCoordinate(normal);
    const ivec3 srcCoord = ivec3(coord * srcSize);
    const ivec3 dstCoord = ivec3(coord * dstSize);

    vec3 irradianceSum = vec3(0.0);
    float irradianceArea = 0.0;

    SurfacePoint surface;
    surface.normal = normal;

    vec3 wo = vec3(0.0);
    vec2 Xi;
    for (Xi.x = 0.0; Xi.x <= 1.0; Xi.x += dt) {
        for (Xi.y = 0.0; Xi.y <= 1.0; Xi.y += dt) {
            vec3 wi = sampleDiffuseBRDF(Xi, normal);
            ivec3 sampleCoord = ivec3(getCubemapCoordinate(wi) * srcSize);
            surface.albedo = imageLoad(srcImage, sampleCoord).rgb;

            vec3 irradiance = evaluateDiffuseBRDF(wo, wi, surface);
            float pdf = sampleDiffusePDF(wo, wi, normal, 1.0);
            irradianceSum += irradiance * pdf;
            irradianceArea += 1.0;
        }
    }
    irradianceSum = PI * irradianceSum / float(irradianceArea);

    imageStore(dstImage, dstCoord, vec4(irradianceSum, 1.0));
    //imageStore(dstImage, dstCoord, imageLoad(srcImage, srcCoord));
}