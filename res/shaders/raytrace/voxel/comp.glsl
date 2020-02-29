#ifndef LOCAL_SIZE_X
#define LOCAL_SIZE_X 8
#endif

#ifndef LOCAL_SIZE_Y
#define LOCAL_SIZE_Y 8
#endif

#define CHILD_INDEX_X0Y0Z0 0 //0b000
#define CHILD_INDEX_X0Y0Z1 1 //0b001
#define CHILD_INDEX_X0Y1Z0 2 //0b010
#define CHILD_INDEX_X0Y1Z1 3 //0b011
#define CHILD_INDEX_X1Y0Z0 4 //0b100
#define CHILD_INDEX_X1Y0Z1 5 //0b101
#define CHILD_INDEX_X1Y1Z0 6 //0b110
#define CHILD_INDEX_X1Y1Z1 7 //0b111

#define STACK_DEPTH 20

// returns 0 if bit is not set and a positive integer if it is
#define BIT_SET(mask, bit) ((mask) & (1 << (bit)))

// returns 0 if the bit is not set, and 1 otherwise
#define BIT_VAL(mask, bit) (BIT_SET(mask, bit) >> (bit))


// SORTING NETWORK FOR SORTING 8 VALUES. ALSO PROVIDES INDICES
#define SWAP(t, a, b) { t = a; a = b; b = t; }
#define SORT2(v0, v1, i0, i1) if (v0 > v1) { SWAP(tempVal, v0, v1) SWAP(tempIdx, i0, i1) }
#define SORT8(type, va, ia) { \
	type tempVal; \
	int tempIdx; \
    SORT2(va[0],va[1],ia[0],ia[1]) SORT2(va[2],va[3],ia[2],ia[3]) SORT2(va[0],va[2],ia[0],ia[2]) SORT2(va[1],va[3],ia[1],ia[3]) \
    SORT2(va[1],va[2],ia[1],ia[2]) SORT2(va[4],va[5],ia[4],ia[5]) SORT2(va[6],va[7],ia[6],ia[7]) SORT2(va[4],va[6],ia[4],ia[6]) \
    SORT2(va[5],va[7],ia[5],ia[7]) SORT2(va[5],va[6],ia[5],ia[6]) SORT2(va[0],va[4],ia[0],ia[4]) SORT2(va[1],va[5],ia[1],ia[5]) \
    SORT2(va[1],va[4],ia[1],ia[4]) SORT2(va[2],va[6],ia[2],ia[6]) SORT2(va[3],va[7],ia[3],ia[7]) SORT2(va[3],va[6],ia[3],ia[6]) \
    SORT2(va[2],va[4],ia[2],ia[4]) SORT2(va[3],va[5],ia[3],ia[5]) SORT2(va[3],va[4],ia[3],ia[4]) \
}

struct Ray {
    vec3 origin;
    vec3 direction;
    vec3 inverseDirection;
};

struct AABB {
    vec3 origin;
    vec3 halfExtents;
};

struct Hit {
    bool found;
    float minDist;
    float maxDist;
    vec3 normal;
};

struct VoxelNode {
    int data;
};

struct StackItem {
    int nodeIndex;
    AABB parentBox;
};

uniform mat4x3 cameraRays;
uniform vec3 cameraPosition;

layout(binding = 0, rgba32f) uniform image2D frame;
layout(std430, binding = 1) buffer SVOBuffer {
	VoxelNode nodes[];
};
uniform int nodeCount;

Ray createRay(vec3 origin, vec3 direction) {
    Ray ray;
    ray.origin = origin;
    ray.direction = direction;
    ray.inverseDirection = 1.0 / direction;
    return ray;
}

Ray createRay(vec2 screenPos) {
    vec4 uvUV = vec4(screenPos.xy, 1.0 - screenPos.xy);
    vec4 interp = uvUV.xzzx * uvUV.yyww; //00,10,11,01
    return createRay(cameraPosition, normalize(cameraRays * interp));
}

vec3 getAABBNormal(AABB box, vec3 point) {
    vec3 normal;
    vec3 d = point - box.origin;
    float curDist, minDist = 1.0/0.0;

    curDist = abs(box.halfExtents.x - abs(d.x));
    if (curDist < minDist) {
        minDist = curDist;
        normal = vec3(sign(d.x), 0.0, 0.0);
    }
    curDist = abs(box.halfExtents.y - abs(d.y));
    if (curDist < minDist) {
        minDist = curDist;
        normal = vec3(0.0, sign(d.y), 0.0);
    }
    curDist = abs(box.halfExtents.z - abs(d.z));
    if (curDist < minDist) { 
        minDist = curDist; 
        normal = vec3(0.0, 0.0, sign(d.z));
    }

    return normal;
}
bool testAABB(Ray ray, AABB box, out Hit hit) {
    vec3 t0 = ((box.origin - box.halfExtents) - ray.origin) * ray.inverseDirection;
    vec3 t1 = ((box.origin + box.halfExtents) - ray.origin) * ray.inverseDirection;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);

    float minDist = max(max(tmin.x, tmin.y), tmin.z);
    float maxDist = min(min(tmax.x, tmax.y), tmax.z);


    if (maxDist < minDist || maxDist <= 0) {
        hit.found = false;
        return false;
    }

    hit.minDist = minDist;
    hit.maxDist = maxDist;
    hit.normal = getAABBNormal(box, ray.origin + ray.direction * minDist);

    hit.found = true;
    return true;
}

bool hasChild(VoxelNode node, int index) {
    return BIT_SET(43, index) != 0;
}

bool isLeaf(VoxelNode node) {
    return node.data == 0;
}

int getChildIndex(VoxelNode node, int index) {
    int firstIndex = node.data >> 8;

    if (firstIndex == 0 || !hasChild(node, index)) {
        return -1;
    }

    int offset = 0;
    for (int i = 0; i < index; i++) {
        if (hasChild(node, i)) offset++;
    }

    return firstIndex + offset;
}

AABB getChildBox(AABB parentBox, int index) {
    float x = (BIT_VAL(index, 2) - 0.5);
    float y = (BIT_VAL(index, 1) - 0.5);
    float z = (BIT_VAL(index, 0) - 0.5);

    AABB box;
    box.origin = parentBox.origin + vec3(x, y, z) * parentBox.halfExtents;
    box.halfExtents = parentBox.halfExtents * 0.5;
    return box;
}

bool getTraversalOrder(VoxelNode node, AABB box, bool farToNear, out int traversalOrder[8]) {
    if (isLeaf(node)) {
        return false;
    }

    float distances[8];
    for (int i = 0; i < 8; i++) {
        vec3 cameraToCenter = getChildBox(box, i).origin - cameraPosition;
        distances[i] = dot(cameraToCenter, cameraToCenter); // distance squared
        traversalOrder[i] = farToNear ? (7 - i) : i;
    }

    SORT8(float, distances, traversalOrder);
    return true;
}

bool stackTraversal_0(Ray ray, AABB rootBox, int rootNode, out vec4 outColour) {
    float minDist = 1.0/0.0;
    int stackPos = rootNode;
    StackItem stack[STACK_DEPTH];
    stack[stackPos++] = StackItem(0, rootBox);
    int traversalOrder[8];
    
    const int maxIterations = 150;
    int iterationsRemaining = maxIterations;
    bool ret = false;
    while (stackPos > 0 && --iterationsRemaining >= 0) {
        StackItem stackItem = stack[--stackPos];
        VoxelNode node = nodes[stackItem.nodeIndex];
        AABB parentBox = stackItem.parentBox;
        
        getTraversalOrder(node, parentBox, true, traversalOrder);
        for (int i = 0; i < 8; i++) {
            int orderedIndex = traversalOrder[i];
            int childIndex = getChildIndex(node, orderedIndex);

            if (childIndex != -1) {
                AABB childBox = getChildBox(parentBox, orderedIndex);
                Hit hit;
                if (testAABB(ray, childBox, hit)) {
                    if (hit.minDist >= minDist) {
                        continue;
                    }
                    if (hit.minDist > 0.0 && (nodes[childIndex].data) == 0) {
                        if (hit.minDist < minDist) {
                            minDist = hit.minDist;
                            outColour.rgb = vec3(hit.normal * 0.5 + 0.5);
                            ret = true;
                        }
                    }
                    
                    if (stackPos < STACK_DEPTH - 1) {
                        vec3 cameraToBox = childBox.origin - cameraPosition;
                        if (dot(cameraToBox, cameraToBox) >= minDist * minDist) {
                            continue;
                        }

                        stack[stackPos++] = StackItem(childIndex, childBox);
                    }
                }
            }
        }
    }

    float cost = 1.0 - float(iterationsRemaining) / float(maxIterations);
    //float a = clamp(cost * 2.0, 0.0, 1.0);
    //float b = clamp(cost * 2.0 - 1.0, 0.0, 1.0);
    //outColour.rgb = (mix(vec3(0,0,1), vec3(0,0.5,0), a) + mix(vec3(0,0.5,0), vec3(1,0,0), b));

    outColour.rgb = vec3(0.0, cost, 0.0);

    return ret;
}

bool stackTraversal_1(Ray ray, AABB rootBox, int rootNode, out vec4 outColour) {
    Hit hit;
    bool ret = false;
    float minDist = 1.0/0.0;
    int traversalOrder[8];

    StackItem stack[STACK_DEPTH];
    int stackIndex = 0;

    stack[stackIndex++] = StackItem(rootNode, rootBox);
    
    while (stackIndex > 0) {
        StackItem stackItem = stack[--stackIndex];
        VoxelNode node = nodes[stackItem.nodeIndex];
        AABB parentBox = stackItem.parentBox;

        if (testAABB(ray, parentBox, hit)) {
            if (isLeaf(node)) {
                if (hit.minDist > 0) {
                    minDist = hit.minDist;
                    outColour.rgb = vec3(hit.normal * 0.5 + 0.5);
                    ret = true;
                }
                if (false) {
                    continue;
                } else {
                    break;
                }
            } else {
                getTraversalOrder(node, parentBox, true, traversalOrder);
                for (int i = 0; i < 8; i++) {
                    int childIndex = getChildIndex(node, traversalOrder[i]);
                    if (childIndex != -1) {
                        VoxelNode child = nodes[childIndex];
                        stack[stackIndex++] = StackItem(childIndex, parentBox);
                    }
                }
            }
        } else {
            continue;
        }
    }

    return ret;
}

bool stackTraversal_1(Ray ray, AABB rootBox, int rootNode, out vec4 outColour) {
    return true;
}

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;
void main() {
    const ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 frameSize = imageSize(frame);

    if (pixelCoord.x >= frameSize.x || pixelCoord.y >= frameSize.y) return;

    vec2 screenPos = vec2(pixelCoord) / vec2(frameSize);

    Ray ray = createRay(screenPos);
    vec4 outColour = vec4(abs(ray.direction), 1.0);

    AABB rootBox;
    rootBox.origin = vec3(0.0, 0.0, 0.0);
    rootBox.halfExtents = vec3(5.0, 5.0, 5.0);
    
    stackTraversal_1(ray, rootBox, 0, outColour);

    //if (pixelCoord.x % LOCAL_SIZE_X == 0 || pixelCoord.y % LOCAL_SIZE_Y == 0) {
    //    outColour.rgb = vec3(0.0);
    //}

    imageStore(frame, pixelCoord, outColour);
}