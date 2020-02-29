in vec3 vs_vertexPosition;

uniform mat4 modelMatrix;
uniform mat4 viewProjectionMatrix;

out vec3 fs_worldPosition;

void main() {
    vec4 worldPosition = modelMatrix * vec4(vs_vertexPosition, 1.0);
    fs_worldPosition = worldPosition.xyz;

    gl_Position = viewProjectionMatrix * worldPosition;
}