
#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 32
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 32
#endif

#ifndef LOCAL_SIZE_Z
#define LOCAL_SIZE_Z 1
#endif

layout (binding = 0, rgba16f) uniform coherent volatile image3D voxelVisualizationTexture;

layout (binding = 1, rgba16ui) uniform coherent volatile uimageBuffer voxelPositionStorage;
layout (binding = 2, rgba8) uniform coherent volatile imageBuffer voxelAlbedoStorage;
layout (binding = 3, rg16f) uniform coherent volatile imageBuffer voxelNormalStorage;
layout (binding = 4, rgb10_a2) uniform coherent volatile imageBuffer voxelEmissiveStorage;

uniform int voxelFragmentCount;

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;
void main(void) {
    const int voxelIndex = int(1024u * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x);

    if (voxelIndex >= voxelFragmentCount) {
       return;
    }

    ivec3 voxelPosition = ivec3(imageLoad(voxelPositionStorage, voxelIndex).xyz);
    vec4 voxelAlbedo = vec4(imageLoad(voxelAlbedoStorage, voxelIndex).rgba);
    vec3 voxelNormal = vec3(imageLoad(voxelNormalStorage, voxelIndex).xyz);
    vec3 voxelEmissive = vec3(imageLoad(voxelEmissiveStorage, voxelIndex).rgb);

    // ivec3(voxelIndex & 0x1FF, (voxelIndex >> 9) & 0x1FF, (voxelIndex >> 18) & 0x1FF)
    imageStore(voxelVisualizationTexture, voxelPosition, vec4(voxelAlbedo.rgb, 1.0));
}