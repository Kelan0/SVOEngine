#include "BVH.h"

const BVH::PrimitiveReference BVH::INVALID_REFERENCE = -1;
const int BVH::BUCKET_COUNT = 12;
const double BVH::NODE_INTERSECT_COST = 1.0;
const double BVH::PRIMITIVE_INTERSECT_COST = 1.5;

BVHNode* BVHNode::leaf(uint64_t offset, uint64_t count, AxisAlignedBB& bound) {
	BVHNode* children[2] = { NULL, NULL };
	return new BVHNode(offset, count, bound, children, 0);
}

BVHNode* BVHNode::interior(int axis, BVHNode* children[2]) {
	assert(children[0] != NULL && children[1] != NULL);
	AxisAlignedBB bound = AxisAlignedBB::combine(children[0]->getBound(), children[1]->getBound());
	return new BVHNode(0, 0, bound, children, axis);
}

uint64_t BVHNode::getPrimitiveOffset() const {
	return m_primitiveOffset;
}

uint64_t BVHNode::getPrimitiveCount() const {
	return m_primitiveCount;
}

AxisAlignedBB BVHNode::getBound() const {
	return m_bound;
}

BVHNode* BVHNode::getChild(uint32_t index) const {
	return m_children[index];
}

int BVHNode::getSplitAxis() const {
	return m_splitAxis;
}

bool BVHNode::isLeaf() const {
	return m_children[0] == NULL && m_children[1] == NULL;
}

BVHNode::BVHNode(uint64_t offset, uint64_t count, AxisAlignedBB& bound, BVHNode* children[2], int splitAxis):
	m_primitiveOffset(offset), 
	m_primitiveCount(count),
	m_bound(bound),
	m_splitAxis(splitAxis) {
	m_children[0] = children[0];
	m_children[1] = children[1];
}

BVHNode::~BVHNode() {
	if (m_children[0] != NULL) delete m_children[0];
	if (m_children[1] != NULL) delete m_children[1];
}

BVH::~BVH() {
	if (m_root != NULL) delete m_root;
}

const std::vector<BVH::PrimitiveReference>& BVH::getPrimitiveReferences() const {
	return m_primitiveReferences;
}

std::vector<BVHBinaryNode> BVH::createLinearNodes() const {
	std::vector<BVHBinaryNode> linearBuffer;
	this->fillBinaryBuffer(linearBuffer);
	return linearBuffer;
}

BVHNode* BVH::getRoot() const {
	return m_root;
}

Mesh* BVH::getDebugMesh() {
	if (m_debugMesh == NULL) {
		Mesh::Builder* builder = new Mesh::Builder();
		BVH::buildRecursiveMesh(m_root, builder);
		builder->build(&m_debugMesh, true);
	}
	return m_debugMesh;
}

void BVH::fillBinaryBuffer(std::vector<BVHBinaryNode>& linearBuffer) const {
	uint32_t index = 0;
	linearBuffer.resize(m_nodeCount);
	BVH::fillBinaryBuffer(m_root, 0xFFFFFFFF, index, linearBuffer);
}

void BVH::fillQuadBuffer(std::vector<BVHQuadNode>& linearBuffer) const {
	uint32_t index = 0;
	linearBuffer.resize(m_nodeCount);
	BVH::fillQuadBuffer(m_root, 0xFFFFFFFF, index, linearBuffer);
}

BVH::BVH(BVHNode* root, std::vector<PrimitiveReference>& primitiveReferences, uint64_t nodeCount) :
	m_primitiveReferences(primitiveReferences),
	m_root(root),
	m_debugMesh(NULL),
	m_nodeCount(nodeCount) {
}

uint32_t BVH::fillBinaryBuffer(BVHNode* node, uint32_t parentIndex, uint32_t& index, std::vector<BVHBinaryNode>& linearBuffer) {
#define pack29_2_1(a, b, c) (((a & 0x1FFFFFFF) << 3) | ((b & 0x3) << 1) | (c & 0x1))

	BVHBinaryNode& bufferNode = linearBuffer[index];
	bufferNode.xmin = node->getBound().getMin(0);
	bufferNode.ymin = node->getBound().getMin(1);
	bufferNode.zmin = node->getBound().getMin(2);
	bufferNode.xmax = node->getBound().getMax(0);
	bufferNode.ymax = node->getBound().getMax(1);
	bufferNode.zmax = node->getBound().getMax(2);
	bufferNode.parentIndex = parentIndex;
	//bufferNode.splitAxis = node->getSplitAxis();

	int currIndex = index++;
	assert(currIndex != 0xFFFFFFFF);

	if (node->isLeaf()) {
		bufferNode.dataOffset = node->getPrimitiveOffset();
		bufferNode.primitiveCount_splitAxis_flags = pack29_2_1(node->getPrimitiveCount(), node->getSplitAxis(), 1);
		//bufferNode.primitiveCount = node->getPrimitiveCount();
	} else {
		const int leftIndex = BVH::fillBinaryBuffer(node->getChild(0), currIndex, index, linearBuffer);
		const int rightIndex = BVH::fillBinaryBuffer(node->getChild(1), currIndex, index, linearBuffer);
		bufferNode.dataOffset = rightIndex;
		bufferNode.primitiveCount_splitAxis_flags = pack29_2_1(0, node->getSplitAxis(), 0);
		//bufferNode.primitiveCount = 0;
	}

	return currIndex;
#undef pack29_2_1
}

uint32_t BVH::fillQuadBuffer(BVHNode* node, uint32_t parentIndex, uint32_t& index, std::vector<BVHQuadNode>& linearBuffer) {
//#define pack25_6_1(a, b, c) (((a & 0x1FFFFFF) << 7) | ((b & 0x3F) << 1) | (c & 0x1))
//
//	BVHQuadNode& bufferNode = linearBuffer[index];
//	bufferNode.xmin = node->getBound().getMin(0);
//	bufferNode.ymin = node->getBound().getMin(1);
//	bufferNode.zmin = node->getBound().getMin(2);
//	bufferNode.xmax = node->getBound().getMax(0);
//	bufferNode.ymax = node->getBound().getMax(1);
//	bufferNode.zmax = node->getBound().getMax(2);
//	bufferNode.parentIndex = parentIndex;
//	//bufferNode.splitAxis = node->getSplitAxis();
//
//	int currIndex = index++;
//	assert(currIndex != 0xFFFFFFFF);
//
//	if (node->isLeaf()) {
//		bufferNode.dataOffset0 = node->getPrimitiveOffset();
//		bufferNode.primitiveCount_splitAxis_flags = pack25_6_1(node->getPrimitiveCount(), node->getSplitAxis(), 1);
//		//bufferNode.primitiveCount = node->getPrimitiveCount();
//	} else {
//		const int leftIndex = BVH::fillQuadBuffer(node->getChild(0), currIndex, index, linearBuffer);
//		const int rightIndex = BVH::fillQuadBuffer(node->getChild(1), currIndex, index, linearBuffer);
//		bufferNode.dataOffset = rightIndex;
//		bufferNode.primitiveCount_splitAxis_flags = pack25_6_1(0, node->getSplitAxis(), 0);
//		//bufferNode.primitiveCount = 0;
//	}
//
//	return currIndex;
//#undef pack25_6_1
	return 0;
}

BVH* BVH::build(const std::vector<Mesh::vertex>& vertices, const std::vector<Mesh::triangle>& triangles, uint64_t triangleOffset, uint64_t triangleCount) {
	if (vertices.empty() || triangles.empty()) {
		return NULL;
	}

	if (triangleOffset >= triangles.size()) {
		return NULL;
	}

	if (triangleCount > triangles.size()) {
		triangleCount = triangles.size() - triangleOffset;
	}

	AxisAlignedBB rootBounds;

	std::vector<Primitive> primitives;
	primitives.reserve(triangles.size());

	std::vector<PrimitiveReference> sortedPrimitives;
	sortedPrimitives.reserve(triangles.size());

	for (size_t i = 0; i < triangles.size(); ++i) {
		std::vector<dvec3> trianglePoints = {
			vertices[triangles[i].i0].position,
			vertices[triangles[i].i1].position,
			vertices[triangles[i].i2].position
		};

		AxisAlignedBB bound = AxisAlignedBB(trianglePoints);
		//if (bound.isDegenerate(true)) {
		//	continue;
		//}

		primitives.push_back({ (BVH::PrimitiveReference) i, bound });
		rootBounds = AxisAlignedBB::combine(rootBounds, bound);
	}

	uint64_t spatialBudget = primitives.size() * 0.2; // extra 20%
	primitives.resize(primitives.size() + spatialBudget);

	uint64_t nodeCount = 0;

	info("Constructing BVH\n");

	uint64_t activeSpatialBudget = spatialBudget;
	uint64_t t0 = Engine::instance()->getCurrentTime();
	BVHNode* root = BVH::buildRecursive(0, primitives.size(), nodeCount, activeSpatialBudget, rootBounds, vertices, triangles, primitives, sortedPrimitives, Left);
	uint64_t t1 = Engine::instance()->getCurrentTime();

	info("Took %.2f msec to build BVH with %d primitives and %d nodes  - %d spatial splits (%.2f%%)", (t1 - t0) / 1000000.0, triangles.size(), nodeCount, (spatialBudget - activeSpatialBudget), (double) (spatialBudget - activeSpatialBudget) / spatialBudget * 100.0);

	return new BVH(root, sortedPrimitives, nodeCount);
}

BVHNode* BVH::initLeaf(uint64_t startIndex, uint64_t endIndex, AxisAlignedBB enclosingBounds, std::vector<Primitive>& primitives, std::vector<PrimitiveReference>& sortedPrimitives) {
	uint64_t offset = sortedPrimitives.size();
	uint64_t count = 0;

	for (uint64_t i = startIndex; i < endIndex; ++i) {
		if (primitives[i].reference == INVALID_REFERENCE)
			continue;

		sortedPrimitives.push_back(primitives[i].reference);
		count++;
	}

	return BVHNode::leaf(offset, count, enclosingBounds);
}

BVHNode* BVH::buildRecursive(uint64_t startIndex, uint64_t endIndex, uint64_t& nodeCount, uint64_t& spatialBudget, AxisAlignedBB& rootBounds, const std::vector<Mesh::vertex>& vertices, const std::vector<Mesh::triangle>& triangles, std::vector<Primitive>& primitives, std::vector<PrimitiveReference>& sortedPrimitives, TreeSide side) {
	assert(startIndex < endIndex);

	++nodeCount;

	//info("%d nodes - %d <-> %d (%d)\n", nodeCount, startIndex, endIndex, (endIndex - startIndex));

	uint64_t primitiveCount = 0;
	AxisAlignedBB enclosingBounds;

	for (size_t i = startIndex; i < endIndex; ++i) {
		if (primitives[i].reference == INVALID_REFERENCE)
			continue;
		enclosingBounds = AxisAlignedBB::combine(enclosingBounds, primitives[i].bound);
		++primitiveCount;
	}

	if (primitiveCount == 0) {
		return NULL;
	}

	if (primitiveCount == 1) {
		// Only one primitive, so create a leaf.

		return BVH::initLeaf(startIndex, endIndex, enclosingBounds, primitives, sortedPrimitives);
	}

	AxisAlignedBB centroidBounds;
	for (size_t i = startIndex; i < endIndex; ++i) {
		if (primitives[i].reference == INVALID_REFERENCE)
			continue;
		centroidBounds = AxisAlignedBB::combine(centroidBounds, primitives[i].bound.getCenter());
	}

	int centroidLargestAxis = centroidBounds.getLargestAxis();

	if (centroidBounds.getFullExtent(centroidLargestAxis) <= 1e-6) {
		// The centroid bounding box is a single point, possibly because all primitives have the same centroid.
		//info("%d <-> %d (%d %d) - %f\n", startIndex, endIndex, endIndex - startIndex, primitiveCount, centroidBounds.getFullExtent(centroidLargestAxis));
		return BVH::initLeaf(startIndex, endIndex, enclosingBounds, primitives, sortedPrimitives);
	}

	uint64_t midIndex = -1;

	if (primitiveCount <= 4) {
		BVH::partitionEqualCounts(centroidLargestAxis, startIndex, endIndex, midIndex, primitives);
	} else {
		int enclosingLargestAxis = centroidBounds.getLargestAxis();

		int partitionAxis = centroidLargestAxis;
		int partitionIndex;
		double partitionCost = INFINITY;

		std::vector<Bucket> objectBuckets;
		objectBuckets.resize(BUCKET_COUNT);
		BVH::calculateObjectSplit(partitionAxis, startIndex, endIndex, centroidBounds, enclosingBounds, primitives, partitionIndex, partitionCost, objectBuckets);

		//// test all three axis for cheapest split
		//for (int i = 0; i < 3; ++i) {
		//	if (centroidBounds.getFullExtent(i) < 1e-8)
		//		continue;
		//
		//	std::vector<Bucket> objectBuckets;
		//	objectBuckets.resize(BUCKET_COUNT);
		//	int currentPartitionAxis = i;
		//	int currentPartitionIndex;
		//	double currentPartitionCost = INFINITY;
		//	BVH::calculateObjectSplit(currentPartitionAxis, startIndex, endIndex, centroidBounds, enclosingBounds, primitives, currentPartitionIndex, currentPartitionCost, objectBuckets);
		//
		//	if (currentPartitionCost < partitionCost) {
		//		partitionAxis = currentPartitionAxis;
		//		partitionIndex = currentPartitionIndex;
		//		partitionCost = currentPartitionCost;
		//	}
		//}

		bool spatialSplit = false;

#if 0 // spatial split enabled
		if (spatialBudget > 0) {
			AxisAlignedBB lb;
			for (int i = 0; i <= partitionIndex; ++i)
				lb = AxisAlignedBB::combine(lb, objectBuckets[i].bounds);
			
			AxisAlignedBB rb;
			for (int i = partitionIndex + 1; i < BUCKET_COUNT; ++i)
				rb = AxisAlignedBB::combine(rb, objectBuckets[i].bounds);
			
			AxisAlignedBB ob = AxisAlignedBB::overlap(lb, rb);
			if (!ob.isDegenerate()) {
				if (ob.getSurfaceArea() / rootBounds.getSurfaceArea() > 1e-9) {
					std::vector<Bucket> spatialBuckets;
					spatialBuckets.resize(BUCKET_COUNT);

					int spatialPartitionIndex;
					double spatialPartitionCost;
					AxisAlignedBB leftBound, rightBound;
					uint64_t leftCount, rightCount;
					BVH::calculateSpatialSplit(enclosingLargestAxis, startIndex, endIndex, enclosingBounds, primitives, spatialPartitionIndex, spatialPartitionCost, spatialBuckets, leftBound, leftCount, rightBound, rightCount);

					if (spatialPartitionCost < partitionCost) {
						partitionCost = spatialPartitionCost;
						partitionIndex = spatialPartitionIndex;
						spatialSplit = true;
						--spatialBudget;

						//uint64_t lc = 0;
						//for (int i = 0; i <= spatialPartitionIndex; ++i)
						//	lc += spatialBuckets[i].enterCount;
						//
						//uint64_t rc = 0;
						//for (int i = spatialPartitionIndex + 1; i < BUCKET_COUNT; ++i)
						//	rc += spatialBuckets[i].exitCount;

						uint64_t left = startIndex;
						uint64_t right = endIndex - 1;

						std::vector<Primitive> temp;
						temp.reserve(spatialBuckets[spatialPartitionIndex].fragments.size());

						for (size_t i = 0; i < spatialBuckets[spatialPartitionIndex].fragments.size(); ++i) {
							Primitive copy = primitives[spatialBuckets[spatialPartitionIndex].fragments[i].originalIndex];
							temp.push_back(copy);
						}

						for (size_t i = 0; i < spatialBuckets[spatialPartitionIndex].fragments.size(); ++i) {
							Fragment& fragment = spatialBuckets[spatialPartitionIndex].fragments[i];
							Primitive originalPrimitive = temp[i];

							dvec3 bmin, bmax;

							bmin = fragment.primitive.bound.getMin();
							bmax = fragment.primitive.bound.getMax();
							bmin[enclosingLargestAxis] = originalPrimitive.bound.getMin(enclosingLargestAxis);
							assert(bmin[enclosingLargestAxis] <= bmax[enclosingLargestAxis]);
							Primitive leftSplitPrim = { fragment.primitive.reference, AxisAlignedBB(bmin, bmax) };

							bmin = fragment.primitive.bound.getMin();
							bmax = fragment.primitive.bound.getMax();
							bmax[enclosingLargestAxis] = originalPrimitive.bound.getMax(enclosingLargestAxis);
							assert(bmin[enclosingLargestAxis] <= bmax[enclosingLargestAxis]);
							Primitive rightSplitPrim = { fragment.primitive.reference, AxisAlignedBB(bmin, bmax) };

							primitives[fragment.originalIndex] = leftSplitPrim;

							// NODE_INTERSECT_COST + PRIMITIVE_INTERSECT_COST * (lc[i] * lsa + rc[i] * rsa) * invEnclosingSurfaceArea;

							//double lsa = leftSplitPrim.bound.getSurfaceArea();
							//double rsa = rightSplitPrim.bound.getSurfaceArea();
							//double ulsa = AxisAlignedBB::combine(leftSplitPrim.bound, originalPrimitive.bound).getSurfaceArea();
							//double ursa = AxisAlignedBB::combine(rightSplitPrim.bound, originalPrimitive.bound).getSurfaceArea();
							//uint64_t lc = spatialBuckets[spatialPartitionIndex].enterCount;
							//uint64_t rc = spatialBuckets[spatialPartitionIndex].exitCount;
							//
							//double csplit = lsa * lc + rsa * rc;
							//double cleft = ulsa * lc + rsa * (rc - 1);
							//double cright = lsa * (lc - 1) + ursa * rc;


							//if (csplit < cleft && csplit < cright) { // insert both
							if (side == Left) {
								primitives[right--] = rightSplitPrim;
							} else {
								primitives[left++] = rightSplitPrim;
							}
							//} else if (cleft < cright) { // insert left
							//
							//} else { // insert right
							//
							//}

							assert(left <= right + 1);
						}
					}
				//} else {
				//	info("Skipped spatial split\n");
				}
			}
		}
#endif

		double leafCost = PRIMITIVE_INTERSECT_COST * primitiveCount;
		if (partitionCost < leafCost) {
			//if (spatialSplit) {
			//	BVH::partitionSpatial(enclosingLargestAxis, startIndex, endIndex, midIndex, enclosingBounds, primitives, partitionIndex);
			//} else {
			BVH::partitionObjects(partitionAxis, startIndex, endIndex, midIndex, centroidBounds, primitives, partitionIndex);
			//}
		} else {
			return BVH::initLeaf(startIndex, endIndex, enclosingBounds, primitives, sortedPrimitives);
		}
	}

	if (midIndex == -1 || midIndex == startIndex || midIndex == endIndex) {
		// There is no valid partition for some reason... Should this be an error?
		return BVH::initLeaf(startIndex, endIndex, enclosingBounds, primitives, sortedPrimitives);
	} else {
		BVHNode* children[2];
		children[0] = BVH::buildRecursive(startIndex, midIndex, nodeCount, spatialBudget, rootBounds, vertices, triangles, primitives, sortedPrimitives, Left);
		children[1] = BVH::buildRecursive(midIndex, endIndex, nodeCount, spatialBudget, rootBounds, vertices, triangles, primitives, sortedPrimitives, Right);

		if (children[0] == NULL && children[1] == NULL)
			return NULL;

		if (children[0] == NULL) return children[1];
		if (children[1] == NULL) return children[0];

		return BVHNode::interior(centroidLargestAxis, children);
	}
}

void BVH::buildRecursiveMesh(BVHNode* node, Mesh::Builder* builder) {
	if (node == NULL) {
		return;
	}

	if (!node->isLeaf()) {
		buildRecursiveMesh(node->getChild(0), builder);
		buildRecursiveMesh(node->getChild(1), builder);
	} else {
		builder->createCuboid(node->getBound().getHalfExtent(), node->getBound().getCenter());
	}
}

void BVH::partitionEqualCounts(int axis, uint64_t startIndex, uint64_t endIndex, uint64_t& midIndex, std::vector<Primitive>& primitives) {
	struct Comparator {
		int axis;

		Comparator(int axis) : axis(axis) {};

		bool operator()(const Primitive& left, const Primitive& right) const {
			return left.bound.getCenter(axis) < right.bound.getCenter(axis);
		}
	};

	midIndex = (startIndex + endIndex) >> 1; // / 2;
	Primitive* startPtr = &primitives[startIndex];
	Primitive* midPtr = &primitives[midIndex];
	Primitive* endPtr = &primitives[endIndex - 1];
	std::nth_element(startPtr, midPtr, endPtr + 1, Comparator(axis));
}

void BVH::partitionObjects(int axis, uint64_t startIndex, uint64_t endIndex, uint64_t& midIndex, AxisAlignedBB bounds, std::vector<Primitive>& primitives, int partitionIndex) {
	std::vector<Primitive> temp;
	temp.reserve(endIndex - startIndex);

	for (uint64_t i = startIndex; i < endIndex; ++i) {
		if (primitives[i].reference == INVALID_REFERENCE)
			continue;

		temp.push_back(primitives[i]);
		primitives[i] = { INVALID_REFERENCE, AxisAlignedBB() };
	}

	uint64_t left = startIndex;
	uint64_t right = endIndex - 1;

	for (size_t i = 0; i < temp.size(); i++) {
		int bucketIndex = min((int)(BUCKET_COUNT * bounds.getUnitCoordinate(temp[i].bound.getCenter(), axis)), BUCKET_COUNT - 1);

		if (bucketIndex <= partitionIndex) {
			primitives[left++] = temp[i];
		} else {
			primitives[right--] = temp[i];
		}

		assert(left <= right + 1);
	}

	midIndex = (left + right) >> 1; // / 2;
}

void BVH::partitionSpatial(int axis, uint64_t startIndex, uint64_t endIndex, uint64_t& midIndex, AxisAlignedBB bounds, std::vector<Primitive>& primitives, int partitionIndex) {
	std::vector<Primitive> temp;
	temp.reserve(endIndex - startIndex);

	for (uint64_t i = startIndex; i < endIndex; ++i) {
		if (primitives[i].reference == INVALID_REFERENCE)
			continue;

		temp.push_back(primitives[i]);
		primitives[i] = { INVALID_REFERENCE, AxisAlignedBB() };
	}

	uint64_t left = startIndex;
	uint64_t right = endIndex - 1;

	for (size_t i = 0; i < temp.size(); i++) {
		int bucketIndex = min((int)(BUCKET_COUNT * bounds.getUnitCoordinate(temp[i].bound.getMin(), axis)), BUCKET_COUNT - 1);

		if (bucketIndex <= partitionIndex) {
			primitives[left++] = temp[i];
		} else {
			primitives[right--] = temp[i];
		}

		assert(left <= right + 1);
	}

	midIndex = (left + right) >> 1; // / 2;
}

void BVH::calculateObjectSplit(int axis, uint64_t startIndex, uint64_t endIndex, AxisAlignedBB centroidBounds, AxisAlignedBB enclosingBounds, std::vector<Primitive>& primitives, int& partitionIndex, double& partitionCost, std::vector<Bucket>& buckets) {
	constexpr int bucketCount = BUCKET_COUNT;
	constexpr int splitCount = BUCKET_COUNT - 1;

	buckets.resize(bucketCount);

	for (uint64_t i = startIndex; i < endIndex; ++i) {
		if (primitives[i].reference == INVALID_REFERENCE)
			continue;

		int bucketIndex = min((int)(bucketCount * centroidBounds.getUnitCoordinate(primitives[i].bound.getCenter(), axis)), splitCount);
		buckets[bucketIndex].bounds = AxisAlignedBB::combine(buckets[bucketIndex].bounds, primitives[i].bound);
		buckets[bucketIndex].primitiveCount++;
	}


	// Count the cumulative left-hand primitive count and bucket bounds.
	uint64_t lc[splitCount];
	AxisAlignedBB lb[splitCount];
	lc[0] = buckets[0].primitiveCount;
	lb[0] = buckets[0].bounds;
	for (int i = 1; i < splitCount; ++i) {
		lc[i] = lc[i - 1] + buckets[i].primitiveCount;
		lb[i] = lb[i - 1].extend(buckets[i].bounds);
	}

	// Count the cumulative right-hand primitive count and bucket bounds.
	uint64_t rc[splitCount];
	AxisAlignedBB rb[splitCount];
	rc[splitCount - 1] = buckets[bucketCount - 1].primitiveCount;
	rb[splitCount - 1] = buckets[bucketCount - 1].bounds;
	for (int i = splitCount - 2; i >= 0; --i) {
		rc[i] = rc[i + 1] + buckets[i + 1].primitiveCount;
		rb[i] = rb[i + 1].extend(buckets[i + 1].bounds);
	}

	partitionCost = INFINITY;
	for (int i = 1; i < splitCount; ++i) {
		double cost = BVH::calculateSplitCost(lb[i], lc[i], rb[i], rc[i], enclosingBounds);

		if (cost < partitionCost) {
			partitionCost = cost;
			partitionIndex = i;
		}
	}
}

void BVH::calculateSpatialSplit(int axis, uint64_t startIndex, uint64_t endIndex, AxisAlignedBB enclosingBounds, std::vector<Primitive>& primitives, int& partitionIndex, double& partitionCost, std::vector<Bucket>& buckets, AxisAlignedBB& leftBound, uint64_t& leftCount, AxisAlignedBB& rightBound, uint64_t& rightCount) {
	constexpr int bucketCount = BUCKET_COUNT;
	constexpr int splitCount = BUCKET_COUNT - 1;
	const double bucketSize = enclosingBounds.getFullExtent(axis) / bucketCount;

	buckets.resize(bucketCount);

	for (uint64_t i = startIndex; i < endIndex; ++i) {
		if (primitives[i].reference == INVALID_REFERENCE)
			continue;

		int minBucketIndex = min((int)(bucketCount * enclosingBounds.getUnitCoordinate(primitives[i].bound.getMin(), axis)), splitCount);
		int maxBucketIndex = min((int)(bucketCount * enclosingBounds.getUnitCoordinate(primitives[i].bound.getMax(), axis)), splitCount);

		buckets[minBucketIndex].enterCount++;
		buckets[maxBucketIndex].exitCount++;

		if (minBucketIndex == maxBucketIndex) {
			buckets[minBucketIndex].bounds = AxisAlignedBB::combine(buckets[minBucketIndex].bounds, primitives[i].bound);
		} else {
			for (int j = minBucketIndex; j <= maxBucketIndex; ++j) {
				dvec3 pmin = primitives[i].bound.getMin();
				dvec3 pmax = primitives[i].bound.getMax();

				const double leftPlane = enclosingBounds.getMin(axis) + bucketSize * j;
				const double rightPlane = leftPlane + bucketSize;

				if (pmax[axis] < leftPlane || pmin[axis] > rightPlane) {
					// Primitive is entirely outside this bucket.
					continue;
				}

				pmin[axis] = max(pmin[axis], leftPlane);
				pmax[axis] = min(pmax[axis], rightPlane);

				AxisAlignedBB clippedBound(pmin, pmax);

				//if (clippedBound.isDegenerate())
				//	continue;

				Fragment frag;
				frag.originalIndex = i;
				frag.primitive.reference = primitives[i].reference;
				frag.primitive.bound = clippedBound;

				buckets[j].fragments.push_back(frag);
				buckets[j].bounds = AxisAlignedBB::combine(buckets[j].bounds, clippedBound);
			}
		}
	}

	// Count the cumulative left-hand primitive count and bucket bounds.
	uint64_t lc[splitCount];
	AxisAlignedBB lb[splitCount];
	lc[0] = buckets[0].enterCount;
	lb[0] = buckets[0].bounds;
	for (int i = 1; i < splitCount; ++i) {
		lc[i] = lc[i - 1] + buckets[i].enterCount;
		lb[i] = lb[i - 1].extend(buckets[i].bounds);
	}

	// Count the cumulative right-hand primitive count and bucket bounds.
	uint64_t rc[splitCount];
	AxisAlignedBB rb[splitCount];
	rc[splitCount - 1] = buckets[bucketCount - 1].exitCount;
	rb[splitCount - 1] = buckets[bucketCount - 1].bounds;
	for (int i = splitCount - 2; i >= 0; --i) {
		rc[i] = rc[i + 1] + buckets[i + 1].exitCount;
		rb[i] = rb[i + 1].extend(buckets[i + 1].bounds);
	}

	partitionCost = INFINITY;
	for (int i = 1; i < splitCount; ++i) {
		double cost = BVH::calculateSplitCost(lb[i], lc[i], rb[i], rc[i], enclosingBounds);

		if (cost < partitionCost) {
			partitionCost = cost;
			partitionIndex = i;
			leftBound = lb[i];
			leftCount = lc[i];
			rightBound = rb[i];
			rightCount = rc[i];
		}
	}
}

double BVH::calculateSplitCost(AxisAlignedBB leftBound, uint64_t leftCount, AxisAlignedBB rightBound, uint64_t rightCount, AxisAlignedBB enclosingBound) {
	return NODE_INTERSECT_COST + PRIMITIVE_INTERSECT_COST * (leftCount * leftBound.getSurfaceArea() + rightCount * rightBound.getSurfaceArea()) / enclosingBound.getSurfaceArea();
}
