#include "globals.glsl"

#define CHILD_FLAG_BIT 0x80000000
#define CHILD_INDEX_MASK 0x7FFFFFFF

#define X_IDX 2
#define Y_IDX 1
#define Z_IDX 0
#define X_BIT (1 << X_IDX)
#define Y_BIT (1 << Y_IDX)
#define Z_BIT (1 << Z_IDX)

#define OCTANT_IDX(x, y, z) ((x << X_IDX) | (y << Y_IDX) | (z << Z_IDX))
#define OCTANT_INVALID 8

in VertexData {
    vec2 vertexPosition;
    vec2 vertexTexture;
} fs_in;

// struct StackNode {
//     int level;
//     int nodeIndex;
//     int childIndex;
//     uint node;
//     vec3 b0;
//     vec3 b1;
// };

struct StackNode {
    int level;
    int nodeIndex;
    int childIndex;
    int currGridSize;
    ivec3 gridOffset;
};

struct RayIntersection {
    float intersectionDistance;
    vec3 intersectionNormal;
    vec3 intersectionColour;
};

out vec3 outAlbedo;
out vec2 outNormal;
out float outRoughness;
out float outMetalness;
out float outAmbientOcclusion;
out vec3 outIrradiance;
out vec3 outReflection;

layout (binding = 0, rgba16ui) uniform /*coherent volatile*/ uimageBuffer voxelPositionStorage;
layout (binding = 1, r32ui) uniform /*coherent volatile*/ uimageBuffer octreeChildIndexStorage;
layout (binding = 2, rgba32ui) uniform /*coherent volatile*/ uimageBuffer octreeDataStorageBuffer;
// layout (binding = 2, rgba8) uniform /*coherent volatile*/ imageBuffer octreeAlbedoStorage;
// layout (binding = 3, rg16f) uniform /*coherent volatile*/ imageBuffer octreeNormalStorage;
// layout (binding = 4, rgb10_a2) uniform /*coherent volatile*/ imageBuffer octreeEmissiveStorage;

layout (binding = 5, rgba8) uniform image2D axisProjectionMap;

uniform vec3 cameraPosition;
uniform mat4x3 cameraRays;
uniform mat4 viewProjectionMatrix;
uniform int octreeLevel;
uniform int octreeDepth;
uniform float voxelGridScale;
uniform vec3 voxelGridCenter;
uniform int voxelGridSize;


vec3 getVoxelWorldPosition(ivec3 gridCoord, int octreeLevel) {
    // int levelDim = (1 << (octreeDepth - (octreeLevel + 1)));
    float gridScale = voxelGridScale;// * levelDim;
    int gridSize = voxelGridSize;// / levelDim;
    vec3 gridCenter = floor((voxelGridCenter) / gridScale) * gridScale;
    
    vec3 worldPosition = gridCenter + (gridCoord - gridSize / 2) * gridScale;
    return worldPosition;//floor(worldPosition / (gridScale * 2)) * (gridScale * 2);
}

void getVoxelWorldBounds(ivec3 gridCoord, int octreeLevel, out vec3 boundMin, out vec3 boundMax) {
    const int i = (octreeDepth) - (octreeLevel);
    const int levelGridSize = voxelGridSize >> i;
    const float levelGridScale = voxelGridScale * (1 << i);

    const vec3 gridCenter = floor((voxelGridCenter) / (voxelGridScale * 2)) * (voxelGridScale * 2);
    const vec3 voxelCoord = gridCenter + ((gridCoord >> i) - (levelGridSize * 0.5)) * levelGridScale;
    boundMin = voxelCoord;
    boundMax = voxelCoord + levelGridScale;
}

int getOctantIndex(ivec3 octant) {
    return (octant.x << 2) | (octant.y << 1) | (octant.z << 0); // dot(ivec3(1, 2, 4), octant);
}

ivec3 getOctantCoord(int octantIndex) {
    return ivec3((octantIndex & 4) >> 2, (octantIndex & 2) >> 1, octantIndex & 1);
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

bool intersectBoxParameter(vec3 tbot, vec3 ttop, out float t0, out float t1) {
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);
    return t1 > max(t0, 0.0);
}

bool intersectBox(const Ray ray, const vec3 boxMin, const vec3 boxMax, out float t0, out float t1) {
    vec3 tbot = ray.inverseDirection * (boxMin - ray.origin);
    vec3 ttop = ray.inverseDirection * (boxMax - ray.origin);
    return intersectBoxParameter(tbot, ttop, t0, t1);
}

bool insideBox(const vec3 point, const vec3 boxMin, const vec3 boxMax, bool incMin = false, bool incMax = false) {
    return
        point.x > boxMin.x && point.y > boxMin.y && point.z > boxMin.z && 
        point.x < boxMax.x && point.y < boxMax.y && point.z < boxMax.z;
}

vec3 getBoxNormal(vec3 point, vec3 boxMin, vec3 boxMax) {
    vec3 origin = (boxMax + boxMin) * 0.5;
    vec3 halfExtents = (boxMax - boxMin) * 0.5;
    vec3 normal;
    vec3 d = point - origin;
    float curDist, minDist = 1.0/0.0;

    curDist = abs(halfExtents.x - abs(d.x));
    if (curDist < minDist) {
        minDist = curDist;
        normal = vec3(sign(d.x), 0.0, 0.0);
    }
    curDist = abs(halfExtents.y - abs(d.y));
    if (curDist < minDist) {
        minDist = curDist;
        normal = vec3(0.0, sign(d.y), 0.0);
    }
    curDist = abs(halfExtents.z - abs(d.z));
    if (curDist < minDist) { 
        minDist = curDist; 
        normal = vec3(0.0, 0.0, sign(d.z));
    }

    return normal;
}

// bool rayPlaneIntersection(Ray ray, vec4 plane, out float t0) {
//     float denom = dot(ray.direction, plane.xyz);
//     if (abs(denom) < 1e-6) return false; // ray is parallel
//     float dist = (-plane.w - dot(ray.origin, plane.xyz)) / denom;
//     if (dist > 0.0) {
//         t0 = dist;
//         return true;
//     }

//     return false;
// }

bool rayPlaneIntersection(Ray ray, vec3 p0, vec3 n, out float t0) {
    float denom = dot(n, ray.direction); 
    if (abs(denom) < 1e-6) return false; // ray is parallel
    float dist = dot(p0 - ray.origin, n) / denom; 
    if (dist > 0.0) {
        t0 = dist;
        return true;
    }

    return false;
} 

int firstNode(vec3 t0, vec3 tm) {
    int index = 0;
    
    if (t0.x > t0.y && t0.x > t0.z) { // max x, YZ plane
        if (tm.y < t0.x) index |= Y_BIT;
        if (tm.z < t0.x) index |= Z_BIT;

    } else if (t0.y > t0.z) {// max y, XZ plane
        if (tm.x < t0.y) index |= X_BIT;
        if (tm.z < t0.y) index |= Z_BIT;

    } else { // max z, XY plane
        if (tm.x < t0.z) index |= X_BIT;
        if (tm.y < t0.z) index |= Y_BIT;
    }

    return index;
}

int nextNode(float x1, float y1, float z1, int i0, int i1, int i2) {
    if (x1 < y1 && x1 < z1) { // min x, YZ plane
        return i0;
    } else if (y1 < z1) { // min y, XZ plane
        return i1;
    } else { // min z, XY plane
        return i2;
    }
}

bool processLeaf(uint nodeIndex, Ray ray, vec3 b0, vec3 b1, int a, out RayIntersection intersection) {
    vec3 t0 = ((b0) - ray.origin) * ray.inverseDirection;
    vec3 t1 = ((b1) - ray.origin) * ray.inverseDirection;

    float tmin, tmax;
    intersectBoxParameter(t0, t1, tmin, tmax);
    if (tmin < tmax && tmax > 0.0) {
        float d = tmin < 0.0 ? tmax : tmin;

        // if (t0.x > t0.y && t0.x > t0.z)
        //     intersection.intersectionNormal = vec3((a & X_BIT) * 2 - 1, 0, 0);
        // else if (t0.y > t0.z)
        //     intersection.intersectionNormal = vec3(0, (a & Y_BIT) * 2 - 1, 0);
        // else
        //     intersection.intersectionNormal = vec3(0, 0, (a & Z_BIT) * 2 - 1);

        uvec4 octreeData = imageLoad(octreeDataStorageBuffer, int(nodeIndex)).rgba;
        intersection.intersectionDistance = d;
        intersection.intersectionColour = unpackUnorm4x8(octreeData[0]).rgb;//imageLoad(octreeAlbedoStorage, int(nodeIndex)).rgb;
        intersection.intersectionNormal = unpackUnorm2x16(octreeData[1]).xyy;//imageLoad(octreeNormalStorage, int(nodeIndex)).xyz;
        return true;
    }
    return false;
}

bool rayParameter(Ray ray, out RayIntersection intersection) {
    vec3 boundMin, boundMax;
    getVoxelWorldBounds(ivec3(0,0,0), 0, boundMin, boundMax);
    ray.origin -= boundMin;

    vec3 dim = boundMax - boundMin;
    int a = 0;

    if (ray.direction.x < 0.0) {
        ray.origin.x = dim.x - ray.origin.x;
        ray.direction.x = -ray.direction.x;
        ray.inverseDirection.x = -ray.inverseDirection.x;
        a |= X_BIT;
    }
    if (ray.direction.y < 0.0) {
        ray.origin.y = dim.y - ray.origin.y;
        ray.direction.y = -ray.direction.y;
        ray.inverseDirection.y = -ray.inverseDirection.y;
        a |= Y_BIT;
    }
    if (ray.direction.z < 0.0) {
        ray.origin.z = dim.z - ray.origin.z;
        ray.direction.z = -ray.direction.z;
        ray.inverseDirection.z = -ray.inverseDirection.z;
        a |= Z_BIT;
    }

    vec3 rt0 = ((boundMin - boundMin) - ray.origin) * ray.inverseDirection;
    vec3 rt1 = ((boundMax - boundMin) - ray.origin) * ray.inverseDirection;


    if (max(max(rt0.x, rt0.y), rt0.z) < min(min(rt1.x, rt1.y), rt1.z)) {
        const int maxLevel = octreeLevel;
        const int stackLength = 32;
        int stackHead = 0;
        StackNode stack[stackLength];
        stack[stackHead++] = StackNode(0, 0, 0, voxelGridSize, ivec3(0,0,0));

        uint node;
        int childIndex, currOctant, count, octantOffset, nextLevel, nextIndex, currGridSize, nextGridSize;
        vec3 b0, b1, bm, t0, t1, tm;
        ivec3 gridOffset;
        StackNode item;
        StackNode traversedChildren[4]; // Ray can't intersect more than 4 children.

        while (stackHead > 0) {
            item = stack[--stackHead];
            gridOffset = item.gridOffset;
            currGridSize = item.currGridSize;
            nextGridSize = currGridSize >> 1;// / 2;
            getVoxelWorldBounds(gridOffset, item.level, b0, b1);
            b0 -= boundMin;
            b1 -= boundMin;
            t0 = (b0 - ray.origin) * ray.inverseDirection;
            t1 = (b1 - ray.origin) * ray.inverseDirection;

            if (item.level > maxLevel) {
                break;
            }

            if (t1.x < 0.0 || t1.y < 0.0 || t1.z < 0.0)
                continue;
            
            node = imageLoad(octreeChildIndexStorage, item.nodeIndex).r;

            if (!isNodeOccupied(node)) {
                continue;
            }

            childIndex = int(node & CHILD_INDEX_MASK);

            if (childIndex == 0) {
                continue;
            }

            if (item.level == maxLevel) {
                if (processLeaf(item.nodeIndex, ray, b0, b1, a, intersection)) {
                    return true;
                }
                continue;
            }

            tm = (t0 + t1) * 0.5;
            bm = (b0 + b1) * 0.5;

            count = 0;
            currOctant = firstNode(t0, tm);

            do {
                octantOffset = currOctant ^ a; // xor
                nextLevel = item.level + 1;
                nextIndex = childIndex + octantOffset;

                switch (currOctant) {
                case OCTANT_IDX(0,0,0): // 000 -> MMM
                    traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, nextGridSize, gridOffset + nextGridSize * ivec3(0,0,0));
                    currOctant = nextNode(tm.x, tm.y, tm.z, OCTANT_IDX(1,0,0), OCTANT_IDX(0,1,0), OCTANT_IDX(0,0,1));
                    break;
                
                case OCTANT_IDX(0,0,1): // 00M -> MM1
                    traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, nextGridSize, gridOffset + nextGridSize * ivec3(0,0,1));
                    currOctant = nextNode(tm.x, tm.y, t1.z, OCTANT_IDX(1,0,1), OCTANT_IDX(0,1,1), OCTANT_INVALID);
                    break;
                
                case OCTANT_IDX(0,1,0): // 0M0 -> M1M
                    traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, nextGridSize, gridOffset + nextGridSize * ivec3(0,1,0));
                    currOctant = nextNode(tm.x, t1.y, tm.z, OCTANT_IDX(1,1,0), OCTANT_INVALID, OCTANT_IDX(0,1,1));
                    break;
                
                case OCTANT_IDX(0,1,1): // 0MM -> M11
                    traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, nextGridSize, gridOffset + nextGridSize * ivec3(0,1,1));
                    currOctant = nextNode(tm.x, t1.y, t1.z, OCTANT_IDX(1,1,1), OCTANT_INVALID, OCTANT_INVALID);
                    break;
                
                case OCTANT_IDX(1,0,0): // M00 -> 1MM
                    traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, nextGridSize, gridOffset + nextGridSize * ivec3(1,0,0));
                    currOctant = nextNode(t1.x, tm.y, tm.z, OCTANT_INVALID, OCTANT_IDX(1,1,0), OCTANT_IDX(1,0,1));
                    break;
            
                case OCTANT_IDX(1,0,1): // M0M -> 1M1
                    traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, nextGridSize, gridOffset + nextGridSize * ivec3(1,0,1));
                    currOctant = nextNode(t1.x, tm.y, t1.z, OCTANT_INVALID, OCTANT_IDX(1,1,1), OCTANT_INVALID);
                    break;
                
                case OCTANT_IDX(1,1,0): // MM0 -> 11M
                    traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, nextGridSize, gridOffset + nextGridSize * ivec3(1,1,0));
                    currOctant = nextNode(t1.x, t1.y, tm.z, OCTANT_INVALID, OCTANT_INVALID, OCTANT_IDX(1,1,1));
                    break;
                
                case OCTANT_IDX(1,1,1): // MMM -> 111
                    traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, nextGridSize, gridOffset + nextGridSize * ivec3(1,1,1));
                    currOctant = 8;
                    break; // It is only possible to exit from this node.

                default:
                    break; // shouldnt happen...
                }
            } while (currOctant < OCTANT_INVALID && count < 4);

            // push traversed children to stack in reverse order
            for (int i = count - 1; i >= 0; --i) {
                stack[stackHead++] = traversedChildren[i];
                if (stackHead >= stackLength) {
                    // do we ignore or stack overflow?
                    break; // ignore
                    // return false; // stack overflow
                }
            }
        }
    }
    return false;
}

// bool rayParameter(Ray ray, vec3 boundMin, vec3 boundMax, out RayIntersection intersection) {
//     ray.origin -= boundMin;
//     boundMax -= boundMin;
//     boundMin -= boundMin;

//     vec3 dim = boundMax - boundMin;
//     int a = 0;
    
//     if (ray.direction.x < 0.0) {
//         ray.origin.x = dim.x - ray.origin.x;
//         ray.direction.x = -ray.direction.x;
//         ray.inverseDirection.x = -ray.inverseDirection.x;
//         a |= X_BIT;
//     }
//     if (ray.direction.y < 0.0) {
//         ray.origin.y = dim.y - ray.origin.y;
//         ray.direction.y = -ray.direction.y;
//         ray.inverseDirection.y = -ray.inverseDirection.y;
//         a |= Y_BIT;
//     }
//     if (ray.direction.z < 0.0) {
//         ray.origin.z = dim.z - ray.origin.z;
//         ray.direction.z = -ray.direction.z;
//         ray.inverseDirection.z = -ray.inverseDirection.z;
//         a |= Z_BIT;
//     }

//     vec3 rt0 = (boundMin - ray.origin) * ray.inverseDirection;
//     vec3 rt1 = (boundMax - ray.origin) * ray.inverseDirection;

//     if (max(max(rt0.x, rt0.y), rt0.z) < min(min(rt1.x, rt1.y), rt1.z)) {
//         const int maxLevel = octreeLevel;
//         const int stackLength = 32;
//         int stackHead = 0;
//         StackNode stack[stackLength];
//         stack[stackHead++] = StackNode(0, 0, 0, 0, boundMin, boundMax);

//         uint node;
//         int childIndex;
//         int currOctant;
//         int count;
//         int octantOffset;
//         int nextLevel;
//         int nextIndex;
//         vec3 b0, b1, bm, t0, t1, tm;
//         StackNode item;
//         StackNode traversedChildren[4]; // Ray can't intersect more than 4 children.

//         while (stackHead > 0) {
//             item = stack[--stackHead];
//             b0 = item.b0;
//             b1 = item.b1;
//             t0 = (b0 - ray.origin) * ray.inverseDirection;
//             t1 = (b1 - ray.origin) * ray.inverseDirection;

//             if (item.level > maxLevel) {
//                 break;
//             }

//             if (t1.x < 0.0 || t1.y < 0.0 || t1.z < 0.0)
//                 continue;
            
//             node = imageLoad(octreeChildIndexStorage, item.nodeIndex).r;

//             if (!isNodeOccupied(node)) {
//                 continue;
//             }

//             childIndex = int(node & CHILD_INDEX_MASK);

//             if (childIndex == 0) {
//                 continue;
//             }

//             if (item.level == maxLevel) {
//                 if (processLeaf(item.nodeIndex, ray, b0, b1, a, intersection)) {
//                     return true;
//                 }
//                 continue;
//             }

//             tm = (t0 + t1) * 0.5;
//             bm = (b0 + b1) * 0.5;

//             count = 0;
//             currOctant = firstNode(t0, tm);

//             do {
//                 octantOffset = currOctant ^ a; // xor
//                 nextLevel = item.level + 1;
//                 nextIndex = childIndex + octantOffset;

//                 switch (currOctant) {
//                 case OCTANT_IDX(0,0,0): // 000 -> MMM
//                     traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, 0, vec3(b0.x, b0.y, b0.z), vec3(bm.x, bm.y, bm.z));
//                     currOctant = nextNode(tm.x, tm.y, tm.z, OCTANT_IDX(1,0,0), OCTANT_IDX(0,1,0), OCTANT_IDX(0,0,1));
//                     break;
                
//                 case OCTANT_IDX(0,0,1): // 00M -> MM1
//                     traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, 0, vec3(b0.x, b0.y, bm.z), vec3(bm.x, bm.y, b1.z));
//                     currOctant = nextNode(tm.x, tm.y, t1.z, OCTANT_IDX(1,0,1), OCTANT_IDX(0,1,1), OCTANT_INVALID);
//                     break;
                
//                 case OCTANT_IDX(0,1,0): // 0M0 -> M1M
//                     traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, 0, vec3(b0.x, bm.y, b0.z), vec3(bm.x, b1.y, bm.z));
//                     currOctant = nextNode(tm.x, t1.y, tm.z, OCTANT_IDX(1,1,0), OCTANT_INVALID, OCTANT_IDX(0,1,1));
//                     break;
                
//                 case OCTANT_IDX(0,1,1): // 0MM -> M11
//                     traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, 0, vec3(b0.x, bm.y, bm.z), vec3(bm.x, b1.y, b1.z));
//                     currOctant = nextNode(tm.x, t1.y, t1.z, OCTANT_IDX(1,1,1), OCTANT_INVALID, OCTANT_INVALID);
//                     break;
                
//                 case OCTANT_IDX(1,0,0): // M00 -> 1MM
//                     traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, 0, vec3(bm.x, b0.y, b0.z), vec3(b1.x, bm.y, bm.z));
//                     currOctant = nextNode(t1.x, tm.y, tm.z, OCTANT_INVALID, OCTANT_IDX(1,1,0), OCTANT_IDX(1,0,1));
//                     break;
            
//                 case OCTANT_IDX(1,0,1): // M0M -> 1M1
//                     traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, 0, vec3(bm.x, b0.y, bm.z), vec3(b1.x, bm.y, b1.z));
//                     currOctant = nextNode(t1.x, tm.y, t1.z, OCTANT_INVALID, OCTANT_IDX(1,1,1), OCTANT_INVALID);
//                     break;
                
//                 case OCTANT_IDX(1,1,0): // MM0 -> 11M
//                     traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, 0, vec3(bm.x, bm.y, b0.z), vec3(b1.x, b1.y, bm.z));
//                     currOctant = nextNode(t1.x, t1.y, tm.z, OCTANT_INVALID, OCTANT_INVALID, OCTANT_IDX(1,1,1));
//                     break;
                
//                 case OCTANT_IDX(1,1,1): // MMM -> 111
//                     traversedChildren[count++] = StackNode(nextLevel, nextIndex, octantOffset, 0, vec3(bm.x, bm.y, bm.z), vec3(b1.x, b1.y, b1.z));
//                     currOctant = 8;
//                     break; // It is only possible to exit from this node.

//                 default:
//                     break; // shouldnt happen...
//                 }
//             } while (currOctant < OCTANT_INVALID && count < 4);

//             // push traversed children to stack in reverse order
//             for (int i = count - 1; i >= 0; --i) {
//                 stack[stackHead++] = traversedChildren[i];
//                 if (stackHead >= stackLength) {
//                     // do we ignore or stack overflow?
//                     break; // ignore
//                     // return false; // stack overflow
//                 }
//             }
//         }
//     }
//     return false;
// }

// bool traceRay(Ray ray, vec3 boundMin, vec3 boundMax, out RayIntersection intersection) {
    
//     float t0, t1, intersectionDistance;
    
//     if (!intersectBox(ray, boundMin, boundMax, t0, t1)) {
//         return false; // does not intersect root
//     }

//     intersectionDistance = t0 < 0.0 ? t1 : t0;

//     const uint rootNode = imageLoad(octreeChildIndexStorage, 0).r;
//     const int stackLength = 64;
//     int stackHead = 0;
//     StackNode stack[stackLength];
//     stack[stackHead++] = StackNode(0, 0, 0, rootNode, boundMin, boundMax);

//     while (stackHead > 0) {
//         StackNode item = stack[--stackHead];
//         if (item.level > octreeLevel) {
//             break;
//         }
        
//         if (item.level == octreeLevel) {
//             uvec4 octreeData = imageLoad(octreeDataStorageBuffer, item.nodeIndex).rgba;
//             intersection.intersectionDistance = intersectionDistance;
//             intersection.intersectionColour = unpackUnorm4x8(octreeData[0]).rgb;//imageLoad(octreeAlbedoStorage, int(nodeIndex)).rgb;
//             intersection.intersectionNormal = unpackUnorm2x16(octreeData[1]).xyy;//imageLoad(octreeNormalStorage, int(nodeIndex)).xyz;
//             return true;
//         }
        
//         vec3 boundExtent = (item.b1 - item.b0) * 0.5;
//         vec3 boundCenter = item.b0 + boundExtent;

//         // bool flag = false;
//         // vec3 axisPlaneIntersections = vec3(INFINITY);
//         // flag |= rayPlaneIntersection(ray, boundCenter, vec3(1,0,0), axisPlaneIntersections.x);
//         // flag |= rayPlaneIntersection(ray, boundCenter, vec3(0,1,0), axisPlaneIntersections.y);
//         // flag |= rayPlaneIntersection(ray, boundCenter, vec3(0,0,1), axisPlaneIntersections.z);

//         // // intersectionDistance = t0 < 0.0 ? t1 : t0;
//         // vec3 intersectionPoint = ray.origin + ray.direction * intersectionDistance;
//         // octant.x = intersectionPoint.x < boundCenter.x ? 0 : 1;
//         // octant.y = intersectionPoint.y < boundCenter.y ? 0 : 1;
//         // octant.z = intersectionPoint.z < boundCenter.z ? 0 : 1;

//         // Only possible for ray to traverse at most 4 sub children.
//         int traversalIndex = 0;
//         StackNode traversalNodes[4];
//         float traversalDistance[] = float[4](INFINITY,INFINITY,INFINITY,INFINITY);

//         for (int i = 0; i < 8; i++) {
//             int childIndex = getChildIndex(item.node, i);
//             uint childNode = imageLoad(octreeChildIndexStorage, childIndex).r;
//             if (!isNodeOccupied(childNode)) {
//                 continue;
//             }

//             ivec3 octant = getOctantCoord(i);
//             vec3 childBoundMin = item.b0 + boundExtent * octant;
//             vec3 childBoundMax = boundCenter + boundExtent * octant;
//             if (!intersectBox(ray, childBoundMin, childBoundMax, t0, t1)) {
//                 continue; // does not intersect child node
//             }

//             traversalNodes[traversalIndex] = StackNode(item.level + 1, childIndex, 0, childNode, childBoundMin, childBoundMax);
//             traversalDistance[traversalIndex] = t0 < 0.0 ? t1 : t0;
//             traversalIndex++;
//         }
//         if (traversalDistance[0] > traversalDistance[1]) {
//             float tempDist = traversalDistance[0]; traversalDistance[0] = traversalDistance[1]; traversalDistance[1] = tempDist;
//             StackNode tempNode = traversalNodes[0]; traversalNodes[0] = traversalNodes[1]; traversalNodes[1] = tempNode;
//         }
//         if (traversalDistance[2] > traversalDistance[3]) {
//             float tempDist = traversalDistance[2]; traversalDistance[2] = traversalDistance[3]; traversalDistance[3] = tempDist;
//             StackNode tempNode = traversalNodes[2]; traversalNodes[2] = traversalNodes[3]; traversalNodes[3] = tempNode;
//         }
//         if (traversalDistance[0] > traversalDistance[2]) {
//             float tempDist = traversalDistance[0]; traversalDistance[0] = traversalDistance[2]; traversalDistance[2] = tempDist;
//             StackNode tempNode = traversalNodes[0]; traversalNodes[0] = traversalNodes[2]; traversalNodes[2] = tempNode;
//         }
//         if (traversalDistance[1] > traversalDistance[3]) {
//             float tempDist = traversalDistance[1]; traversalDistance[1] = traversalDistance[3]; traversalDistance[3] = tempDist;
//             StackNode tempNode = traversalNodes[1]; traversalNodes[1] = traversalNodes[3]; traversalNodes[3] = tempNode;
//         }
//         if (traversalDistance[1] > traversalDistance[2]) {
//             float tempDist = traversalDistance[1]; traversalDistance[1] = traversalDistance[2]; traversalDistance[2] = tempDist;
//             StackNode tempNode = traversalNodes[1]; traversalNodes[1] = traversalNodes[2]; traversalNodes[2] = tempNode;
//         }

//         if (!isinf(traversalDistance[0])) {
//             intersectionDistance = traversalDistance[0];
//         }
//         for (int i = 0; i < 4; i++) {
//             if (isinf(traversalDistance[i])) {
//                 break;
//             }

//             stack[stackHead++] = traversalNodes[i];
//         }
//     }

//     return false;
// }

void main() {
    outAlbedo = vec3(0.0, 0.0, 0.0);
    outNormal = vec2(INFINITY);
    outRoughness = 1.0;
    outMetalness = 0.0;
    outAmbientOcclusion = 0.0;
    outIrradiance = vec3(0.0);
    outReflection = vec3(0.0);
    gl_FragDepth = gl_FragCoord.z;
    
    #if 0
    int padding = 50;
    ivec2 offset = ivec2(padding, padding);
    int projectionSize = 250;

    for (int i = 0; i < 3; i++) {
        if (
            gl_FragCoord.x >= offset.x && 
            gl_FragCoord.y >= offset.y && 
            gl_FragCoord.x < offset.x + projectionSize && 
            gl_FragCoord.y < offset.y + projectionSize) {

            ivec2 coord = ivec2(vec2(gl_FragCoord.xy - offset.xy) / projectionSize * voxelGridSize);
            outAlbedo[i] = imageLoad(axisProjectionMap, coord)[i];
            return;
        }

        offset.x += projectionSize + padding;
    }
    #endif

    const vec3 axisScale = vec3(-1,1,1);
    Ray ray = createRay(fs_in.vertexTexture, cameraPosition, cameraRays);
    ray.direction *= axisScale;
    ray.inverseDirection *= axisScale;
    
    // vec3 boxMin = (voxelGridCenter - vec3(voxelGridSize * voxelGridScale * 0.5));
    // vec3 boxMax = (voxelGridCenter + vec3(voxelGridSize * voxelGridScale * 0.5));
    // vec3 boxMin = getVoxelWorldPosition(ivec3(0, 0, 0), octreeLevel);// - (0.5 * voxelGridScale);;
    // vec3 boxMax = getVoxelWorldPosition(ivec3(voxelGridSize - 1), octreeLevel);// + (0.5 * voxelGridScale);;

    RayIntersection intersection;
    if (!rayParameter(ray, intersection)) {
        discard;
    }

    vec3 intersectionPoint = ray.origin + ray.direction * intersection.intersectionDistance;
    vec4 projectedPosition = viewProjectionMatrix * vec4(intersectionPoint, 1.0);
    projectedPosition.xyz /= projectedPosition.w;
    if (projectedPosition.z < -1.0) discard;
    gl_FragDepth = projectedPosition.z * 0.5 + 0.5;
    
    outAlbedo = intersection.intersectionColour;
    outNormal = intersection.intersectionNormal.xy;
}