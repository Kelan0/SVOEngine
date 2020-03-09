#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 16
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 16
#endif



layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;
void main() {
    const ivec2 workgroupCoord = ivec2(gl_WorkGroupID.xy);
    const ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 frameSize = imageSize(frame);

}