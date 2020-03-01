layout(triangles) in;
layout(triangle_strip, max_vertices = 9) out;

in VertexData {
    vec3 worldPosition;
    vec3 worldNormal;
    vec3 worldTangent;
    vec2 vertexTexture;
    float projectedDepth;
    flat int hasTangent;
    flat int materialIndex;
} gs_in[];

out VertexData {
    vec3 worldPosition;
    vec3 worldNormal;
    vec3 worldTangent;
    vec2 vertexTexture;
    float projectedDepth;
    flat int hasTangent;
    flat int materialIndex;
} gs_out;

void main() {
    // for (int layer = 0; layer < 3; layer++) {
        gl_Layer = 0;//layer;

        for (int i = 0; i < 3; i++) {
            gs_out.worldPosition = gs_in[i].worldPosition;
            gs_out.worldNormal = gs_in[i].worldNormal;
            gs_out.worldTangent = gs_in[i].worldTangent;
            gs_out.vertexTexture = gs_in[i].vertexTexture;
            gs_out.projectedDepth = gs_in[i].projectedDepth;
            gs_out.hasTangent = gs_in[i].hasTangent;
            gs_out.materialIndex = gs_in[i].materialIndex;
            gl_Position = gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    // }
}