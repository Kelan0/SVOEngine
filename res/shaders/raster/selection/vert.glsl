in vec3 vs_vertexPosition;
in vec3 vs_vertexNormal;

uniform mat4 modelMatrix;
uniform mat4 viewProjectionMatrix;

out vec3 fs_worldPosition;
out vec3 fs_worldNormal;

void main() {
    mat4 normalMatrix = inverse(transpose(modelMatrix));
    vec4 worldPosition = modelMatrix * vec4(vs_vertexPosition, 1.0);
    vec4 worldNormal = normalMatrix * vec4(vs_vertexNormal, 0.0);
    
    fs_worldPosition = worldPosition.xyz;
    fs_worldNormal = worldNormal.xyz;

    gl_Position = viewProjectionMatrix * worldPosition;
}