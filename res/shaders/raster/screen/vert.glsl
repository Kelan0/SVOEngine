out vec2 fs_vertexPosition;
out vec2 fs_vertexTexture;

void main() {
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);

    fs_vertexPosition.x = x;
    fs_vertexPosition.y = y;
    fs_vertexTexture.x = (x + 1.0) * 0.5;
    fs_vertexTexture.y = (y + 1.0) * 0.5;

    gl_Position = vec4(x, y, 0.0, 1.0);
}