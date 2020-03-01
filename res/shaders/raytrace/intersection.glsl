
#ifndef INTERSECTION_GLSL
#define INTERSECTION_GLSL

#include "raytrace/raytrace.glsl"

#define PRIM_COST 1.5
#define NODE_COST 1.0

#define STACKLESS_TRAVERSAL 0

bool rayIntersectsBox(const Ray ray, const vec3 boxMin, const vec3 boxMax, out float t0, out float t1) {
    vec3 tbot = ray.inverseDirection * (boxMin - ray.origin);
    vec3 ttop = ray.inverseDirection * (boxMax - ray.origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);
    return t1 >= max(t0, 0.0);
}

bool rayIntersectsBox(const Ray ray, const AxisAlignedBB box, out float t0, out float t1) {
    return rayIntersectsBox(ray, box.minBound, box.maxBound, t0, t1);
}

bool rayIntersectsBox(const Ray ray, const float xmin, const float ymin, const float zmin, const float xmax, const float ymax, const float zmax, out float t0, out float t1) {
    return rayIntersectsBox(ray, vec3(xmin, ymin, zmin), vec3(xmax, ymax, zmax), t0, t1);
}

AxisAlignedBB getNodeBounds(in BVHNode node) {
    AxisAlignedBB bound;
    bound.minBound.x = node.xmin;
    bound.minBound.y = node.ymin;
    bound.minBound.z = node.zmin;
    bound.maxBound.x = node.xmax;
    bound.maxBound.y = node.ymax;
    bound.maxBound.z = node.zmax;
    return bound;
}

#define isLeaf(node) \
    ((((node).primitiveCount_splitAxis_flags) & 0x1) == 1)

#define getSplitAxis(node) \
    (((node).primitiveCount_splitAxis_flags >> 1) & 0x3)

#define getPrimitiveCount(node) \
    (((node).primitiveCount_splitAxis_flags >> 3) & 0x1FFFFFFF)

#define getPrimitiveOffset(node) \
    ((node).dataOffset) // Node is assumed to be a leaf.

#define getParent(node) \
    ((node).parentIndex)

#define getLeftChild(nodeIndex, node) \
    ((nodeIndex) + 1)

#define getRightChild(nodeIndex, node) \
    ((node).dataOffset) // Node is assumed to not be a leaf.

#define getSibling(nodeIndex, parentIndex, parent) \
    (((nodeIndex) != getLeftChild((parentIndex), (parent))) ? \
        (getLeftChild((parentIndex), (parent))) : \
        (getRightChild((parentIndex), (parent))))

#define getNearChild(ray, nodeIndex, node) \
    (((ray).direction[getSplitAxis((node))] >= 0.0) ? getLeftChild((nodeIndex), (node)) : getRightChild((nodeIndex), (node)))

#define getFarChild(ray, nodeIndex, node) \
    (((ray).direction[getSplitAxis(node)] >= 0.0) ? getRightChild((nodeIndex), (node)) : getLeftChild((nodeIndex), (node)))

bool rayIntersectsNode(in Ray ray, in BVHNode node, inout float dist) {
    float t0, t1, t;
    if (rayIntersectsBox(ray, node.xmin, node.ymin, node.zmin, node.xmax, node.ymax, node.zmax, t0, t1)) {
        t = max(0.0, t0);
        if (t < dist) {
            dist = t;
            return true;
        }
    }
    return false;
}

// Moller Trumbore triangle intersection test
bool rayIntersectsTriangle(in Ray ray, in vec3 v0, in vec3 v1, in vec3 v2, bool cullBackface, inout float dist, out vec3 barycentric) {
    #define _EPS 1e-6
    vec3 e1, e2, h, s, q;
    float a,f,u,v;
    e1 = v1 - v0;
    e2 = v2 - v0;
    h = cross(ray.direction, e2);
    a = dot(e1, h);
    if (cullBackface && a < 0.0)
        return false;
    if (abs(a) < _EPS)
        return false;
    f = 1.0 / a;
    s = ray.origin - v0;
    u = f * dot(s, h);
    if (u < 0.0 || u > 1.0)
        return false;
    q = cross(s, e1);
    v = f * dot(ray.direction, q);
    if (v < 0.0 || u + v > 1.0)
        return false;
    float t = f * dot(e2, q);
    if (t > _EPS && t < dist) {
        dist = t;
        barycentric = vec3(1.0 - u - v, u, v);
        return true;
    } else {
        return false;
    }
    #undef _EPS
}

bool occlusionRayIntersectsNodeTriangles(in Ray ray, in BVHNode node, in bool cullBackface, inout IntersectionInfo intersection) {
    uint primitiveOffset = getPrimitiveOffset(node);
    uint primitiveCount = getPrimitiveCount(node);

    uint startIndex = primitiveOffset;
    uint endIndex = startIndex + primitiveCount;

    Vertex v0, v1, v2;
    for (uint i = startIndex; i < endIndex; ++i) {
        uint ref = bvhReferences[i];
        getTriangleVertices(ref, v0, v1, v2);

        if (rayIntersectsTriangle(ray, v0.position, v1.position, v2.position, cullBackface, intersection.dist, intersection.barycentric)) {
            intersection.triangleIndex = ref;
            intersection.v0 = v0;
            intersection.v1 = v1;
            intersection.v2 = v2;
            return true;
        }
    }

    return false;
}

bool sampleRayIntersectsNodeTriangles(in Ray ray, in BVHNode node, in bool cullBackface, inout IntersectionInfo intersection) {
    uint primitiveOffset = getPrimitiveOffset(node);
    uint primitiveCount = getPrimitiveCount(node);

    uint startIndex = primitiveOffset;
    uint endIndex = startIndex + primitiveCount;

    bool hitFound = false;
    Vertex v0, v1, v2;
    for (uint i = startIndex; i < endIndex; ++i) {
        uint ref = bvhReferences[i];
        getTriangleVertices(ref, v0, v1, v2);

        if (rayIntersectsTriangle(ray, v0.position, v1.position, v2.position, cullBackface, intersection.dist, intersection.barycentric)) {
            intersection.triangleIndex = ref;
            intersection.v0 = v0;
            intersection.v1 = v1;
            intersection.v2 = v2;
            hitFound = true;
        }
    }

    return hitFound;
}

#if STACKLESS_TRAVERSAL

// Finds any ray intersection and immediately stops. Not guaranteed to be the closest, but is faster than a sample ray
bool occlusionRayIntersectsBVH(in Ray ray, in bool cullBackface, inout IntersectionInfo intersection) {
    uint lastNodeIndex, currentNodeIndex, parentNodeIndex;
    uint nearChild, farChild, tryChild;
    float nodeDist;
    
    lastNodeIndex = currentNodeIndex = 0;

    BVHNode currentNode = bvhNodes[currentNodeIndex];

    #define SET_CURRENT_NODE(newNode) \
        currentNodeIndex = (newNode); \
        currentNode = bvhNodes[currentNodeIndex]; \

    if (!isLeaf(currentNode)) {
        SET_CURRENT_NODE(getNearChild(ray, currentNodeIndex, currentNode));
    }

    // TODO: is immediately traversing down to the deepest node the camera is inside faster?

    uint iter = 0;
    while (currentNodeIndex != 0xFFFFFFFF) {
        // cost += NODE_COST;
        
        currentNode = bvhNodes[currentNodeIndex];
        if (isLeaf(currentNode)) {
            if (occlusionRayIntersectsNodeTriangles(ray, currentNode, cullBackface, intersection)) {
                return true;
            }

            lastNodeIndex = currentNodeIndex;
            SET_CURRENT_NODE(getParent(currentNode));
            continue;
        }

        nearChild = getNearChild(ray, currentNodeIndex, currentNode);
        farChild = getFarChild(ray, currentNodeIndex, currentNode);

        if (lastNodeIndex == farChild) {
            lastNodeIndex = currentNodeIndex;
            SET_CURRENT_NODE(getParent(currentNode));
            continue;
        }

        nodeDist = intersection.dist;

        parentNodeIndex = getParent(currentNode);
        tryChild = (lastNodeIndex == parentNodeIndex) ? nearChild : farChild;
        if (rayIntersectsNode(ray, currentNode, nodeDist)) {
            lastNodeIndex = currentNodeIndex;
            SET_CURRENT_NODE(tryChild);
        } else {
            if (lastNodeIndex == parentNodeIndex) {
                lastNodeIndex = nearChild;
            } else {
                lastNodeIndex = currentNodeIndex;
                SET_CURRENT_NODE(parentNodeIndex);
            }
        }
    }

    return false;
}

// Finds the absolute closest ray intersection
bool sampleRayIntersectsBVH(in Ray ray, in bool cullBackface, inout IntersectionInfo intersection) {
    uint lastNodeIndex, currentNodeIndex, parentNodeIndex;
    uint nearChild, farChild, tryChild;
    float nodeDist;
    
    lastNodeIndex = currentNodeIndex = 0;

    BVHNode currentNode = bvhNodes[currentNodeIndex];

    #define SET_CURRENT_NODE(newNode) \
        currentNodeIndex = (newNode); \
        currentNode = bvhNodes[currentNodeIndex]; \

    if (!isLeaf(currentNode)) {
        SET_CURRENT_NODE(getNearChild(ray, currentNodeIndex, currentNode));
    }

    // TODO: is immediately traversing down to the deepest node the camera is inside faster?

    bool intersectionFound = false;

    while (currentNodeIndex != 0xFFFFFFFF) {
        // cost += NODE_COST;
        
        currentNode = bvhNodes[currentNodeIndex];
        if (isLeaf(currentNode)) {
            if (sampleRayIntersectsNodeTriangles(ray, currentNode, cullBackface, intersection)) {
                intersectionFound = true;
            }

            lastNodeIndex = currentNodeIndex;
            SET_CURRENT_NODE(getParent(currentNode));
            continue;
        }

        nearChild = getNearChild(ray, currentNodeIndex, currentNode);
        farChild = getFarChild(ray, currentNodeIndex, currentNode);

        if (lastNodeIndex == farChild) {
            lastNodeIndex = currentNodeIndex;
            SET_CURRENT_NODE(getParent(currentNode));
            continue;
        }

        nodeDist = intersection.dist;

        parentNodeIndex = getParent(currentNode);
        tryChild = (lastNodeIndex == parentNodeIndex) ? nearChild : farChild;
        if (rayIntersectsNode(ray, currentNode, nodeDist)) {
            lastNodeIndex = currentNodeIndex;
            SET_CURRENT_NODE(tryChild);
        } else {
            if (lastNodeIndex == parentNodeIndex) {
                lastNodeIndex = nearChild;
            } else {
                lastNodeIndex = currentNodeIndex;
                SET_CURRENT_NODE(parentNodeIndex);
            }
        }
    }

    return intersectionFound;
}



#else // !STACKLESS_TRAVERSAL

#define STACK_DEPTH 32

#define GET_NODE(srcIdx, i, n) \
    i = srcIdx; \
    n = bvhNodes[i];

#define PUSH_STACK(i, n) \
    indexStack[stackHead] = i; \
    /*nodeStack[stackHead] = n;*/ \
    ++stackHead;

#define POP_STACK(i, n) \
    --stackHead; \
    i = indexStack[stackHead]; \
    /*n = nodeStack[stackHead];*/ \
    n = bvhNodes[i];

#define CHECK_STACK_DEPTH() \
    //if (stackHead == STACK_DEPTH) continue;

// Finds any ray intersection and immediately stops. Not guaranteed to be the closest, but is faster than a sample ray
bool occlusionRayIntersectsBVH(in Ray ray, in bool cullBackface, inout IntersectionInfo intersection) {
    uint stackHead = 0;
    uint indexStack[STACK_DEPTH];
    // BVHNode nodeStack[STACK_DEPTH];

    uint index, leftIndex, rightIndex;
    BVHNode node, leftNode, rightNode;

    GET_NODE(0, index, node); // get root
    PUSH_STACK(index, node); // push root

    float leftDist, rightDist;
    bool leftHit, rightHit;
    
    while (stackHead > 0) {
        // cost += NODE_COST;
        POP_STACK(index, node);

        if (isLeaf(node)) {
            if (occlusionRayIntersectsNodeTriangles(ray, node, cullBackface, intersection)) {
                return true; // first intersection, immediately return
            }
            continue;
        }

        CHECK_STACK_DEPTH();

        GET_NODE(getLeftChild(index, node), leftIndex, leftNode);
        GET_NODE(getRightChild(index, node), rightIndex, rightNode);

        leftDist = rightDist = intersection.dist;
        leftHit = rayIntersectsNode(ray, leftNode, leftDist);
        rightHit = rayIntersectsNode(ray, rightNode, rightDist);

        if (!leftHit && !rightHit) {
            continue;
        }

        if (leftHit && rightHit) {
            if (leftDist < rightDist) {
                PUSH_STACK(rightIndex, rightNode);
                PUSH_STACK(leftIndex, leftNode);
            } else {
                PUSH_STACK(leftIndex, leftNode);
                PUSH_STACK(rightIndex, rightNode);
            }
        } else if (leftHit) {
            PUSH_STACK(leftIndex, leftNode);
        } else {
            PUSH_STACK(rightIndex, rightNode);
        }
    }

    return false;
}

// Finds the absolute closest ray intersection
bool sampleRayIntersectsBVH(in Ray ray, in bool cullBackface, inout IntersectionInfo intersection) {
    uint stackHead = 0;
    uint indexStack[STACK_DEPTH];
    // BVHNode nodeStack[STACK_DEPTH];

    uint index, leftIndex, rightIndex;
    BVHNode node, leftNode, rightNode;

    GET_NODE(0, index, node); // get root
    PUSH_STACK(index, node); // push root

    bool hitFound = false;
    float leftDist, rightDist;
    bool leftHit, rightHit;
    
    while (stackHead > 0) {
        // cost += NODE_COST;
        POP_STACK(index, node);

        if (isLeaf(node)) {
            if (sampleRayIntersectsNodeTriangles(ray, node, cullBackface, intersection)) {
                hitFound = true;
            }
            continue;
        }

        CHECK_STACK_DEPTH();

        GET_NODE(getLeftChild(index, node), leftIndex, leftNode);
        GET_NODE(getRightChild(index, node), rightIndex, rightNode);

        leftDist = rightDist = intersection.dist;
        leftHit = rayIntersectsNode(ray, leftNode, leftDist);
        rightHit = rayIntersectsNode(ray, rightNode, rightDist);

        if (!leftHit && !rightHit) {
            continue;
        }

        if (leftHit && rightHit) {
            if (leftDist < rightDist) {
                PUSH_STACK(rightIndex, rightNode);
                PUSH_STACK(leftIndex, leftNode);
            } else {
                PUSH_STACK(leftIndex, leftNode);
                PUSH_STACK(rightIndex, rightNode);
            }
        } else if (leftHit) {
            PUSH_STACK(leftIndex, leftNode);
        } else {
            PUSH_STACK(rightIndex, rightNode);
        }
    }

    return hitFound;
}

#undef STACK_DEPTH
#undef GET_NODE
#undef PUSH_STACK
#undef POP_STACK
#undef CHECK_STACK_DEPTH

#endif

#endif