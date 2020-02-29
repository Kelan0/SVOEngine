
#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 8
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 8
#endif

#ifndef LOCAL_SIZE_Z
#define LOCAL_SIZE_Z 1
#endif

#define CHILD_FLAG_BIT 0x80000000
#define CHILD_INDEX_MASK 0x7FFFFFFF

layout (binding = 0, rgba16ui) uniform coherent volatile uimageBuffer voxelPositionStorage;
layout (binding = 1, r32ui) uniform coherent volatile uimageBuffer octreeChildIndexStorage;
layout (binding = 2, rgba8) uniform coherent volatile imageBuffer octreeAlbedoStorage;
layout (binding = 3, rg16f) uniform coherent volatile imageBuffer octreeNormalStorage;
layout (binding = 4, rgb10_a2) uniform coherent volatile imageBuffer octreeEmissiveStorage;

// layout (binding = 0) uniform atomic_uint levelTileAllocCount;
// layout (binding = 1) uniform atomic_uint levelTileTotalCount;
// layout (binding = 2) uniform atomic_uint totalTileAllocCount;

layout (std140, binding = 0) uniform OctreePassData {
	int octreeLevel_;
	int levelTileCount_;
	int levelTileStart_;
	int childTileAllocCount_;
	int childTileAllocStart_;
	int voxelFragmentCount_;
	int voxelGridSize_;
	float voxelGridScale_;
	vec4 voxelGridCenter_;
};

layout (std430, binding = 0) coherent buffer OctreeConstructionData {
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

	int octreeLevel;
	int levelTileCount;
	int levelTileStart;
	int childTileAllocCount;
	int childTileAllocStart;
	int voxelFragmentCount;
	int voxelGridSize;
	float voxelGridScale;
	vec4 voxelGridCenter; // must be aligned to 16 bytes

    uint levelTileAllocCount;
    uint levelTileTotalCount;
    uint totalTileAllocCount;
};

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;
void main(void) {
    const int levelNodeIndex = int(gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * 1024);

    if (levelNodeIndex >= levelTileCount * 8) {
        return;
    }

    // Go through each node in this level, and "allocate" an index to each that has been flagged
    // for further division. The allocation is just assigning the index into the octree buffer
    // for the child to be stored. The data at that index is not yet initialized.

    const int octreeNodeIndex = levelTileStart * 8 + levelNodeIndex;
    uint childIndex = imageLoad(octreeChildIndexStorage, octreeNodeIndex).r;

    if ((childIndex & CHILD_FLAG_BIT) != 0) { // Node has been flagged for allocation
        atomicAdd(totalTileAllocCount, 1);
        uint childTileAllocOffset = atomicAdd(levelTileAllocCount, 1);
        uint childAllocIndex = (childTileAllocStart * 8 + childTileAllocOffset * 8);
        childAllocIndex |= CHILD_FLAG_BIT; // This index will be traversed in the next level.
        imageStore(octreeChildIndexStorage, octreeNodeIndex, uvec4(childAllocIndex, 0u, 0u, 0u));
    }

    if (atomicAdd(levelTileTotalCount, 1) == levelTileCount * 8 - 1) { // final invocation
        memoryBarrier();
        int nextTotalAllocCount = int((totalTileAllocCount));

		passLevelTileCount = nextTotalAllocCount - passTotalAllocCount;
		passLevelTileStart = passTotalAllocCount;
		passTotalAllocCount = nextTotalAllocCount;
        
        childTileAllocCount = int((levelTileAllocCount));
        levelTileAllocCount = 0;
        levelTileTotalCount = 0;

        uint nextWorkgroupSizeY = ((passLevelTileCount * 8 + 1023) / 1024 + 7) / 8;
        allocPassWorkgroupSizeY = nextWorkgroupSizeY;
        initPassWorkgroupSizeY = nextWorkgroupSizeY;
        memoryBarrier();
    }
}