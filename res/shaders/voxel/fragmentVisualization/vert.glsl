#define CHILD_FLAG_BIT 0x80000000
#define STATIC_FLAG_BIT 0x40000000
#define CHILD_INDEX_MASK 0x3FFFFFFF//0x7FFFFFFF // 2^30 indices

in vec3 fs_vertexPosition;

out VertexData {
    ivec3 gridCoord;
    vec3 voxelPosition;
    vec4 voxelAlbedo;
    vec3 voxelNormal;
    vec3 voxelEmissive;
} vs_out;

layout (binding = 0, rgba16ui) uniform /*coherent volatile*/ uimageBuffer voxelPositionStorage;
layout (binding = 1, rgba32ui) uniform /*coherent volatile*/ uimageBuffer voxelDataStorage;
// layout (binding = 1, rgba8) uniform coherent volatile imageBuffer voxelAlbedoStorage;
// layout (binding = 2, rg16f) uniform coherent volatile imageBuffer voxelNormalStorage;
// layout (binding = 3, rgb10_a2) uniform coherent volatile imageBuffer voxelEmissiveStorage;
layout (binding = 2, r32ui) uniform /*coherent volatile*/ uimageBuffer octreeChildIndexStorage;
layout (binding = 3, rgba32ui) uniform /*coherent volatile*/ uimageBuffer octreeDataStorage;

uniform float voxelGridScale;
uniform vec3 voxelGridCenter;
uniform mat4 viewProjectionMatrix;
uniform int voxelGridSize;
uniform int octreeLevel;
uniform int octreeDepth;


int getOctantIndex(ivec3 octant) {
    return (octant.x << 2) | (octant.y << 1) | (octant.z << 0); // dot(ivec3(1, 2, 4), octant);
}

int getChildIndex(uint node, int octantIndex) {
    return int(node & CHILD_INDEX_MASK) + octantIndex;
}

int getChildIndex(uint node, ivec3 octant) {
    return getChildIndex(node, getOctantIndex(octant));
}

bool isNodeOccupied(uint node) {
    return (node & CHILD_FLAG_BIT) != 0;
}

bool descendOctree(ivec3 gridCoord, int octreeLevel, inout int nodeIndex, inout uint node) {
    nodeIndex = 0;
    node = 0;
    
    if (octreeLevel != 0) {
        int currGridSize = voxelGridSize;
        ivec3 gridOffset = ivec3(0,0,0);
        ivec3 octant;
        int i;

        node = imageLoad(octreeChildIndexStorage, nodeIndex).r;

        for(i = 0; i < octreeLevel; ++i) {
            if (!isNodeOccupied(node))
                return false;
            
            currGridSize >>= 1; // /= 2
            octant = clamp(1 + gridCoord - gridOffset - currGridSize, 0, 1);
            gridOffset += currGridSize * octant;

            nodeIndex = getChildIndex(node, octant);
            node = imageLoad(octreeChildIndexStorage, nodeIndex).r;
        }
    }

    return true;
}

void main() {
    int idx = gl_VertexID;
    ivec3 gridSize = ivec3(voxelGridSize);
    
    // int z = idx / (gridSize.x * gridSize.y);
    // idx -= (z * gridSize.x * gridSize.y);
    // int y = idx / gridSize.x;
    // int x = idx % gridSize.x;

    // ivec3 gridCoord = ivec3(x, y, z);

    uint node;
    int nodeIndex;
    ivec3 gridCoord = ivec3(imageLoad(voxelPositionStorage, idx).xyz);
    if (!descendOctree(gridCoord, octreeLevel, nodeIndex, node)) {
        return;
    }

    uvec4 voxelData = imageLoad(octreeDataStorage, nodeIndex);

    vec4 voxelAlbedo = unpackUnorm4x8(voxelData[0]).rgba; // vec4(imageLoad(voxelAlbedoStorage, idx).rgba);
    vec3 voxelNormal = unpackUnorm2x16(voxelData[1]).xyy; // vec3(imageLoad(voxelNormalStorage, idx).xyz);
    vec3 voxelEmissive = unpackUnorm4x8(voxelData[2]).rgb; // vec3(imageLoad(voxelEmissiveStorage, idx).rgb);

    vec3 voxelPosition = voxelGridCenter + (vec3(gridCoord) - (vec3(gridSize) * 0.5 - 0.5)) * voxelGridScale;

    vs_out.gridCoord = gridCoord;
    vs_out.voxelPosition = voxelPosition;
    vs_out.voxelAlbedo = voxelAlbedo;
    vs_out.voxelNormal = voxelNormal;
    vs_out.voxelEmissive = voxelEmissive;

    gl_Position = viewProjectionMatrix * vec4(voxelPosition, 1.0);
}
