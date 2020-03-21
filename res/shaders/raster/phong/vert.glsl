in vec3 vs_vertexPosition;
in vec3 vs_vertexNormal;
in vec3 vs_vertexTangent;
in vec2 vs_vertexTexture;
in int vs_vertexMaterial;

uniform mat4 modelMatrix;
uniform mat4 viewProjectionMatrix;
uniform mat4 prevModelMatrix;
uniform mat4 prevViewProjectionMatrix;

out VertexData {
    vec3 worldPosition;
    vec3 worldNormal;
    vec3 worldTangent;
    vec2 vertexTexture;
    vec4 projectedPosition;
    vec4 prevProjectedPosition;
    flat int hasTangent;
    flat int materialIndex;
} vs_out;

void main() {
    mat4 normalMatrix = inverse(transpose(modelMatrix));
    vec4 worldPosition = modelMatrix * vec4(vs_vertexPosition, 1.0);
    vec4 worldNormal = normalMatrix * vec4(vs_vertexNormal, 0.0);
    vec4 worldTangent = normalMatrix * vec4(vs_vertexTangent, 0.0);

    vec4 projectedPosition = viewProjectionMatrix * worldPosition;
    vec4 prevProjectedPosition = prevViewProjectionMatrix * worldPosition;

    vs_out.worldPosition = worldPosition.xyz;
    vs_out.worldNormal = worldNormal.xyz;
    vs_out.worldTangent = worldTangent.xyz;
    vs_out.vertexTexture = vs_vertexTexture;
    vs_out.projectedPosition = projectedPosition;
    vs_out.prevProjectedPosition = prevProjectedPosition;
    vs_out.hasTangent = dot(worldTangent.xyz, worldTangent.xyz) > 1e-2 ? 1 : 0; // tangent is non-zero vector
    vs_out.materialIndex = vs_vertexMaterial;

    gl_Position = projectedPosition;
}