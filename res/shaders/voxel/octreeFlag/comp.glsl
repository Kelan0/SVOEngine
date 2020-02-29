#include "globals.glsl"

#ifndef DATA_SIZE
#define DATA_SIZE 64
#endif

#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 64
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 16
#endif

#ifndef LOCAL_SIZE_Z
#define LOCAL_SIZE_Z 1
#endif

#define CHILD_FLAG_BIT 0x80000000
#define STATIC_FLAG_BIT 0x40000000
#define CHILD_INDEX_MASK 0x3FFFFFFF//0x7FFFFFFF // 2^30 indices

// coherent qualifier significantly affects performance (512^3 volume with coherent in 8ms, vs 1.5ms without)
// It may not be needed if all shader invocations in the same pass are independent, and do not rely on the 
// reads/writes of others.

// rgba16ui = rg32ui = rgb8+rgb8
layout (binding = 0, r32ui) uniform /*coherent volatile*/ uimageBuffer octreeChildIndexStorage;
layout (binding = 1, rgba32ui) uniform /*coherent volatile*/ uimageBuffer octreeDataStorage;
// layout (binding = 1, rgba8) uniform /*coherent volatile*/ imageBuffer octreeAlbedoStorage;
// layout (binding = 2, rg16f) uniform /*coherent volatile*/ imageBuffer octreeNormalStorage;
// layout (binding = 3, rgb10_a2) uniform /*coherent volatile*/ imageBuffer octreeEmissiveStorage;
layout (binding = 3, r32ui) uniform /*coherent volatile*/ uimageBuffer octreeLeafHeadIndexBuffer;

layout (binding = 4, rgba16ui) uniform /*coherent volatile*/ uimageBuffer voxelPositionStorage;
layout (binding = 5, rgba32ui) uniform /*coherent volatile*/ uimageBuffer voxelDataStorage;
// layout (binding = 5, rgba8) uniform /*coherent volatile*/ imageBuffer voxelAlbedoStorage;
// layout (binding = 6, rg16f) uniform /*coherent volatile*/ imageBuffer voxelNormalStorage;
// layout (binding = 7, rgb10_a2) uniform /*coherent volatile*/ imageBuffer voxelEmissiveStorage;

struct VoxelLinkedListNode {
    uint albedo; // rgba8
    uint normal; // rg16f
    uint emissive; // rgb10_a2
    uint next;
};

layout (std430, binding = 0) /*coherent*/ buffer OctreeConstructionData { // Making this coherent MASSIVELY affects performance...
	uint flagPassWorkgroupSizeX;
	uint flagPassWorkgroupSizeY;
	uint flagPassWorkgroupSizeZ;
    uint allocPassWorkgroupSizeX;
    uint allocPassWorkgroupSizeY;
    uint allocPassWorkgroupSizeZ;
    uint initPassWorkgroupSizeX;
    uint initPassWorkgroupSizeY;
    uint initPassWorkgroupSizeZ;
	int passLevelTileCount;
	int passLevelTileStart;
	int passTotalAllocCount;

	vec4 voxelGridCenter; // must be aligned to 16 bytes
	int voxelFragmentCount;
	int voxelGridSize;
	int octreeLevel;
	int levelTileCount;
	int levelTileStart;
	int childTileAllocCount;
	int childTileAllocStart;
    int stage;

    uint levelTileAllocCount;
    uint levelTileTotalCount;
    uint totalTileAllocCount;
    uint leafLinkedListCounter;

    VoxelLinkedListNode voxelList[];
};

#define NODE_FLAG_STAGE 0
#define NODE_ALLOC_STAGE 1
#define NODE_INIT_STAGE 2
#define LEAF_VOXEL_STORAGE_STAGE 3
#define LEAF_VOXEL_AVERAGE_STAGE 4
#define LEAF_VOXEL_PROPAGATION 5

//uniform int stage;
uniform bool staticGeometry;
uniform bool storeFragments;
uniform int maxLeafVoxelListLength;

bool isPassCount(uint count) {
    if (atomicAdd(levelTileTotalCount, 1) == count) {
        levelTileTotalCount = 0;
        return true;
    }

    return false;
}

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
            octant.x = gridCoord.x < (gridOffset.x + currGridSize) ? 0 : 1;
            octant.y = gridCoord.y < (gridOffset.y + currGridSize) ? 0 : 1;
            octant.z = gridCoord.z < (gridOffset.z + currGridSize) ? 0 : 1;
            gridOffset += currGridSize * octant;

            nodeIndex = getChildIndex(node, octant);
            node = imageLoad(octreeChildIndexStorage, nodeIndex).r;
        } 
    }

    return true;
}

void nodeFlagStage() {
    int voxelIndex = int(gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * DATA_SIZE);
    int m_octreeLevel = octreeLevel;
    int nodeIndex = 0;
    uint node = 0;

    if (voxelIndex == 0) {
        // Not necessarily 0th invocation, no atomic counter needed since this only needs to happen before the alloc pass.
        levelTileCount = passLevelTileCount;
        levelTileStart = passLevelTileStart;
        childTileAllocStart = passTotalAllocCount;
        passLevelTileCount = 0;
    }
    
	if (voxelIndex >= voxelFragmentCount) {
	    return;
    }

    if (!descendOctree(ivec3(imageLoad(voxelPositionStorage, voxelIndex)), m_octreeLevel, nodeIndex, node)) {
        return;
    }
    
    if (!isNodeOccupied(node)) {
        node |= CHILD_FLAG_BIT;
        // if (staticGeometry) 
        //     node |= STATIC_FLAG_BIT;

        // imageAtomicOr(octreeChildIndexStorage, nodeIndex, CHILD_FLAG_BIT); // slower :(
        imageStore(octreeChildIndexStorage, nodeIndex, uvec4(node,0,0,0));
    }
}

void nodeAllocStage() {
    const int levelNodeIndex = int(gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * DATA_SIZE);
    const int m_levelTileCount = levelTileCount;

    if (levelNodeIndex >= m_levelTileCount * 8) {
        return;
    }

    // Go through each node in this level, and "allocate" an index to each that has been flagged
    // for further division. The allocation is just assigning the index into the octree buffer
    // for the child to be stored. The data at that index is not yet initialized.

    const int octreeNodeIndex = levelTileStart * 8 + levelNodeIndex;
    const uint node = imageLoad(octreeChildIndexStorage, octreeNodeIndex).r;

    if (isNodeOccupied(node)) { // Node has been flagged for allocation
        uint childTileAllocOffset = atomicAdd(levelTileAllocCount, 1);
        uint childAllocIndex = (childTileAllocStart * 8 + childTileAllocOffset * 8);
        childAllocIndex |= CHILD_FLAG_BIT; // This index will be traversed in the next level.
        imageStore(octreeChildIndexStorage, octreeNodeIndex, uvec4(childAllocIndex, 0u, 0u, 0u));
    }

    // MOVE THIS OUT OF SHADER?
    // TODO: test if it is faster for this to be its own separate pass rather than an atomic count...
    if (isPassCount(m_levelTileCount * 8 - 1)) { // final invocation
        // memoryBarrier();
        // totalTileAllocCount += levelTileAllocCount;
        // int nextTotalAllocCount = int((totalTileAllocCount));

		passLevelTileCount = int(levelTileAllocCount);
		passLevelTileStart = int(passTotalAllocCount);
		passTotalAllocCount += int(levelTileAllocCount);
        
        childTileAllocCount = int((levelTileAllocCount));
        levelTileAllocCount = 0;

        uint nextWorkgroupSizeY = ((passLevelTileCount * 8 + (DATA_SIZE - 1)) / DATA_SIZE + (LOCAL_SIZE_Y - 1)) / LOCAL_SIZE_Y;
        allocPassWorkgroupSizeY = nextWorkgroupSizeY;
        initPassWorkgroupSizeY = nextWorkgroupSizeY;
        //stage = NODE_INIT_STAGE;
        // memoryBarrier();
    }
}

void nodeInitStage() {
    int childNodeIndex = int(gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * DATA_SIZE);
	
	if (childNodeIndex == 0) {
		++octreeLevel;
	}
    
    //if (isPassCount(childTileAllocCount * 8 - 1)) {
    //    stage = NODE_FLAG_STAGE;
    //}
	
    if (childNodeIndex >= int(childTileAllocCount * 8)) {
	    return;
    }

    int octreeNodeIndex = childTileAllocStart * 8 + childNodeIndex;
    imageStore(octreeChildIndexStorage, octreeNodeIndex, uvec4(0u, 0u, 0u, 0u));
    imageStore(octreeDataStorage, octreeNodeIndex, uvec4(0u, 0u, 0u, 0u));
}

void leafVoxelStorageStage() {
    int voxelIndex = int(gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * DATA_SIZE);
    int m_octreeLevel = octreeLevel - 1;
    int nodeIndex = 0;
    uint node = 0;

	if (voxelIndex >= voxelFragmentCount) {
	    return;
    }

    if (!descendOctree(ivec3(imageLoad(voxelPositionStorage, voxelIndex)), m_octreeLevel, nodeIndex, node)) {
        return;
    }
    
    if (isNodeOccupied(node)) {
        uvec4 voxelData = imageLoad(voxelDataStorage, voxelIndex);
        uint listIndex = atomicAdd(leafLinkedListCounter, 1);
        uint prevHead = imageAtomicExchange(octreeLeafHeadIndexBuffer, nodeIndex, listIndex);

        VoxelLinkedListNode node;
        node.albedo = voxelData[0];
        node.normal = voxelData[1];
        node.emissive = voxelData[2];
        node.next = prevHead;

        voxelList[listIndex] = node;
    }
}

void leafVoxelAverageStage() {
    int voxelIndex = int(gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * DATA_SIZE);
    int m_octreeLevel = octreeLevel - 1;
    int nodeIndex = 0;
    uint node = 0;

	if (voxelIndex >= voxelFragmentCount) {
	    return;
    }

    if (!descendOctree(ivec3(imageLoad(voxelPositionStorage, voxelIndex)), m_octreeLevel, nodeIndex, node)) {
        return;
    }
    
    if (isNodeOccupied(node)) {
        uint listNodeIndex = imageLoad(octreeLeafHeadIndexBuffer, nodeIndex).r;

        vec4 voxelAlbedo = vec4(0.0);
        vec2 voxelNormal = vec2(0.0);
        vec4 voxelEmissive = vec4(0.0);

        int count;
        for (count = 0; count < maxLeafVoxelListLength && listNodeIndex != 0xFFFFFFFF; ++count) {
            VoxelLinkedListNode node = voxelList[listNodeIndex];
            voxelAlbedo += unpackUnorm4x8(node.albedo);
            voxelNormal += unpackUnorm2x16(node.normal);
            voxelEmissive += unpackUnorm4x8(node.emissive);
            listNodeIndex = node.next;
        }

        if (count > 0) {
            float invSum = 1.0 / count;
            uint packedVoxelAlbedo = packUnorm4x8(voxelAlbedo * invSum);
            uint packedVoxelNormal = packUnorm2x16(voxelNormal * invSum);
            uint packedVoxelEmissive = packUnorm4x8(voxelEmissive * invSum);

            imageStore(octreeDataStorage, nodeIndex, uvec4(packedVoxelAlbedo, packedVoxelNormal, packedVoxelEmissive, 0u));
        }
    }
}

void leafVoxelPropagationStage() {
    // start at max octree level
    // descend octree down to current level
    // set node values to average of the occupied children
    // on last invocation, decrement current octree level

    // DO WE NEED TO GO THROUGH ALL VOXEL FRAGMENTS HERE? THE LEAF NODES ARE ALREADY BUILT.
    // POSSIBLE TO ONLY GO THROUGH THE NODES OF THE CURRENT LEVEL? HOW DO WE KNOW THEIR POSITION?
    // POSSIBLE TO PROCESS OCTREE LEVEL WITHOUT DESCENDING TREE FROM ROOT?
    // TODO: investigate stage assignment from within shader rather than uniform.
    // TODO: investigate moving some atomicAdd calls to dedicated atomic counters. Is this faster? Possible to reset from within shader?

    int voxelIndex = int(gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * DATA_SIZE);
    int m_octreeLevel = octreeLevel - 2;
    int nodeIndex = 0;
    uint node = 0;

	if (m_octreeLevel < 0 || voxelIndex >= voxelFragmentCount) {
	    return;
    }

    // TODO: iterate over level nodes directly rather than descending from the root for all fragment positions...
    if (!descendOctree(ivec3(imageLoad(voxelPositionStorage, voxelIndex)), m_octreeLevel, nodeIndex, node)) {
        return;
    }
    
    if (isNodeOccupied(node)) {
        vec4 voxelAlbedo = vec4(0.0);
        vec2 voxelNormal = vec2(0.0);
        vec4 voxelEmissive = vec4(0.0);
        int count = 0;

        for (int i = 0; i < 8; ++i) {
            const int childNodeIndex = getChildIndex(node, i);
            const uint childNode = imageLoad(octreeChildIndexStorage, childNodeIndex).r;
            if (isNodeOccupied(childNode)) {
                const uvec4 childNodeData = imageLoad(octreeDataStorage, childNodeIndex);
                voxelAlbedo += unpackUnorm4x8(childNodeData[0]);
                voxelNormal += unpackUnorm2x16(childNodeData[1]);
                voxelEmissive += unpackUnorm4x8(childNodeData[2]);
                ++count;
            }
        }

        if (count > 0) {
            float invSum = 1.0 / count;
            uint packedVoxelAlbedo = packUnorm4x8(voxelAlbedo * invSum);
            uint packedVoxelNormal = packUnorm2x16(voxelNormal * invSum);
            uint packedVoxelEmissive = packUnorm4x8(voxelEmissive * invSum);

            imageStore(octreeDataStorage, nodeIndex, uvec4(packedVoxelAlbedo, packedVoxelNormal, packedVoxelEmissive, 0u));
        }
    }

    if (isPassCount(voxelFragmentCount - 1)) {
        --octreeLevel;
    }
}

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;
void main() {
    // TODO optimizations:
    // minimuse imageStore/Load operations
    // minimise direct SSBO access
    // move read-only SSBO data to uniform buffer
    // minimise division
    // keep data packed where possible
    // find cache coherency optimisations
    // align data to cache lines (128 bytes)

    // FLAG -> ALLOC -> INIT    executed for each octree level
    // STORAGE -> AVERAGE       executed for final octree leaf nodes
    switch (stage) {
        case NODE_FLAG_STAGE: nodeFlagStage(); break;
        case NODE_ALLOC_STAGE: nodeAllocStage(); break;
        case NODE_INIT_STAGE: nodeInitStage(); break;
        case LEAF_VOXEL_STORAGE_STAGE: leafVoxelStorageStage(); break;
        case LEAF_VOXEL_AVERAGE_STAGE: leafVoxelAverageStage(); break;
        case LEAF_VOXEL_PROPAGATION: leafVoxelPropagationStage(); break;
    }
}