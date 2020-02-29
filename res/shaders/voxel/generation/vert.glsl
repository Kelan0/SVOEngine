in vec3 vs_vertexPosition;
in vec3 vs_vertexNormal;
in vec3 vs_vertexTangent;
in vec2 vs_vertexTexture;

uniform mat4 modelMatrix;
uniform mat4 viewProjectionMatrix;

out VertexData {
    vec3 worldPosition;
    vec3 worldNormal;
    vec3 worldTangent;
    vec2 vertexTexture;
    flat int hasTangent;
} vs_out;

void main() {
    mat4 normalMatrix = inverse(transpose(modelMatrix));
    vec4 worldPosition = modelMatrix * vec4(vs_vertexPosition, 1.0);
    vec4 worldNormal = normalMatrix * vec4(vs_vertexNormal, 0.0);
    vec4 worldTangent = normalMatrix * vec4(vs_vertexTangent, 0.0);

    vs_out.worldPosition = worldPosition.xyz;
    vs_out.worldNormal = worldNormal.xyz;
    vs_out.worldTangent = worldTangent.xyz;
    vs_out.vertexTexture = vs_vertexTexture;
    vs_out.hasTangent = dot(vs_vertexTangent, vs_vertexTangent) > 1e-2 ? 1 : 0; // tangent is non-zero vector

    gl_Position = worldPosition;
}