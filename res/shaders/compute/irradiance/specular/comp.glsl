#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 1
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 1
#endif

#ifndef LOCAL_SIZE_Z
#define LOCAL_SIZE_Z 1
#endif

#define PI 3.14159265359

const vec2 invAtan = vec2(0.15915494309, 0.31830988618);
const vec3 cubeCornerVertices[24] = vec3[](
    vec3(+1.0, -1.0, -1.0), vec3(+1.0, +1.0, -1.0), vec3(+1.0, +1.0, +1.0), vec3(+1.0, -1.0, +1.0), // +x
    vec3(-1.0, -1.0, -1.0), vec3(-1.0, -1.0, +1.0), vec3(-1.0, +1.0, +1.0), vec3(-1.0, +1.0, -1.0), // -x
    vec3(-1.0, +1.0, -1.0), vec3(-1.0, +1.0, +1.0), vec3(+1.0, +1.0, +1.0), vec3(+1.0, +1.0, -1.0), // +y
    vec3(-1.0, -1.0, -1.0), vec3(+1.0, -1.0, -1.0), vec3(+1.0, -1.0, +1.0), vec3(-1.0, -1.0, +1.0), // -y
    vec3(-1.0, -1.0, +1.0), vec3(+1.0, -1.0, +1.0), vec3(+1.0, +1.0, +1.0), vec3(-1.0, +1.0, +1.0), // +z
    vec3(-1.0, -1.0, -1.0), vec3(-1.0, +1.0, -1.0), vec3(+1.0, +1.0, -1.0), vec3(+1.0, -1.0, -1.0)  // -z
);

uniform ivec2 environmentMapSize;
uniform ivec2 irradianceMapSize;

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
    vec2 textureCoord = (vec2(invocation.xy + 0.5)) / vec2(irradianceMapSize);
    
    const vec3 srcSize = vec3(environmentMapSize, 1.0);
    const vec3 dstSize = vec3(irradianceMapSize, 1.0);
    const float dt = (2.0 * PI) / 250.0;

    const vec3 normal = calculateRay(textureCoord, invocation.z);
    const vec3 coord = getCubemapCoordinate(normal);
    const ivec3 srcCoord = ivec3(coord * srcSize);
    const ivec3 dstCoord = ivec3(coord * dstSize);

    vec3 irradianceSum = vec3(0.0);
    float irradianceArea = 0.0;

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, normal);
    up = cross(normal, right);

    for(float phi = 0.0; phi < 2.0 * PI; phi += dt) {
        for(float theta = 0.0; theta < 0.5 * PI; theta += dt) {
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;
            ivec3 sampleCoord = ivec3(getCubemapCoordinate(sampleVec) * srcSize);
            irradianceSum += imageLoad(srcImage, sampleCoord).rgb * cos(theta) * sin(theta);
            irradianceArea += 1.0;
        }
    }
    irradianceSum = PI * irradianceSum * (1.0 / float(irradianceArea));

    imageStore(dstImage, dstCoord, imageLoad(srcImage, srcCoord));
}