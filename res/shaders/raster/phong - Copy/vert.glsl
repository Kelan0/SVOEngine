in vec3 vs_vertexPosition;
in vec3 vs_vertexNormal;
in vec3 vs_vertexTangent;
in vec2 vs_vertexTexture;

uniform mat4 modelMatrix;
uniform mat4 viewProjectionMatrix;

out vec3 fs_worldPosition;
out vec3 fs_worldNormal;
out vec3 fs_worldTangent;
out vec2 fs_vertexTexture;
out flat int fs_hasTangent;

void main() {
    mat4 normalMatrix = inverse(transpose(modelMatrix));
    vec4 worldPosition = modelMatrix * vec4(vs_vertexPosition, 1.0);
    vec4 worldNormal = normalMatrix * vec4(vs_vertexNormal, 0.0);
    vec4 worldTangent = normalMatrix * vec4(vs_vertexTangent, 0.0);

    fs_worldPosition = worldPosition.xyz;
    fs_worldNormal = worldNormal.xyz;
    fs_worldTangent = worldTangent.xyz;
    fs_vertexTexture = vs_vertexTexture;
    fs_hasTangent = dot(vs_vertexTangent, vs_vertexTangent) > 1e-2 ? 1 : 0; // tangent is non-zero vector

    gl_Position = viewProjectionMatrix * worldPosition;
}