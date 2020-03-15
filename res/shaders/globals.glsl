#ifndef GLOBALS_GLSL
#define GLOBALS_GLSL

#define PI 3.1415926535897932384626433832795028841971
#define INV_PI 0.3183098861837906715377675267450287240689
#define TWO_PI 6.2831853071795864769252867665590057683943

#ifndef MAX_LIGHTS
#define MAX_LIGHTS 16
#endif

#ifndef NORMAL_SPHERICAL_ENCODING
#define NORMAL_SPHERICAL_ENCODING 1 // Encode normals as spherical coordinates
#endif

// ONLY GUARANTEED FOR OPENGL 440+
#ifndef INFINITY
#define INFINITY 1./0.
#endif

#ifndef NAN
#define NAN 0./0.
#endif

uniform vec4 randomInterval = vec4(0.82971940, 0.92297589, 0.25741126, 0.34263822);

struct Fragment {
    vec3 albedo;
    vec3 transmission;
    vec3 emission;
    vec3 normal;
    float roughness;
    float metalness;
    float ambientOcclusion;
    vec3 irradiance;
    vec3 reflection;
    float depth;

    uint next;
};

struct PackedFragment {
    uint albedoRGB_roughnessR; // rgb8 + r8
    uint transmissionRGB_metalnessR; // rgb8 + r8
    uint normal; // rg16f
    uint irradianceRG; // rg16f
    uint irradianceB_reflectionR; // r16f + r16f
    uint reflectionGB; // rg16f
    float depth; // r32f

    uint next;
};

struct Material {
    vec3 albedo;
    vec3 transmission;
    vec3 emission;
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

    sampler2D alphaMap;
    bool hasAlphaMap;
};

struct PackedMaterial {
    uint albedoTextureLo;
    uint albedoTextureHi;
    
    uint normalTextureLo;
    uint normalTextureHi;
    
    uint roughnessTextureLo;
    uint roughnessTextureHi;
    
    uint metalnessTextureLo;
    uint metalnessTextureHi;
    
    uint ambientOcclusionTextureLo;
    uint ambientOcclusionTextureHi;
    
    uint alphaTextureLo;
    uint alphaTextureHi;

    uint albedoRGBA8;
    uint transmissionRGBA8;
    uint emissionRG16;
    uint emissionB16_roughnessR8_metalnessR8; // TODO: more roughness bits for less metalness bits? 10-bit roughness/6-bit metalness?
    uint flags;
    uint padding0;
    uint padding1;
    uint padding2;
};

struct SurfacePoint {
    bool exists;
    bool transparent;
    bool emissive;
    bool invalidNormal;

    // Local properties
    vec3 position;
    vec3 normal;
    float depth;

    // Material properties
    vec3 albedo;
    vec3 transmission;
    vec3 emission;
    float metalness;
    float roughness;
    float ambientOcclusion;
    vec3 irradiance;
    vec3 reflection;
};

struct Ray {
    vec3 origin;
    vec3 direction;
    vec3 inverseDirection;
    uint dominantAxis;
};

struct PointLight {
    vec3 position;
    vec3 colour;
    vec3 attenuation;
    float radius;

    bool hasShadowMap;
    samplerCube shadowMapTexture;
};

struct DirectionLight {
    vec3 direction;
    vec3 colour;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 colour;
    vec3 attenuation;
    float radius;
    float innerRadius;
    float outerRadius;
};

// no vec3s for alignment
struct RawVertex {
   float px, py, pz;    // position
   float nx, ny, nz;    // normal
   float bx, by, bz;    // tangent
   float tx, ty;        // texture
   int m;               // material index
};

struct Vertex {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec2 texture;
    int material;
};

struct Triangle {
    uint i0, i1, i2;
};

struct AxisAlignedBB {
    vec3 minBound;
    vec3 maxBound;
};

#if NORMAL_SPHERICAL_ENCODING == 1

vec2 encodeNormal(vec3 normal, float scale = 1.0) {
    return (vec2(atan(normal.y, normal.x) / PI, normal.z) + 1.0) * 0.5;
}

vec3 decodeNormal(vec2 enc, float scale = 1.0) {
    vec2 ang = enc * 2.0 - 1.0;
    float rad = ang.x * PI;
    float sinTheta = sin(rad);
    float cosTheta = cos(rad);
    float sinPhi = sqrt(1.0 - ang.y * ang.y);
    float cosPhi = ang.y;
    return vec3(cosTheta * sinPhi, sinTheta * sinPhi, cosPhi);
}

#else

// Stereographic Projection - https://aras-p.info/texts/CompactNormalStorage.html
vec2 encodeNormal(vec3 n, float scale = 1.777777777) {
    vec2 enc = n.xy / (n.z + 1.0);
    enc /= scale;
    enc = enc * 0.5 + 0.5;
    return enc;
}

vec3 decodeNormal(vec2 enc, float scale = 1.777777777) {
    vec3 nn = vec3(enc.xy, 0.0) * vec3(2.0 * scale, 2.0 * scale, 0.0) + vec3(-scale, -scale, 1.0);
    float g = 2.0 / dot(nn.xyz, nn.xyz);
    vec3 n;
    n.xy = g * nn.xy;
    n.z = g - 1.0;
    return n;
}

#endif


Fragment unpackFragment(PackedFragment packedFragment) {
    vec4 data;
    Fragment fragment;
    
    data = unpackUnorm4x8(packedFragment.albedoRGB_roughnessR);
    fragment.albedo = data.xyz;
    fragment.roughness = data.w;

    data = unpackUnorm4x8(packedFragment.transmissionRGB_metalnessR);
    fragment.transmission = data.xyz;
    fragment.metalness = data.w;

    fragment.normal = decodeNormal(unpackUnorm2x16(packedFragment.normal).xy);

    fragment.irradiance.rg = unpackUnorm2x16(packedFragment.irradianceRG).rg;

    data.xy = unpackUnorm2x16(packedFragment.irradianceB_reflectionR);
    fragment.irradiance.b = data.x;
    fragment.reflection.r = data.y;

    data.xy = unpackUnorm2x16(packedFragment.reflectionGB);
    fragment.reflection.gb = data.xy;

    fragment.emission = vec3(0.0);

    fragment.depth = packedFragment.depth;

    return fragment;
}

PackedFragment packFragment(Fragment fragment) {
    PackedFragment packedFragment;
    
    packedFragment.albedoRGB_roughnessR = packUnorm4x8(vec4(fragment.albedo, fragment.roughness));

    packedFragment.transmissionRGB_metalnessR = packUnorm4x8(vec4(fragment.transmission, fragment.metalness));

    packedFragment.normal = packUnorm2x16(encodeNormal(fragment.normal));

    packedFragment.irradianceRG = packUnorm2x16(fragment.irradiance.rg);
    packedFragment.irradianceB_reflectionR = packUnorm2x16(vec2(fragment.irradiance.b, fragment.reflection.r));
    packedFragment.reflectionGB = packUnorm2x16(fragment.reflection.gb);

    packedFragment.depth = fragment.depth;

    return packedFragment;
}

Material unpackMaterial(PackedMaterial packedMaterial) {
    Material material;
    
    vec4 v;

    v.xyzw = unpackUnorm4x8(packedMaterial.albedoRGBA8);
    material.albedo = v.rgb;

    v.xyzw = unpackUnorm4x8(packedMaterial.transmissionRGBA8);
    material.transmission = v.rgb;

    v.xy = unpackUnorm2x16(packedMaterial.emissionRG16);
    v.zw = unpackUnorm2x16(packedMaterial.emissionB16_roughnessR8_metalnessR8);
    material.emission = v.rgb * 256.0;

    v.xyzw = unpackUnorm4x8(packedMaterial.emissionB16_roughnessR8_metalnessR8);
    material.roughness = v.z;
    material.metalness = v.w;

    material.albedoMap = sampler2D(uvec2(packedMaterial.albedoTextureLo, packedMaterial.albedoTextureHi));
    material.hasAlbedoMap = bool(packedMaterial.flags & (1 << 0));

    material.normalMap = sampler2D(uvec2(packedMaterial.normalTextureLo, packedMaterial.normalTextureHi));
    material.hasNormalMap = bool(packedMaterial.flags & (1 << 1));

    material.roughnessMap = sampler2D(uvec2(packedMaterial.roughnessTextureLo, packedMaterial.roughnessTextureHi));
    material.hasRoughnessMap = bool(packedMaterial.flags & (1 << 2));
    material.roughnessInverted = bool(packedMaterial.flags & (1 << 7));

    material.metalnessMap = sampler2D(uvec2(packedMaterial.metalnessTextureLo, packedMaterial.metalnessTextureHi));
    material.hasMetalnessMap = bool(packedMaterial.flags & (1 << 3));

    material.ambientOcclusionMap = sampler2D(uvec2(packedMaterial.ambientOcclusionTextureLo, packedMaterial.ambientOcclusionTextureHi));
    material.hasAmbientOcclusionMap = bool(packedMaterial.flags & (1 << 4));

    material.alphaMap = sampler2D(uvec2(packedMaterial.alphaTextureLo, packedMaterial.alphaTextureHi));
    material.hasAlphaMap = bool(packedMaterial.flags & (1 << 5));

    return material;
}

// unprojectWorldPosition
vec3 depthToWorldPosition(float depth, vec2 coord, mat4 invViewProjectionMatrix) {
    vec4 point;
    point.xy = coord * 2.0 - 1.0;
    point.z = depth * 2.0 - 1.0;
    point.w = 1.0;
    point = invViewProjectionMatrix * point;
    return point.xyz / point.w;
}

vec3 projectWorldPosition(vec3 worldPosition, mat4 viewProjectionMatrix) {
    vec4 point = viewProjectionMatrix * vec4(worldPosition, 1.0);
    return (point.xyz / point.w) * 0.5 + 0.5;
}

SurfacePoint packedFragmentToSurfacePoint(PackedFragment packedFragment, vec2 coord, mat4 invViewProjectionMatrix) {
    vec4 data;
    SurfacePoint point;

    data = unpackUnorm4x8(packedFragment.albedoRGB_roughnessR);
    point.albedo = data.xyz;
    point.roughness = data.w;

    data = unpackUnorm4x8(packedFragment.transmissionRGB_metalnessR);
    point.transmission = data.xyz;
    point.metalness = data.w;

    point.normal = decodeNormal(unpackUnorm2x16(packedFragment.normal).xy);

    point.irradiance.rg = unpackUnorm2x16(packedFragment.irradianceRG).rg;

    data.xy = unpackUnorm2x16(packedFragment.irradianceB_reflectionR);
    point.irradiance.b = data.x;
    point.reflection.r = data.y;

    data.xy = unpackUnorm2x16(packedFragment.reflectionGB);
    point.reflection.gb = data.xy;

    point.depth = packedFragment.depth;


    point.position = depthToWorldPosition(point.depth, coord, invViewProjectionMatrix);

    const float eps = 1.0 / 256.0;
    point.exists = point.depth > 0.0 && point.depth < 1.0;
    point.transparent = point.transmission.r > eps || point.transmission.g > eps || point.transmission.b > eps;
    point.emissive = point.emission.r > eps || point.emission.g > eps || point.emission.b > eps;
    point.invalidNormal = false;//isinf(point.normal.x + point.normal.y + point.normal.z) || isnan(point.normal.x + point.normal.y + point.normal.z);
    point.ambientOcclusion = 1.0;

    return point;
}

SurfacePoint fragmentToSurfacePoint(Fragment fragment, vec2 coord, mat4 invViewProjectionMatrix) {
    SurfacePoint point;

    point.depth = fragment.depth;
    point.normal = fragment.normal;
    point.position = depthToWorldPosition(point.depth, coord, invViewProjectionMatrix);

    point.albedo = fragment.albedo;
    point.transmission = fragment.transmission;
    point.emission = fragment.emission;
    point.metalness = fragment.metalness;
    point.roughness = fragment.roughness;
    point.irradiance = fragment.irradiance;
    point.reflection = fragment.reflection;

    const float eps = 1.0 / 255.0;
    point.exists = point.depth > 0.0 && point.depth < 1.0;
    point.transparent = point.transmission.r > eps || point.transmission.g > eps || point.transmission.b > eps;
    point.emissive = point.emission.r > eps || point.emission.g > eps || point.emission.b > eps;
    point.invalidNormal = false;//isinf(point.normal.x + point.normal.y + point.normal.z) || isnan(point.normal.x + point.normal.y + point.normal.z);
    point.ambientOcclusion = 1.0;
    
    return point;
}

SurfacePoint fragmentToSurfacePoint(Fragment fragment, Ray ray) {
    SurfacePoint point;

    point.depth = fragment.depth;
    point.normal = fragment.normal;
    point.position = ray.origin + ray.direction * fragment.depth;

    point.albedo = fragment.albedo;
    point.transmission = fragment.transmission;
    point.emission = fragment.emission;
    point.metalness = fragment.metalness;
    point.roughness = fragment.roughness;
    point.irradiance = fragment.irradiance;
    point.reflection = fragment.reflection;

    const float eps = 1.0 / 255.0;
    point.exists = point.depth > 0.0 && point.depth < 1.0;
    point.transparent = point.transmission.r > eps || point.transmission.g > eps || point.transmission.b > eps;
    point.emissive = point.emission.r > eps || point.emission.g > eps || point.emission.b > eps;
    point.invalidNormal = false;//isinf(point.normal.x + point.normal.y + point.normal.z) || isnan(point.normal.x + point.normal.y + point.normal.z);
    point.ambientOcclusion = 1.0;
    
    return point;
}

float getLinearDepth(float depth, float nearPlane, float farPlane) {
    return 2.0 * nearPlane * farPlane / (farPlane + nearPlane - (depth * 2.0 - 1.0) * (farPlane - nearPlane));
}

vec3 calculateNormalMap(in vec2 texture, in sampler2D normalMap, in vec3 surfaceTangent, in vec3 surfaceNormal) {
    vec3 mappedNormal = texture2D(normalMap, texture).xyz * 2.0 - 1.0;
    //vec3 mappedNormal = texelFetch(normalMap, ivec2(fract(texture) * textureSize(normalMap, 0)), 0).xyz * 2.0 - 1.0;

    // TODO: handle BUMP map better
    bool grayscale = abs(mappedNormal.r - mappedNormal.g) < 1e-4 && abs(mappedNormal.r - mappedNormal.b) < 1e-4; // r g b all same value.
    if (grayscale) { // normal is grayscale bump map
        return surfaceNormal;
    }

    vec3 surfaceBitangent = cross(surfaceTangent, surfaceNormal);
    mat3 tbn = mat3(surfaceTangent, surfaceBitangent, surfaceNormal);

    return (tbn * mappedNormal);
}

Fragment calculateFragment(vec3 position, vec3 normal, vec3 tangent, vec2 texture, float depth, bool hasTangent) {
    Fragment fragment;
    fragment.albedo = vec3(1.0);
    fragment.transmission = vec3(0.0);
    fragment.emission = vec3(0.0);
    fragment.normal = normalize(normal);
    fragment.roughness = 1.0;
    fragment.metalness = 0.0;
    fragment.ambientOcclusion = 1.0;
    fragment.transmission = vec3(0.0);
    fragment.irradiance = vec3(0.0);
    fragment.reflection = vec3(0.0);
    fragment.depth = depth;
    fragment.next = 0xFFFFFFFF;

    return fragment;
}

Fragment calculateFragment(Material material, vec3 position, vec3 normal, vec3 tangent, vec2 texture, float depth, bool hasTangent, bool hasMaterial) {
    Fragment fragment = calculateFragment(position, normal, tangent, texture, depth, hasTangent);
    
    if (hasMaterial) {
        fragment.albedo = material.albedo;
        fragment.transmission = material.transmission;
        fragment.emission = material.emission;
        float surfaceAlpha = 1.0;

        if (material.hasAlbedoMap) {
            vec4 albedoColour = texture2D(material.albedoMap, texture).rgba;
            fragment.albedo *= albedoColour.rgb;
            surfaceAlpha *= albedoColour.a;
        }
        if (material.hasAlphaMap) {
            surfaceAlpha *= texture2D(material.alphaMap, texture).r;
        }

        fragment.transmission = 1.0 - ((1.0 - fragment.transmission) * surfaceAlpha);

        //if (lightingEnabled) {
            fragment.roughness = material.roughness;
            fragment.metalness = material.metalness;
            
            if (material.hasNormalMap) 
                fragment.normal = calculateNormalMap(texture, material.normalMap, normalize(tangent), fragment.normal);
            
            if (material.hasRoughnessMap)
                fragment.roughness = texture2D(material.roughnessMap, texture).r;
            
            if (material.hasMetalnessMap)
                fragment.metalness = texture2D(material.metalnessMap, texture).r;

            if (material.hasAmbientOcclusionMap)
                fragment.ambientOcclusion = texture2D(material.ambientOcclusionMap, texture).r;

            if (material.hasRoughnessMap && material.roughnessInverted)
                fragment.roughness = 1.0 - fragment.roughness;
        //}
    }

    return fragment;
}

Ray createRay(vec3 origin, vec3 direction) {
    Ray ray;
    ray.origin = origin;
    ray.direction = direction;
    ray.inverseDirection = 1.0 / direction;
    
    vec3 v = abs(direction);
    if (v.x > v.y && v.x > v.z)
        ray.dominantAxis = 0;
    else if (v.y > v.z)
        ray.dominantAxis = 1;
    else
        ray.dominantAxis = 2;

    return ray;
}

Ray createRay(vec2 screenPos, vec3 cameraPos, mat4x3 cameraRays) {
    vec3 v00 = cameraRays[0];
    vec3 v10 = cameraRays[1];
    vec3 v11 = cameraRays[2];
    vec3 v01 = cameraRays[3];
    vec3 vy0 = mix(v00, v10, screenPos.x);
    vec3 vy1 = mix(v01, v11, screenPos.x);
    vec3 vxy = mix(vy0, vy1, screenPos.y);
    return createRay(cameraPos, normalize(vxy));
}

vec3 heatmap(float norm) {
    // // black -> magenta -> yellow -> white
    // const uint gradientCount = 4;
    // vec3 colours[gradientCount] = vec3[](vec3(0, 0, 0), vec3(1, 0, 1), vec3(1, 1, 0), vec3(1, 1, 1));

    // // blue -> cyan -> green -> yellow -> red
    // const uint gradientCount = 5;
    // vec3 colours[gradientCount] = vec3[](vec3(0, 0, 1), vec3(0, 1, 1), vec3(0, 1, 0), vec3(1, 1, 0), vec3(1, 0, 0));

    // // black -> blue -> cyan -> green -> yellow -> red -> white
    // const uint gradientCount = 7;
    // vec3 colours[gradientCount] = vec3[](vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 1, 1), vec3(0, 1, 0), vec3(1, 1, 0), vec3(1, 0, 0), vec3(1, 1, 1));

    // float f = clamp(norm, 0.0, 1.0) * (gradientCount - 1);
    // uint a = uint(f);
    // uint b = uint(f + 1);
    // f -= a;
    // // r = colours[a][0] * (1 - f) + colours[b][0] * f;
    // return colours[a] * (1 - f) + colours[b] * f;

    float f = 2.0 * clamp(norm, 0.0, 1.0);
    float r = max(0.0, f - 1.0);
    float b = max(0.0, 1.0 - f);
    float g = max(0.0, 1.0 - b - r);
    return vec3(r, g, b) * (1.0 - max(0.0, norm - 1.0));
}



uint hash1u(uint x) {
    x += x >> 11;
    x ^= x << 7;
    x += x >> 15;
    x ^= x << 5;
    x += x >> 12;
    x ^= x << 9;
    return x;
}

uint hash2u(uint x, uint y) {
    x += x >> 11;
    x ^= x << 7;
    x += y;
    x ^= x << 6;
    x += x >> 15;
    x ^= x << 5;
    x += x >> 12;
    x ^= x << 9;
    return x;
}

uint hash3u(uint x, uint y, uint z) {
    x += x >> 11;
    x ^= x << 7;
    x += y;
    x ^= x << 3;
    x += z ^ ( x >> 14 );
    x ^= x << 6;
    x += x >> 15;
    x ^= x << 5;
    x += x >> 12;
    x ^= x << 9;
    return x;
}

// Uniformly distributed random float between 0 and 1
float random(vec2 seed) {
    uvec2 u = floatBitsToUint(seed);
    return uintBitsToFloat((hash2u(u.x, u.y) & 0x007FFFFFu) | 0x3F800000u) - 1.0;
}

float nextRandom(inout vec2 seed) {
    seed -= randomInterval.xy;
    return random(seed);
}

vec2 nextRandomVec2(inout vec2 seed) {
    return vec2(nextRandom(seed), nextRandom(seed));
}

vec3 nextRandomVec3(inout vec2 seed) {
    return vec3(nextRandom(seed), nextRandom(seed), nextRandom(seed));
}

vec4 nextRandomVec4(inout vec2 seed) {
    return vec4(nextRandom(seed), nextRandom(seed), nextRandom(seed), nextRandom(seed));
}

// The following two functions are from
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // 0x100000000
 }
 
vec2 Hammersley(uint i, uint n) {
     return vec2(float(i) / float(n), RadicalInverse_VdC(i));
 }

vec3 getTriangleWorldPoint(vec2 Xi, vec3 v0, vec3 v1, vec3 v2) {
    float u = Xi.x;
    float v = Xi.y;
 
    if (u + v >= 1.0) {
        u = 1.0 - u;
        v = 1.0 - v;
    }
 
    return v0 + (((v1 - v0) * u) + ((v2 - v0) * v));
}

vec3 getRandomBarycentricCoord(vec2 seed) {
    float u = nextRandom(seed);
    float v = nextRandom(seed);

    if (u + v >= 1.0) {
        u = 1.0 - u;
        v = 1.0 - v;
    }

    return vec3(1.0 - u - v, u, v);
}

vec3 getRandomTrianglePoint(inout vec2 seed, vec3 v0, vec3 v1, vec3 v2) {
    return getTriangleWorldPoint(nextRandomVec2(seed), v0, v1, v2);
}

float getTriangleSurfaceArea(vec3 v0, vec3 v1, vec3 v2) {
    return 0.5 * length(cross(v1 - v0, v2 - v0));
}
#endif