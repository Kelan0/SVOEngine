#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 32
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 32
#endif

#ifndef LOCAL_SIZE_Z
#define LOCAL_SIZE_Z 1
#endif

layout (binding = 5, rgba32f) uniform image2D xAxisProjectionImage;
layout (binding = 6, rgba32f) uniform image3D voxelStorage;

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;
void main(void) {
    const ivec3 coord = ivec3(gl_GlobalInvocationID.xyz);
    
    if (coord.z == 0) {
        imageStore(xAxisProjectionImage, coord.xy, vec4(0.0));
    }
    imageStore(voxelStorage, coord.xyz, vec4(0.0));
}