#pragma once

#include "core/pch.h"
#include "core/scene/Bounding.h"
#include "core/renderer/geometry/Mesh.h"

class BVHNode {
	friend class BVH;
public:
	static BVHNode* leaf(uint64_t offset, uint64_t count, AxisAlignedBB& bound);

	static BVHNode* interior(int axis, BVHNode* children[2]);

	uint64_t getPrimitiveOffset() const;

	uint64_t getPrimitiveCount() const;

	AxisAlignedBB getBound() const;

	BVHNode* getChild(uint32_t index) const;

	int getSplitAxis() const;

	bool isLeaf() const;

private:
	BVHNode(uint64_t offset, uint64_t count, AxisAlignedBB& bound, BVHNode* children[2], int splitAxis);

	~BVHNode();

	uint64_t m_primitiveOffset;
	uint64_t m_primitiveCount;
	AxisAlignedBB m_bound;
	BVHNode* m_children[2];
	int m_splitAxis;
};

struct BVHBinaryNode {
	float xmin, ymin, zmin;
	float xmax, ymax, zmax;
	uint32_t parentIndex;
	uint32_t dataOffset;
	uint32_t primitiveCount_splitAxis_flags;
};

struct BVHQuadNode {
	float xmin, ymin, zmin;
	float xmax, ymax, zmax;
	uint32_t parentIndex;
	uint32_t dataOffset0;
	uint32_t dataOffset1;
	uint32_t dataOffset2;
	uint32_t primitiveCount_splitAxis_flags;
};

class BVH {
public:
	typedef uint32_t PrimitiveReference;

	static const PrimitiveReference INVALID_REFERENCE;
	static const int BUCKET_COUNT;
	static const double NODE_INTERSECT_COST;
	static const double PRIMITIVE_INTERSECT_COST;

	struct Primitive {
		PrimitiveReference reference = INVALID_REFERENCE;
		AxisAlignedBB bound;
	};

	struct Fragment {
		uint64_t originalIndex; // The original index of the primitive that was split
		Primitive primitive; // The new primitive with clipped bounds for this fragment
	};

	struct Bucket {
		AxisAlignedBB bounds;
		uint64_t primitiveCount;
		uint64_t enterCount;
		uint64_t exitCount;
		std::vector<Fragment> fragments;
	};

	enum TreeSide {
		Left,
		Right
	};

	~BVH();

	const std::vector<PrimitiveReference>& getPrimitiveReferences() const;

	std::vector<BVHBinaryNode> createLinearNodes() const;

	BVHNode* getRoot() const;

	Mesh* getDebugMesh();

	void fillBinaryBuffer(std::vector<BVHBinaryNode>& linearBuffer) const;

	void fillQuadBuffer(std::vector<BVHQuadNode>& linearBuffer) const;

	static BVH* build(const std::vector<Mesh::vertex>& vertices, const std::vector<Mesh::triangle>& triangles, uint64_t triangleOffset = 0, uint64_t triangleCount = -1);

private:
	BVH(BVHNode* root, std::vector<PrimitiveReference>& primitiveReferences, uint64_t nodeCount);

	static uint32_t fillBinaryBuffer(BVHNode* node, uint32_t parentIndex, uint32_t& index, std::vector<BVHBinaryNode>& linearBuffer);

	static uint32_t fillQuadBuffer(BVHNode* node, uint32_t parentIndex, uint32_t& index, std::vector<BVHQuadNode>& linearBuffer);

	static BVHNode* initLeaf(uint64_t startIndex, uint64_t endIndex, AxisAlignedBB enclosingBounds, std::vector<Primitive>& primitives, std::vector<PrimitiveReference>& sortedPrimitives);

	static BVHNode* buildRecursive(uint64_t startIndex, uint64_t endIndex, uint64_t& nodeCount, uint64_t& spatialBudget, AxisAlignedBB& rootBounds, const std::vector<Mesh::vertex>& vertices, const std::vector<Mesh::triangle>& triangles, std::vector<Primitive>& primitives, std::vector<PrimitiveReference>& sortedPrimitives, TreeSide side);

	static void buildRecursiveMesh(BVHNode* node, Mesh::Builder* builder);

	static void partitionEqualCounts(int axis, uint64_t startIndex, uint64_t endIndex, uint64_t& midIndex, std::vector<Primitive>& primitives);

	static void partitionObjects(int axis, uint64_t startIndex, uint64_t endIndex, uint64_t& midIndex, AxisAlignedBB bounds, std::vector<Primitive>& primitives, int partitionIndex);

	static void partitionSpatial(int axis, uint64_t startIndex, uint64_t endIndex, uint64_t& midIndex, AxisAlignedBB bounds, std::vector<Primitive>& primitives, int partitionIndex);

	static void calculateObjectSplit(int axis, uint64_t startIndex, uint64_t endIndex, AxisAlignedBB centroidBounds, AxisAlignedBB enclosingBounds, std::vector<Primitive>& primitives, int& partitionIndex, double& partitionCost, std::vector<Bucket>& buckets);
	
	static void calculateSpatialSplit(int axis, uint64_t startIndex, uint64_t endIndex, AxisAlignedBB enclosingBounds, std::vector<Primitive>& primitives, int& partitionIndex, double& partitionCost, std::vector<Bucket>& buckets, AxisAlignedBB& leftBound, uint64_t& leftCount, AxisAlignedBB& rightBound, uint64_t& rightCount);

	static double calculateSplitCost(AxisAlignedBB leftBound, uint64_t leftCount, AxisAlignedBB rightBound, uint64_t rightCount, AxisAlignedBB enclosingBound);

	std::vector<PrimitiveReference> m_primitiveReferences;
	BVHNode* m_root;
	Mesh* m_debugMesh;
	uint64_t m_nodeCount;
};

