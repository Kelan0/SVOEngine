layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

uniform mat4 modelMatrix;
uniform mat4 viewProjectionMatrixDirections[6];
uniform vec3 cameraPosition;

out float fs_worldDistance;

void renderLayer(int layer) {
    gl_Layer = layer;
    
    // u = (1 - 1 / (x + 1))
    // (1 - u) = (1 / (x + 1))
    // 1 / (1 - u) = x + 1
    // 1 / (1 - u) - 1 = x
    fs_worldDistance = length(cameraPosition - gl_in[0].gl_Position.xyz);
    gl_Position = viewProjectionMatrixDirections[layer] * gl_in[0].gl_Position;
    EmitVertex();
    fs_worldDistance = length(cameraPosition - gl_in[1].gl_Position.xyz);
    gl_Position = viewProjectionMatrixDirections[layer] * gl_in[1].gl_Position;
    EmitVertex();
    fs_worldDistance = length(cameraPosition - gl_in[2].gl_Position.xyz);
    gl_Position = viewProjectionMatrixDirections[layer] * gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
}

void main() {
    renderLayer(0);
    renderLayer(1);
    renderLayer(2);
    renderLayer(3);
    renderLayer(4);
    renderLayer(5);
}