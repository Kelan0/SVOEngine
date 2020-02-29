#include "globals.glsl"
#include "lighting.glsl"

float NormalDistributionGGX(float NDotH2, float a2) {
    float f = NDotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * f * f);
}

float GeometrySchlickGGX(float cosTheta, float k) {
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

float GeometrySmith(float NDotL, float NDotV, float k) {
    // if analytic light source, k = (roughness+1)/2
    // if integrated light source, k = ((roughness+1)^2)/8

    // optimise by inlining GeometrySchlickGGX
    float gl = GeometrySchlickGGX(NDotL, k);
    float gv = GeometrySchlickGGX(NDotV, k);
    return gl * gv;
}

vec3 FresnelSchlick(float VDotH, vec3 f0, vec3 f90) {
    float f = 1.0 - VDotH;
    float f2 = f * f; // squared
    return f0 + (f90 - f0) * (f2 * f2 * f); // pow(f, 5) vs 3 multiplications?
}

vec3 orientToNormal(vec3 v, vec3 N) {
    vec3 W = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 U = cross(W, N);
    vec3 V = cross(N, U);
    return v.x * U + v.y * V + v.z * N;
}

float sampleDiffusePDF(vec3 wo, vec3 wi, vec3 N, float a) {
    vec3 L = wi;
    return dot(L, N) * INV_PI;
}

float sampleSpecularPDF(vec3 wo, vec3 wi, vec3 N, float a) {
    vec3 V = wo;
    vec3 L = wi;
    vec3 H = normalize(N + L); // optimise

    float NDotH = dot(N, H);
    float VDotH = dot(V, H);

    float D = NormalDistributionGGX(NDotH, a);

    return (D * NDotH) / (4.0 * VDotH);
}

vec3 sampleDiffuseBRDF(vec2 Xi, vec3 N) {
    float phi = 2.0 * PI * Xi.x;
    float theta = 0.5 * PI * Xi.y;
    float cosPhi = cos(phi);
    float sinPhi = sin(phi);
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    
    vec3 v;
    v.x = sinTheta * cosPhi;
    v.y = sinTheta * sinPhi;
    v.z = cosTheta;
    return orientToNormal(v, N);
}

// Returns the importance-sampled half-vector around which to reflect the incident light direction
vec3 sampleSpecularBRDF(vec2 Xi, vec3 N, vec3 V, float a2) {
    float phi = 2.0 * PI * Xi.x;
    float cosPhi = cos(phi);
    float sinPhi = sin(phi);
    float cosTheta = sqrt((1.0 - Xi.y) / (Xi.y * (a2 - 1.0) + 1.0)); // how is this derived???
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H;
    H.x = sinTheta * cosPhi;
    H.y = sinTheta * sinPhi;
    H.z = cosTheta;
    H = orientToNormal(H, N);
    return normalize(2.0 * dot(V, H) * H - V); // reflect
}

vec3 sampleBRDF(vec3 wo, SurfacePoint surface, vec2 seed) {
    float diffuseRatio = 0.5 * (1.0 - surface.metalness);

    vec2 Xi = nextRandomVec2(seed);

    float a = surface.roughness * surface.roughness;
    float a2 = a * a;

    if (nextRandom(seed) < diffuseRatio) {
        return sampleDiffuseBRDF(Xi, surface.normal);
    } else {
        return sampleSpecularBRDF(Xi, surface.normal, wo, a2);
    }
}

// Evaluates the BRDF for the given surface for the outgoing direction (wo) and incoming direction (wi)
vec3 evaluateBRDF(vec3 wo, vec3 wi, SurfacePoint surface, bool analyticLightSource = false) {
    const vec3 N = surface.normal;
    const vec3 V = wo;
    const vec3 L = wi;

    float NDotV = dot(N, V);
    float NDotL = dot(N, L);
    if (NDotV <= 0.0 || NDotL <= 0.0) {
        return vec3(0.0); // Zero radiance
    }

    const vec3 H = normalize(V + L);
    const float NDotH = dot(H, L);
    const float VDotH = dot(V, H);

    const float NDotH2 = NDotH * NDotH;

    const float a = surface.roughness * surface.roughness;
    const float a2 = a * a;

    float k;

    if (analyticLightSource) {
        k = a + 1.0;
        k = k * k * 0.125;
    } else {
        k = a2 * 0.5;
    }

    vec3 f0 = mix(DIELECTRIC_BASE_REFLECTIVITY, surface.albedo, surface.metalness);
    vec3 f90 = vec3(1.0 - a); //vec3(1.0);

    float D = NormalDistributionGGX(NDotH2, a2);
    float G = GeometrySmith(NDotL, NDotV, k);
    vec3 F = FresnelSchlick(VDotH, f0, f90);

    vec3 DFG = vec3(D * G); // fresnel term multiplied in kS
    vec3 specular = DFG / (4.0 * NDotV * NDotL);
    vec3 diffuse = surface.albedo * INV_PI; // c / pi

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - surface.metalness);

    return kD * diffuse + kS * specular;
}

vec3 evaluateDiffuseBRDF(vec3 wo, vec3 wi, SurfacePoint surface, bool analyticLightSource = false) {
    return surface.albedo * INV_PI;
}