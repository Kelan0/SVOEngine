in vec3 vs_vertexPosition;

uniform mat4 modelMatrix;

void main() {
    gl_Position = modelMatrix * vec4(vs_vertexPosition, 1.0);
}