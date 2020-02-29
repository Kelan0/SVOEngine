#define PI 3.14159265359

const float cosTheta = cos(PI * 0.25); // 45 deg
const float sinTheta = sin(PI * 0.25); 
const mat3 horizontalFilter = mat3( 
     1.0, 2.0, 1.0, 
     0.0, 0.0, 0.0, 
    -1.0, -2.0, -1.0 
);
const mat3 verticalFilter = mat3( 
     1.0, 0.0, -1.0, 
     2.0, 0.0, -2.0, 
     1.0, 0.0, -1.0 
);
    
in vec3 fs_worldPosition;
in vec3 fs_worldNormal;

uniform ivec2 screenSize;
uniform sampler2D depthTexture;
uniform sampler2D normalTexture;
uniform vec3 cameraPosition;
uniform float nearPlane;
uniform float farPlane;

out vec4 outColour;

float getDepth(vec2 coord) {
    return texture2D(depthTexture, coord / vec2(screenSize)).r;
}

float getLinearDepth(float depth) {
    return 2.0 * nearPlane * farPlane / (farPlane + nearPlane - (depth * 2.0 - 1.0) * (farPlane - nearPlane));
}

vec3 getNormal(vec2 coord) {
    return texture2D(normalTexture, coord / vec2(screenSize)).xyz * 2.0 - 1.0;
}

float detectEdge(mat3 kernel) {
    float gx = dot(horizontalFilter[0], kernel[0]) + dot(horizontalFilter[1], kernel[1]) + dot(horizontalFilter[2], kernel[2]); 
    float gy = dot(verticalFilter[0], kernel[0]) + dot(verticalFilter[1], kernel[1]) + dot(verticalFilter[2], kernel[2]);
    return sqrt(gx * gx + gy * gy);
}

float detectEdge(vec2 coord) {
    vec3 normal = getNormal(coord);

    mat3 depthKernel, normalKernel;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            vec2 p = coord + vec2(i - 1.0, j - 1.0);
            depthKernel[i][j] = getLinearDepth(getDepth(p)); // * 1.0 - max(0.0, dot(normal, getNormal(p))) // ?
            normalKernel[i][j] = dot(normal, getNormal(p));
        }
    }

    float depthEdge = detectEdge(depthKernel) + 0.03;
    float normalEdge = detectEdge(normalKernel) + 0.03;// + 0.3 * (1.0 - min(depthEdge, 1.0));

    return depthEdge * normalEdge;
}

void main() {
    float fragDepth = getLinearDepth(gl_FragCoord.z);
    float bufferDepth = getLinearDepth(getDepth(gl_FragCoord.xy));
    if (fragDepth > bufferDepth + 0.001)
        discard;

    if (detectEdge(gl_FragCoord.xy) >= 1.0) {
        outColour = vec4(0.1, 0.5, 0.3, 1.0);
        return;
    }

    vec2 fragCoord = mat2(cosTheta, sinTheta, -sinTheta, cosTheta) * (gl_FragCoord.xy + screenSize);
    ivec2 pixelCoord = ivec2(fragCoord * 0.5);

    ivec2 md = (pixelCoord % 2);// + 2) % 2;
    if (md.x == 0 && md.y == 0) {
        outColour = vec4(0.2, 1.0, 0.6, 1.0);
    } else {
        discard;
    }
}