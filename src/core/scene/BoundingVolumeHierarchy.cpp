#include "BoundingVolumeHierarchy.h"
#include "core/renderer/geometry/Mesh.h"
#include "core/renderer/ScreenRenderer.h"
#include "core/scene/Scene.h"



const BoundingVolumeHierarchy::PrimitiveReference BoundingVolumeHierarchy::INVALID_REFERENCE = -1;
const int BoundingVolumeHierarchy::BUCKET_COUNT = 12;
const double BoundingVolumeHierarchy::NODE_INTERSECT_COST = 1.0;
const double BoundingVolumeHierarchy::PRIM_INTERSECT_COST = 1.5;

#define BUCKET_INDEX(coord, axis, bounds) min((int)(BoundingVolumeHierarchy::BUCKET_COUNT * bounds.getUnitCoordinate(coord, (int)axis)), BUCKET_COUNT - 1)

BVHNode* BVHNode::leaf(uint32_t offset, uint32_t count, AxisAlignedBB& bound) {
	BVHNode* node = new  BVHNode();
	node->m_primitiveOffset = offset;
	node->m_primitiveCount = count;
	node->m_bound = bound;
	node->m_left = NULL;
	node->m_right = NULL;
	node->m_splitAxis = 0;
	return node;
}


BVHNode* BVHNode::interior(int axis, BVHNode* left, BVHNode* right) {
	BVHNode* node = new BVHNode();
	node->m_primitiveOffset = 0;
	node->m_primitiveCount = 0; // not leaf
	node->m_bound = AxisAlignedBB::combine(left->getBound(), right->getBound());
	node->m_left = left;
	node->m_right = right;
	node->m_splitAxis = axis;
	return node;
}


uint32_t BVHNode::getPrimitiveOffset() const {
	return m_primitiveOffset;
}


uint32_t BVHNode::getPrimitiveCount() const {
	return m_primitiveCount;
}


AxisAlignedBB BVHNode::getBound() const {
	return m_bound;
}


BVHNode* BVHNode::getLeft() const {
	return m_left;
}


BVHNode* BVHNode::getRight() const {
	return m_right;
}


int BVHNode::getSplitAxis() const {
	return m_splitAxis;
}


bool BVHNode::isLeaf() const {
	return m_primitiveCount > 0;
}


BVHNode::~BVHNode() {
	if (m_left != NULL) delete m_left;
	if (m_right != NULL) delete m_right;
}




BoundingVolumeHierarchy::BoundingVolumeHierarchy(BVHNode* root, PrimitiveReferenceList& primitiveReferences, BufferNodeList& linearBuffer) :
	m_root(root), 
	m_primitiveReferences(primitiveReferences),
	m_linearBuffer(linearBuffer) {
}

BoundingVolumeHierarchy::~BoundingVolumeHierarchy() {
	if (m_root != NULL) delete m_root;
	if (m_debugMesh != NULL) delete m_debugMesh;
	if (m_debugShader != NULL) delete m_debugShader;
}

const BVHNode* BoundingVolumeHierarchy::getRoot() const {
	return m_root;
}

const BoundingVolumeHierarchy::PrimitiveReferenceList BoundingVolumeHierarchy::getPrimitiveList() const {
	return m_primitiveReferences;
}

const BoundingVolumeHierarchy::BufferNodeList BoundingVolumeHierarchy::getLinearBuffer() const {
	return m_linearBuffer;
}

void BoundingVolumeHierarchy::renderDebug(double dt, double partialTicks) {
	ShaderProgram* debugShader = this->debugShader();

	Engine::screenRenderer()->unbindFramebuffer();
	ShaderProgram::use(debugShader);
	
	Engine::scene()->applyUniforms(debugShader);
	Engine::screenRenderer()->drawFullscreenQuad();

	ShaderProgram::use(NULL);
	Engine::screenRenderer()->bindFramebuffer();
}

void BoundingVolumeHierarchy::buildDebugMesh() {
	std::vector<std::pair<uint32_t, uint32_t>> levelRenderSections;

	Mesh::Builder* debugMeshBuilder = new Mesh::Builder();

	this->recursiveBuildDebugMesh(m_root, 0, debugMeshBuilder, levelRenderSections);

	debugMeshBuilder->build(&m_debugMesh, true);
}

Mesh* BoundingVolumeHierarchy::getDebugMesh() const {
	return m_debugMesh;
}

void BoundingVolumeHierarchy::fillTrianglePrimitiveList(uint64_t triangleOffset, uint64_t triangleCount, const std::vector<Mesh::vertex>& vertices, const std::vector<Mesh::triangle>& triangles, PrimitiveInfoList& primitiveInfo) {
	if (triangleCount == -1) {
		triangleCount = triangles.size();
	}

	if (triangleCount == 0 || triangleCount > triangles.size() || triangleCount + triangleOffset > triangles.size()) {
		return;
	}

	uint64_t primitiveInfoOffset = primitiveInfo.size();
	primitiveInfo.resize(primitiveInfoOffset + triangleCount);

	//std::vector<dvec3> triVerts;
	//triVerts.resize(3);

	// TODO: multi-thread this
	for (int i = triangleOffset; i < triangleOffset + triangleCount; ++i) {
		dvec3 v0 = vertices[triangles[i].i0].position;
		dvec3 v1 = vertices[triangles[i].i1].position;
		dvec3 v2 = vertices[triangles[i].i2].position;

		PrimitiveInfo& prim = primitiveInfo[primitiveInfoOffset + i];
		prim.bound = AxisAlignedBB::combine(prim.bound, v0);
		prim.bound = AxisAlignedBB::combine(prim.bound, v1);
		prim.bound = AxisAlignedBB::combine(prim.bound, v2);
		prim.reference = i;
	}
}

BoundingVolumeHierarchy* BoundingVolumeHierarchy::build(std::vector<Mesh::vertex>& vertices, std::vector<Mesh::triangle>& triangles, uint64_t triangleOffset, uint64_t triangleCount) {
	if (vertices.empty() || triangles.empty()) {
		return NULL;
	}

	if (triangleCount == -1) { // underflow max uint
		triangleCount = triangles.size();
	}

	if (triangleOffset + triangleCount > triangles.size()) {
		return NULL;
	}

	constexpr double spatialAllocationPadding = 0.2; // pad 20% more space

	PrimitiveInfoList primitiveInfoList;
	BoundingVolumeHierarchy::fillTrianglePrimitiveList(triangleOffset, triangleCount, vertices, triangles, primitiveInfoList);
	size_t spatialSplitBudget = (int)(primitiveInfoList.size() * spatialAllocationPadding);
	primitiveInfoList.resize(primitiveInfoList.size() + spatialSplitBudget);

	PrimitiveReferenceList sortedPrimitives;
	BufferNodeList linearBuffer;

	sortedPrimitives.reserve(primitiveInfoList.size());

	info("Building BVH for %d triangles\n", primitiveInfoList.size());

	uint64_t t0 = Engine::instance()->getCurrentTime();

	ConstructionContext ctx;
	ctx.spatialSplitBudget = spatialSplitBudget;
	ctx.vertices = vertices;
	ctx.triangles = triangles;
	ctx.primitiveInfoList = primitiveInfoList;
	ctx.sortedPrimitiveList = sortedPrimitives;

	BVHNode* root = BoundingVolumeHierarchy::recursiveBuild(0, primitiveInfoList.size(), 0, TreeSide::Left, ctx);

	uint64_t t1 = Engine::instance()->getCurrentTime();

	int index = 0;
	linearBuffer.resize(ctx.nodeCount);
	// BoundingVolumeHierarchy::flattenTree(root, index, linearBuffer);

	uint64_t t2 = Engine::instance()->getCurrentTime();


	info("Took %.2f msec to build BVH with %d primitives, %d nodes (%d spatial splits), [%d - %d] leaf depth range\n", (t1 - t0) / 1000000.0, sortedPrimitives.size(), ctx.nodeCount, ctx.spatialSplitCount, ctx.minLeafDepth, ctx.maxLeafDepth);
	info("Took %.2f msec to flatten BVH tree\n", (t2 - t1) / 1000000.0);

	return new BoundingVolumeHierarchy(root, sortedPrimitives, linearBuffer);
}

ShaderProgram* BoundingVolumeHierarchy::debugShader() {
	if (m_debugShader == NULL) {
		m_debugShader = new ShaderProgram();
		m_debugShader->addShader(GL_VERTEX_SHADER, "shaders/bvh/debug/vert.glsl");
		m_debugShader->addShader(GL_FRAGMENT_SHADER, "shaders/bvh/debug/frag.glsl");
		Mesh::addVertexInputs(m_debugShader);
		m_debugShader->completeProgram();
	}
	return m_debugShader;
}

int BoundingVolumeHierarchy::flattenTree(BVHNode* node, int& index, BufferNodeList& linearBuffer) {
	BufferNode& bufferNode = linearBuffer[index];
	bufferNode.boundMin = node->getBound().getMin();
	bufferNode.boundMax = node->getBound().getMax();
	bufferNode.splitAxis = node->getSplitAxis();

	int currIndex = index++;

	if (node->isLeaf()) {
		bufferNode.dataOffset = node->getPrimitiveOffset();
		bufferNode.primitiveCount = node->getPrimitiveCount();
	} else {
		const int leftIndex = BoundingVolumeHierarchy::flattenTree(node->getLeft(), index, linearBuffer);
		const int rightIndex = BoundingVolumeHierarchy::flattenTree(node->getRight(), index, linearBuffer);
		bufferNode.dataOffset = rightIndex;
		bufferNode.primitiveCount = 0;
	}

	return currIndex;
}

BVHNode* BoundingVolumeHierarchy::recursiveBuild(uint32_t startIndex, uint32_t endIndex, uint32_t stackDepth, TreeSide side, ConstructionContext& ctx) {
	if (endIndex <= startIndex || startIndex < 0) {
		//info("WTF %d <-> %d\n", startIndex, endIndex);
		return NULL;
	}

	ctx.nodeCount++;

	uint32_t primCount = 0;
	uint32_t nodeMaxPrimCount = 512;

	AxisAlignedBB enclosingBounds;
	for (int i = startIndex; i < endIndex; ++i) {
		if (ctx.primitiveInfoList[i].reference == INVALID_REFERENCE)
			continue;

		enclosingBounds = AxisAlignedBB::combine(enclosingBounds, ctx.primitiveInfoList[i].bound);
		++primCount;
	}

	if (primCount == 0) {
		return NULL;
	}

	if (primCount == 1) {
		return BoundingVolumeHierarchy::initLeaf(startIndex, endIndex, stackDepth, primCount, enclosingBounds, ctx);
	} 
	
	AxisAlignedBB centroidBounds;
	for (int i = startIndex; i < endIndex; ++i) {
		if (ctx.primitiveInfoList[i].reference == INVALID_REFERENCE)
			continue;

		centroidBounds = AxisAlignedBB::combine(centroidBounds, ctx.primitiveInfoList[i].bound.getCenter());
	}

	int enclosingAxis = enclosingBounds.getLargestAxis();
	int centroidAxis = centroidBounds.getLargestAxis();

	if (centroidBounds.isDegenerate() || centroidBounds.getFullExtent(centroidAxis) <= 1e-6) { // bounding box is invalid or planar. We don't know what to do with it, so just create a leaf
		return BoundingVolumeHierarchy::initLeaf(startIndex, endIndex, stackDepth, primCount, enclosingBounds, ctx);
	}

	int midIndex = (endIndex + startIndex) / 2;

	switch (ctx.splitMethod) {
		case SplitMethod::Middle: {
			midIndex = BoundingVolumeHierarchy::partitionMiddle(startIndex, endIndex, centroidBounds, centroidAxis, ctx);
			if (midIndex != startIndex && midIndex != endIndex) {
				break;
			}
		}
		case SplitMethod::EqualCount: {
			midIndex = BoundingVolumeHierarchy::partitionEqualCount(startIndex, endIndex, centroidAxis, ctx);
			break;
		}
		case SplitMethod::SAH:
		default: {
			if (primCount <= 4) {
				midIndex = BoundingVolumeHierarchy::partitionEqualCount(startIndex, endIndex, centroidAxis, ctx);
			} else {
				const double bucketSize = (enclosingBounds.getFullExtent(centroidAxis)) / BUCKET_COUNT;
				const double leafCost = PRIM_INTERSECT_COST * primCount;

				std::vector<Bucket> objectBuckets;
				objectBuckets.resize(BUCKET_COUNT);
				int bucketIndex = 0;
				double minCost = INFINITY;

				BoundingVolumeHierarchy::calculateObjectSplitBucket(startIndex, endIndex, centroidAxis, centroidBounds, enclosingBounds, ctx, bucketIndex, minCost, objectBuckets);

				bool spatialSplit = false;

				if (ctx.spatialSplitCount < ctx.spatialSplitBudget) {
					std::vector<Bucket> spatialBuckets;
					spatialBuckets.resize(BUCKET_COUNT);
					int spatialBucketIndex = 0;
					double spatialMinCost = INFINITY;
					BoundingVolumeHierarchy::calculateSpatialSplitBucket(startIndex, endIndex, enclosingAxis, centroidBounds, enclosingBounds, ctx, spatialBucketIndex, spatialMinCost, spatialBuckets);

					if (spatialMinCost < minCost) {
						spatialSplit = true;
						ctx.spatialSplitCount++;
						bucketIndex = spatialBucketIndex;
						minCost = spatialMinCost;
						
						Bucket& bucket = spatialBuckets[bucketIndex];
						std::vector<PrimitiveInfo> temp;
						temp.resize(bucket.fragments.size());
						
						uint32_t left = startIndex;
						uint32_t right = endIndex - 1;
						for (size_t i = 0; i < bucket.fragments.size(); ++i) {
							temp[i] = ctx.primitiveInfoList[bucket.fragments[i].originalIndex];
						}
						
						for (size_t i = 0; i < bucket.fragments.size(); ++i) {
							PrimitiveFragment& frag = bucket.fragments[i];
							PrimitiveInfo& primInfo = temp[i];
						
							dvec3 v;
							v = frag.splitInfo.bound.getMin();
							v[enclosingAxis] = primInfo.bound.getMin(enclosingAxis);
							AxisAlignedBB lb = AxisAlignedBB(v, frag.splitInfo.bound.getMax());
						
							v = frag.splitInfo.bound.getMax();
							v[enclosingAxis] = primInfo.bound.getMax(enclosingAxis);
							AxisAlignedBB rb = AxisAlignedBB(frag.splitInfo.bound.getMin(), v);
						
							//if () {
						
							ctx.primitiveInfoList[frag.originalIndex] = { frag.splitInfo.reference, lb };
						
							if (side == TreeSide::Left) {
								ctx.primitiveInfoList[right--] = { frag.splitInfo.reference, rb };
							} else {
								ctx.primitiveInfoList[left++] = { frag.splitInfo.reference, rb };
							}
						
							//assert(right >= left);
						
							//}
						}
					}
				}

				if (primCount > nodeMaxPrimCount || minCost < leafCost) {
					if (!spatialSplit) {
						midIndex = BoundingVolumeHierarchy::partitionBucket(startIndex, endIndex, bucketIndex, centroidAxis, centroidBounds, false, ctx);
					} else {
						midIndex = BoundingVolumeHierarchy::partitionBucket(startIndex, endIndex, bucketIndex, enclosingAxis, enclosingBounds, true, ctx);
					}

				} else {
					return BoundingVolumeHierarchy::initLeaf(startIndex, endIndex, stackDepth, primCount, enclosingBounds, ctx);
				}
			}
			break;
		}
	}

	//if (startIndex >= midIndex || endIndex <= midIndex) {
	//	info("??????????????? %d %d %d\n", startIndex, midIndex, endIndex);
	//}

	if (midIndex <= startIndex || midIndex >= endIndex) {
		return BoundingVolumeHierarchy::initLeaf(startIndex, endIndex, stackDepth, primCount, enclosingBounds, ctx);
	}

	BVHNode* leftNode = recursiveBuild(startIndex, midIndex, stackDepth + 1, TreeSide::Left, ctx);
	BVHNode* rightNode = recursiveBuild(midIndex, endIndex, stackDepth + 1, TreeSide::Right, ctx);


	if (leftNode == NULL || rightNode == NULL) {
		return BoundingVolumeHierarchy::initLeaf(startIndex, endIndex, stackDepth, primCount, enclosingBounds, ctx);
	}

	return BVHNode::interior(centroidAxis, leftNode, rightNode);
}

void BoundingVolumeHierarchy::recursiveBuildDebugMesh(BVHNode* node, uint32_t stackDepth, Mesh::Builder* debugMeshBuilder, std::vector<std::pair<uint32_t, uint32_t>>& levelRenderSections) {
	if (node == NULL) {
		return;
	}

	if (!node->isLeaf()) {
		recursiveBuildDebugMesh(node->getLeft(), stackDepth + 1, debugMeshBuilder, levelRenderSections);
		recursiveBuildDebugMesh(node->getRight(), stackDepth + 1, debugMeshBuilder, levelRenderSections);
	} else {
		debugMeshBuilder->createCuboid(node->getBound().getHalfExtent(), node->getBound().getCenter());
	}
}

uint32_t BoundingVolumeHierarchy::partitionMiddle(uint32_t startIndex, uint32_t endIndex, AxisAlignedBB bounds, int axis, ConstructionContext& ctx) {
	const double midCoord = bounds.getCenter(axis);
	PrimitiveInfo* startPtr = &ctx.primitiveInfoList[startIndex];
	PrimitiveInfo* endPtr = &ctx.primitiveInfoList[endIndex - 1];
	PrimitiveInfo* midPtr = std::partition(startPtr, endPtr + 1, [axis, midCoord](const PrimitiveInfo& prim) {
		return prim.bound.getCenter(axis) < midCoord;
	});
	return startIndex + midPtr - startPtr;
}

uint32_t BoundingVolumeHierarchy::partitionEqualCount(uint32_t startIndex, uint32_t endIndex, int axis, ConstructionContext& ctx) {
	int midIndex = (endIndex + startIndex) >> 1; // / 2
	PrimitiveInfo* startPtr = &ctx.primitiveInfoList[startIndex];
	PrimitiveInfo* endPtr = &ctx.primitiveInfoList[endIndex - 1];
	PrimitiveInfo* midPtr = &ctx.primitiveInfoList[midIndex];
	std::nth_element(startPtr, midPtr, endPtr + 1, [axis](const PrimitiveInfo& left, const PrimitiveInfo& right) {
		return left.bound.getCenter(axis) < right.bound.getCenter(axis);
	});
	return midIndex;
}

uint32_t BoundingVolumeHierarchy::partitionBucket(uint32_t startIndex, uint32_t endIndex, int partitionBucket, int axis, AxisAlignedBB bounds, bool spatial, ConstructionContext& ctx) {
	std::vector<PrimitiveInfo> temp;
	temp.reserve(endIndex - startIndex); // slight over-allocation if any invalid references

	// copy all valid primitives to temp array, then invalidate existing primitives
	for (uint32_t i = startIndex; i < endIndex; ++i) {
		if (ctx.primitiveInfoList[i].reference == INVALID_REFERENCE)
			continue;

		temp.push_back(ctx.primitiveInfoList[i]);

		ctx.primitiveInfoList[i].reference = INVALID_REFERENCE;
		ctx.primitiveInfoList[i].bound = AxisAlignedBB();
	}

	assert(!temp.empty());

	uint32_t left = startIndex;
	uint32_t right = endIndex - 1;

	bool flag;
	for (size_t i = 0; i < temp.size(); ++i) {
		int bucketIndex;

		if (spatial) {
			bucketIndex = BUCKET_INDEX(temp[i].bound.getMin(), axis, bounds);
		} else {
			bucketIndex = BUCKET_INDEX(temp[i].bound.getCenter(), axis, bounds);
		}

		flag = bucketIndex <= partitionBucket;

		if (flag) {
			ctx.primitiveInfoList[left++] = temp[i];
		} else {
			ctx.primitiveInfoList[right--] = temp[i];
		}
	}

	// re-adjust after final invocation
	//if (flag) left--;
	//else right++;

	assert(left <= right + 1);

	return (left + right) >> 1; // / 2;
}

bool BoundingVolumeHierarchy::calculateObjectSplitBucket(uint32_t startIndex, uint32_t endIndex, int axis, AxisAlignedBB centroidBounds, AxisAlignedBB enclosingBounds, ConstructionContext& ctx, int& bucketIndex, double& bucketCost, std::vector<Bucket>& buckets) {

	// Add all sortedPrimitives to the appropriate bucket
	for (int i = startIndex; i < endIndex; ++i) {
		if (ctx.primitiveInfoList[i].reference == INVALID_REFERENCE)
			continue;

		int currIndex = BUCKET_INDEX(ctx.primitiveInfoList[i].bound.getCenter(), axis, centroidBounds);
		buckets[currIndex].primCount++;
		buckets[currIndex].bound = buckets[currIndex].bound.extend(ctx.primitiveInfoList[i].bound);
	}

	Bucket leftCumulative[BUCKET_COUNT - 1];
	Bucket rightCumulative[BUCKET_COUNT - 1];

	leftCumulative[0] = buckets[0];
	rightCumulative[BUCKET_COUNT - 2] = buckets[BUCKET_COUNT - 1];

	for (int i = 1, j; i < BUCKET_COUNT - 1; ++i) {
		j = BUCKET_COUNT - (i + 2);
		leftCumulative[i].bound = leftCumulative[i - 1].bound.extend(buckets[i].bound);
		leftCumulative[i].primCount = leftCumulative[i - 1].primCount + buckets[i].primCount;
		rightCumulative[j].bound = rightCumulative[j + 1].bound.extend(buckets[j + 1].bound);
		rightCumulative[j].primCount = rightCumulative[j + 1].primCount + buckets[j + 1].primCount;
	}

	bool flag = false; // true when a lower cost bucket is found

	// Compute cost of each bucket and find the minimum
	for (int i = 1; i < BUCKET_COUNT - 1; ++i) {
		AxisAlignedBB lb = leftCumulative[i].bound;
		int lc = leftCumulative[i].primCount;
		AxisAlignedBB rb = rightCumulative[i].bound;
		int rc = rightCumulative[i].primCount;

		double lp = calculateIntersectProbability(lb, enclosingBounds);
		double rp = calculateIntersectProbability(rb, enclosingBounds);
		double cost = calculateSAHCost(lp, rp, lc, rc);

		if (cost < bucketCost) {
			bucketCost = cost;
			bucketIndex = i;
			flag = true;
		}
	}

	return flag;
}

bool BoundingVolumeHierarchy::calculateSpatialSplitBucket(uint32_t startIndex, uint32_t endIndex, int axis, AxisAlignedBB centroidBounds, AxisAlignedBB enclosingBounds, ConstructionContext& ctx, int& bucketIndex, double& bucketCost, std::vector<Bucket>& buckets) {

	const double bucketSize = enclosingBounds.getFullExtent(axis) / BUCKET_COUNT;

	// Add all sortedPrimitives to the appropriate bucket
	for (int i = startIndex; i < endIndex; ++i) {
		if (ctx.primitiveInfoList[i].reference == INVALID_REFERENCE)
			continue;

		int minIndex = BUCKET_INDEX(ctx.primitiveInfoList[i].bound.getMin(), axis, enclosingBounds);
		int maxIndex = BUCKET_INDEX(ctx.primitiveInfoList[i].bound.getMax(), axis, enclosingBounds);

		buckets[minIndex].enterCount++;
		buckets[maxIndex].exitCount++;

		if (minIndex != maxIndex) {
			for (int j = minIndex; j <= maxIndex; ++j) {
				AxisAlignedBB primBound = ctx.primitiveInfoList[i].bound;

				double leftPlane = enclosingBounds.getMin(axis) + j * bucketSize;
				double rightPlane = leftPlane + bucketSize;

				if (primBound.getMax(axis) < leftPlane || primBound.getMin(axis) > rightPlane) {
					// primitive bound is not within this bucket
					continue;
				}

				dvec3 bmin = primBound.getMin();
				dvec3 bmax = primBound.getMax();
				bmin[axis] = max(bmin[axis], leftPlane);
				bmax[axis] = min(bmax[axis], rightPlane);
				assert(bmin[axis] <= bmax[axis]);

				PrimitiveFragment fragment;
				fragment.splitInfo.bound = AxisAlignedBB(bmin, bmax);
				fragment.splitInfo.reference = ctx.primitiveInfoList[i].reference;
				fragment.originalIndex = i;
				buckets[j].fragments.push_back(fragment);
				buckets[j].bound = AxisAlignedBB::combine(buckets[j].bound, primBound);
			}
		} else { // primitive spans only one bucket
			buckets[minIndex].bound = AxisAlignedBB::combine(buckets[minIndex].bound, ctx.primitiveInfoList[i].bound);
		}
	}
	AxisAlignedBB leftBound[BUCKET_COUNT - 1];
	int leftCount[BUCKET_COUNT - 1];

	AxisAlignedBB rightBound[BUCKET_COUNT - 1];
	int rightCount[BUCKET_COUNT - 1];

	leftBound[0] = buckets[0].bound;
	leftCount[0] = buckets[0].enterCount;
	rightBound[BUCKET_COUNT - 2] = buckets[BUCKET_COUNT - 1].bound;
	rightCount[BUCKET_COUNT - 2] = buckets[BUCKET_COUNT - 1].exitCount;

	for (int i = 1, j; i < BUCKET_COUNT - 1; ++i) {
		j = BUCKET_COUNT - (i + 2);
		leftBound[i] = leftBound[i - 1].extend(buckets[i].bound);
		leftCount[i] = leftCount[i - 1] + buckets[i].enterCount;
		rightBound[j] = rightBound[j + 1].extend(buckets[j + 1].bound);
		rightCount[j] = rightCount[j + 1] + buckets[j + 1].exitCount;
	}

	bool flag = false; // true when a lower cost bucket is found

	// Compute cost of each bucket and find the minimum
	for (int i = 1; i < BUCKET_COUNT - 1; ++i) {
		AxisAlignedBB lb = leftBound[i];
		int lc = leftCount[i];
		AxisAlignedBB rb = rightBound[i];
		int rc = rightCount[i];

		double lp = calculateIntersectProbability(lb, enclosingBounds);
		double rp = calculateIntersectProbability(rb, enclosingBounds);
		double cost = calculateSAHCost(lp, rp, lc, rc);

		if (cost < bucketCost) {
			bucketCost = cost;
			bucketIndex = i;
			flag = true;
		}
	}

	return flag;
}

int BoundingVolumeHierarchy::splitSAH(uint32_t startIndex, uint32_t endIndex, AxisAlignedBB centroidBounds, AxisAlignedBB enclosingBounds, int centroidAxis, int enclosingAxis, ConstructionContext& ctx) {
	const int primCount = endIndex - startIndex;
	const int nodeMaxPrimCount = 512;
	const double bucketSize = (enclosingBounds.getFullExtent(centroidAxis)) / BUCKET_COUNT;
	const double leafCost = PRIM_INTERSECT_COST * primCount;
	
	std::vector<Bucket> buckets;
	buckets.resize(BUCKET_COUNT);
	int bucketIndex = 0;
	double minCost = INFINITY;
	
	BoundingVolumeHierarchy::calculateObjectSplitBucket(startIndex, endIndex, centroidAxis, centroidBounds, enclosingBounds, ctx, bucketIndex, minCost, buckets);

	// if (enough memory for spatial split) {
	bool spatialSplit = BoundingVolumeHierarchy::calculateSpatialSplitBucket(startIndex, endIndex, enclosingAxis, centroidBounds, enclosingBounds, ctx, bucketIndex, minCost, buckets);

	if (spatialSplit) {
		Bucket& bucket = buckets[bucketIndex];
		std::vector<PrimitiveInfo> temp;
		temp.resize(bucket.fragments.size());

		for (size_t i = 0; i < bucket.fragments.size(); ++i) {
			temp[i] = ctx.primitiveInfoList[bucket.fragments[i].originalIndex];
		}
	}

	// }

	if (primCount > nodeMaxPrimCount || minCost < leafCost) {
		if (!spatialSplit) {
			return BoundingVolumeHierarchy::partitionBucket(startIndex, endIndex, bucketIndex, centroidAxis, centroidBounds, false, ctx);
		} else {
			return BoundingVolumeHierarchy::partitionBucket(startIndex, endIndex, bucketIndex, centroidAxis, enclosingBounds, true, ctx);
		}

		//// Split sortedPrimitives at the minimum cost bucket
		//PrimitiveInfo* startPtr = &ctx.primitiveInfoList[startIndex];
		//PrimitiveInfo* endPtr = &ctx.primitiveInfoList[endIndex - 1];
		//PrimitiveInfo* midPtr = std::partition(startPtr, endPtr + 1, [centroidBounds, centroidAxis, bucketIndex](const PrimitiveInfo& prim) {
		//	return BoundingVolumeHierarchy::calculateBucketIndex(prim.bound.getCenter(), centroidAxis, centroidBounds) <= bucketIndex;
		//});
		//
		//return startIndex + midPtr - startPtr;
	} else {
		// no mid point
		return -1;
	}
}

double BoundingVolumeHierarchy::calculateIntersectProbability(AxisAlignedBB innerBound, AxisAlignedBB outerBound) {
	return innerBound.getSurfaceArea() / outerBound.getSurfaceArea();
}

double BoundingVolumeHierarchy::calculateSAHCost(double leftProbability, double rightProbability, uint32_t leftCount, uint32_t rightCount) {
	return NODE_INTERSECT_COST + PRIM_INTERSECT_COST * (leftProbability * leftCount + rightProbability * rightCount);
}

//int BoundingVolumeHierarchy::calculateBucketIndex(dvec3 coord, int axis, AxisAlignedBB bounds) {
//	return min((int)(BUCKET_COUNT * bounds.getUnitCoordinate(coord, (int)axis)), BUCKET_COUNT - 1);
//}

bool BoundingVolumeHierarchy::calculateClippedTriangleBounds(int axis, double position, dvec3 v0, dvec3 v1, dvec3 v2, AxisAlignedBB& clippedBound) {
	double d0 = v0[axis] - position;
	double d1 = v1[axis] - position;
	double d2 = v2[axis] - position;
	bool b0 = d0 < 0.0;
	bool b1 = d1 < 0.0;
	bool b2 = d2 < 0.0;

	if ((b0 && b1 && b2) || (!b0 && !b1 && !b2)) { // All points are on the same side, no clipping
		return false;
	}

	dvec3 points[4]; // unordered clipped polygon points.
	
#define CLIPPED_SEGMENT(va, vb, ta, tb) (va + (vb - va) * (ta / (ta - tb)))

	if ((b0 && b1) || (!b0 && !b1)) { // v2 straddles plane, clip v0v2 and v1v2
		points[0] = v0;
		points[1] = v1;
		points[2] = CLIPPED_SEGMENT(v0, v2, d0, d2);
		points[3] = CLIPPED_SEGMENT(v1, v2, d1, d2);
	} else if ((b1 && b2) || (!b1 && !b2)) { // v0 straddles plane, clip v1v0 and v2v0
		points[0] = v1;
		points[1] = v2;
		points[2] = CLIPPED_SEGMENT(v1, v0, d1, d0);
		points[3] = CLIPPED_SEGMENT(v2, v0, d2, d0);
	} else if ((b2 && b0) || (!b2 && !b0)) { // v1 straddles plane, clip v2v1 and v0v1
		points[0] = v2;
		points[1] = v0;
		points[2] = CLIPPED_SEGMENT(v2, v1, d2, d1);
		points[3] = CLIPPED_SEGMENT(v0, v1, d0, d1);
	}

	for (int i = 0; i < 4; i++) {
		clippedBound = clippedBound.extend(points[i]);
	}

#undef CLIPPED_SEGMENT

	return true;
}

void BoundingVolumeHierarchy::findOptimalObjectPartition() {

}

BVHNode* BoundingVolumeHierarchy::initLeaf(uint32_t startIndex, uint32_t endIndex, uint32_t stackDepth, int primCount, AxisAlignedBB enclosingBounds, ConstructionContext& ctx) {
	ctx.maxLeafDepth = max(ctx.maxLeafDepth, stackDepth);
	ctx.minLeafDepth = min(ctx.minLeafDepth, stackDepth);

	size_t primOffset = ctx.sortedPrimitiveList.size();
	for (int i = startIndex; i < endIndex; ++i) {
		ctx.sortedPrimitiveList.push_back(ctx.primitiveInfoList[i].reference);
	}
	return BVHNode::leaf(primOffset, primCount, enclosingBounds);
}