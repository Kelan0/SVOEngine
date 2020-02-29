
#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 8
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 8
#endif

#ifndef LOCAL_SIZE_Z
#define LOCAL_SIZE_Z 1
#endif

layout (binding = 0, rgba16ui) uniform coherent volatile uimageBuffer voxelPositionStorage;
layout (binding = 1, r32ui) uniform coherent volatile uimageBuffer octreeChildIndexStorage;
layout (binding = 2, rgba8) uniform coherent volatile imageBuffer octreeAlbedoStorage;
layout (binding = 3, rg16f) uniform coherent volatile imageBuffer octreeNormalStorage;
layout (binding = 4, rgb10_a2) uniform coherent volatile imageBuffer octreeEmissiveStorage;

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

// uniform int octreeLevel;
// uniform int childTileAllocCount;
// uniform int childTileAllocStart;

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;
void main(void) {
    int childNodeIndex = int(gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * 1024);
	
	if (childNodeIndex == int(childTileAllocCount * 8)) {
		octreeLevel++;
	    return;
	}
	
    if(childNodeIndex >= int(childTileAllocCount * 8)) {
	    return;
    }

    int octreeNodeIndex = childTileAllocStart * 8 + childNodeIndex;
    imageStore(octreeChildIndexStorage, octreeNodeIndex, uvec4(0u, 0u, 0u, 0u));
    imageStore(octreeAlbedoStorage, octreeNodeIndex, vec4(0.0, 0.0, 0.0, 0.0));
    imageStore(octreeNormalStorage, octreeNodeIndex, vec4(0.0, 0.0, 0.0, 0.0));
    imageStore(octreeEmissiveStorage, octreeNodeIndex, vec4(0.0, 0.0, 0.0, 0.0));
}