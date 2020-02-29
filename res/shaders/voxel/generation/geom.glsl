layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in VertexData {
    vec3 worldPosition;
    vec3 worldNormal;
    vec3 worldTangent;
    vec2 vertexTexture;
    flat int hasTangent;
} gs_in[];

out VertexData {
    // vec3 projectedPosition;
    vec3 worldPosition;
    vec3 worldNormal;
    vec3 worldTangent;
    vec2 vertexTexture;
    flat int hasTangent;
    flat int dominantAxis;
    // vec4 aabb; // xy=min, zw=max
} gs_out;

uniform mat4 axisProjections[3];
uniform float voxelGridScale;
uniform vec3 voxelGridCenter;
uniform int voxelGridSize;

bool testTriangleBounds(vec3 p0, vec3 p1, vec3 p2) {
    float r = voxelGridSize * voxelGridScale * 0.5;
    vec3 t0 = min(p0, min(p1, p2));
    vec3 t1 = max(p0, max(p1, p2));
    vec3 g0 = voxelGridCenter - r;
    vec3 g1 = voxelGridCenter + r;

    if (t1.x < g0.x || t0.x > g1.x) return false;
    if (t1.y < g0.y || t0.y > g1.y) return false;
    if (t1.z < g0.z || t0.z > g1.z) return false;

    return true; // return true only when triangle is inside the voxel grid bounds.
}

void main() {
    vec3 p0 = gs_in[0].worldPosition;
    vec3 p1 = gs_in[1].worldPosition;
    vec3 p2 = gs_in[2].worldPosition;

    if (!testTriangleBounds(p0, p1, p2)) {
        return;
    }

    vec3 norm = cross(p1 - p0, p2 - p0);
    vec3 absNorm = abs(norm);

    int dominantAxis = absNorm[0] > absNorm[1] ? 0 : 1;
    dominantAxis = absNorm[2] > absNorm[dominantAxis] ? 2 : dominantAxis;
    //if (dominantAxis != 2) return;

    mat4 viewProjectionMatrix = axisProjections[dominantAxis];

    vec4 projectedPosition[3];
    projectedPosition[0] = viewProjectionMatrix * vec4(p0, 1.0);
    projectedPosition[1] = viewProjectionMatrix * vec4(p1, 1.0);
    projectedPosition[2] = viewProjectionMatrix * vec4(p2, 1.0);
    // vec2 ps = 1.0 / voxelGridSize.xy;
    // float pl = 1.41421 / voxelGridSize.x;
    // vec2 hPixel = ps * 0.5;

    // vec4 aabb = vec4(0.0);
    // aabb.xy = min(projectedPosition[0].xy, min(projectedPosition[1].xy, projectedPosition[2].xy));
    // aabb.zw = max(projectedPosition[0].xy, max(projectedPosition[1].xy, projectedPosition[2].xy));

    // vec3 e01 = vec3(projectedPosition[1].xy - projectedPosition[0].xy, 0.0);
    // vec3 e12 = vec3(projectedPosition[2].xy - projectedPosition[1].xy, 0.0);
    // vec3 e20 = vec3(projectedPosition[0].xy - projectedPosition[2].xy, 0.0);
    // vec3 n01 = cross(e01, vec3(0.0, 0.0, 1.0));
    // vec3 n12 = cross(e12, vec3(0.0, 0.0, 1.0));
    // vec3 n20 = cross(e20, vec3(0.0, 0.0, 1.0));
    // float d_e01_n20 = dot(e01.xy, n20.xy);
    // float d_e12_n01 = dot(e12.xy, n01.xy);
    // float d_e20_n12 = dot(e20.xy, n12.xy);
    // float d_e20_n01 = dot(e20.xy, n01.xy);
    // float d_e01_n12 = dot(e01.xy, n12.xy);
    // float d_e12_n20 = dot(e12.xy, n20.xy);

    // if (abs(d_e01_n20) < 0.00001) d_e01_n20 = sign(d_e01_n20) * 0.00001;
    // if (abs(d_e12_n01) < 0.00001) d_e12_n01 = sign(d_e12_n01) * 0.00001;
    // if (abs(d_e20_n12) < 0.00001) d_e20_n12 = sign(d_e20_n12) * 0.00001;
    // if (abs(d_e20_n01) < 0.00001) d_e20_n01 = sign(d_e20_n01) * 0.00001;
    // if (abs(d_e01_n12) < 0.00001) d_e01_n12 = sign(d_e01_n12) * 0.00001;
    // if (abs(d_e12_n20) < 0.00001) d_e12_n20 = sign(d_e12_n20) * 0.00001;

    // projectedPosition[0].xy += pl * ((e01.xy / d_e01_n20) + (e20.xy / d_e20_n01));
    // projectedPosition[1].xy += pl * ((e12.xy / d_e12_n01) + (e01.xy / d_e01_n12));
    // projectedPosition[2].xy += pl * ((e20.xy / d_e20_n12) + (e12.xy / d_e12_n20));

    for (int i = 0; i < 3; i++) {
        vec4 position = projectedPosition[i];
        
        // gs_out.projectedPosition = position.xyz;
        gs_out.worldPosition = gs_in[i].worldPosition;
        gs_out.worldNormal = gs_in[i].worldNormal;
        gs_out.worldTangent = gs_in[i].worldTangent;
        gs_out.vertexTexture = gs_in[i].vertexTexture;
        gs_out.hasTangent = gs_in[i].hasTangent;
        gs_out.dominantAxis = dominantAxis;
        // gs_out.aabb = aabb;
        gl_Position = position;//viewProjectionMatrix * vec4(gs_in[i].worldPosition, 1.0);;
        EmitVertex();
    }
    EndPrimitive();
}