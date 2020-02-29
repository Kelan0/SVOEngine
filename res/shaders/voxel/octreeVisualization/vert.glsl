out VertexData {
    vec2 vertexPosition;
    vec2 vertexTexture;
} vs_out;


void main() {
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);

    vs_out.vertexPosition.x = x;
    vs_out.vertexPosition.y = y;
    vs_out.vertexTexture.x = 0.0 + ((x + 1.0) * 0.5);
    vs_out.vertexTexture.y = 0.0 + ((y + 1.0) * 0.5);

    gl_Position = vec4(x, y, 0.0, 1.0);
}