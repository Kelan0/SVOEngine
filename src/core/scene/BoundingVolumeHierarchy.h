#pragma once

#include "core/pch.h"
#include "core/scene/Bounding.h"
#include "core/renderer/geometry/Mesh.h"

//struct BVHTrianglePrimitive : public PrimitiveReference {
//	uint32_t i0, i1, i2;
//};

//truct BVHBoundingPrimitive : public PrimitiveReference {
//	dvec3 center;
//	dvec3 u, v, w;
//;

//class BVHNode {
//	friend class BoundingVolumeHierarchy;
//public:
//	static BVHNode* leaf(uint32_t offset, uint32_t count, AxisAlignedBB& bound);
//
//	static BVHNode* interior(int axis, BVHNode* left, BVHNode* right);
//
//	uint32_t getPrimitiveOffset() const;
//
//	uint32_t getPrimitiveCount() const;
//
//	AxisAlignedBB getBound() const;
//
//	BVHNode* getLeft() const;
//
//	BVHNode* getRight() const;
//
//	int getSplitAxis() const;
//
//	bool isLeaf() const;
//
//private:
//	~BVHNode();
//
//	uint32_t m_primitiveOffset;
//	uint32_t m_primitiveCount;
//	AxisAlignedBB m_bound;
//	BVHNode* m_left;
//	BVHNode* m_right;
//	int m_splitAxis;
//};
//
//enum class SplitMethod {
//	Middle,
//	EqualCount,
//	SAH
//};
//
//enum class TreeSide {
//	Left, Right
//};
//
//class BoundingVolumeHierarchy {
//public:
//	typedef uint32_t PrimitiveReference; // the index of a primitive
//	static const PrimitiveReference INVALID_REFERENCE;
//	static const int BUCKET_COUNT;
//	static const double NODE_INTERSECT_COST;
//	static const double PRIM_INTERSECT_COST;
//
//	struct PrimitiveInfo {
//		PrimitiveReference reference = INVALID_REFERENCE;
//		AxisAlignedBB bound;
//	};
//
//	struct PrimitiveFragment {
//		PrimitiveInfo splitInfo;
//		uint32_t originalIndex;
//	};
//
//	struct BufferNode {
//		fvec3 boundMin;
//		fvec3 boundMax;
//		uint32_t dataOffset; // reference into primitive list if this is a leaf, reference of right hand child otherwise.
//		uint16_t primitiveCount; // 0 if this is not a leaf node
//		uint16_t splitAxis; // 0=X, 1=Y, 2=Z, 16 bit int in order to pad the struct to 32 bytes
//	};
//
//	typedef std::vector<PrimitiveReference> PrimitiveReferenceList;
//	typedef std::vector<PrimitiveInfo> PrimitiveInfoList;
//	typedef std::vector<BufferNode> BufferNodeList;
//
//	struct ConstructionContext {
//		SplitMethod splitMethod = SplitMethod::SAH;
//		uint32_t maxLeafDepth = 0;
//		uint32_t minLeafDepth = -1;
//		size_t spatialSplitBudget = 0;
//		size_t spatialSplitCount = 0;
//		size_t nodeCount = 0;
//		std::vector<Mesh::vertex> vertices;
//		std::vector<Mesh::triangle> triangles;
//		PrimitiveInfoList primitiveInfoList;
//		PrimitiveReferenceList sortedPrimitiveList;
//	};
//
//	struct Bucket {
//		uint32_t primCount = 0;
//		uint32_t enterCount = 0;
//		uint32_t exitCount = 0;
//		AxisAlignedBB bound;
//		std::vector<PrimitiveFragment> fragments;
//	};
//
//	~BoundingVolumeHierarchy();
//
//	const BVHNode* getRoot() const;
//	
//	const PrimitiveReferenceList getPrimitiveList() const;
//
//	const BufferNodeList getLinearBuffer() const;
//
//	void renderDebug(double dt, double partialTicks);
//
//	void buildDebugMesh();
//
//	Mesh* getDebugMesh() const;
//
//	static void fillTrianglePrimitiveList(uint64_t triangleOffset, uint64_t triangleCount, const std::vector<Mesh::vertex>& vertices, const std::vector<Mesh::triangle>& triangles, PrimitiveInfoList& primitiveInfo);
//
//	//static BoundingVolumeHierarchy* build(PrimitiveReferenceList& primitiveReference, PrimitiveInfoList& primitiveInfoLost);
//	static BoundingVolumeHierarchy* build(std::vector<Mesh::vertex>& vertices, std::vector<Mesh::triangle>& triangles, uint64_t triangleOffset = 0, uint64_t triangleCount = -1);
//
//private:
//	BoundingVolumeHierarchy(BVHNode* root, PrimitiveReferenceList& primitiveReferences, BufferNodeList& linearBuffer);
//
//	ShaderProgram* debugShader();
//
//	static int flattenTree(BVHNode* node, int& index, BufferNodeList& linearBuffer);
//
//	static BVHNode* recursiveBuild(uint32_t startIndex, uint32_t endIndex, uint32_t stackDepth, TreeSide side, ConstructionContext& ctx);
//
//	static void recursiveBuildDebugMesh(BVHNode* node, uint32_t stackDepth, Mesh::Builder* debugMeshBuilder, std::vector<std::pair<uint32_t, uint32_t>>& levelRenderSections);
//
//	static inline uint32_t partitionMiddle(uint32_t startIndex, uint32_t endIndex, AxisAlignedBB bounds, int axis, ConstructionContext& ctx);
//
//	static inline uint32_t partitionEqualCount(uint32_t startIndex, uint32_t endIndex, int axis, ConstructionContext& ctx);
//
//	static inline uint32_t partitionBucket(uint32_t startIndex, uint32_t endIndex, int partitionBucket, int axis, AxisAlignedBB bounds, bool spatial, ConstructionContext& ctx);
//
//	static inline bool calculateObjectSplitBucket(uint32_t startIndex, uint32_t endIndex, int axis, AxisAlignedBB centroidBounds, AxisAlignedBB enclosingBounds, ConstructionContext& ctx, int& bucketIndex, double& bucketCost, std::vector<Bucket>& buckets);
//	
//	static inline bool calculateSpatialSplitBucket(uint32_t startIndex, uint32_t endIndex, int axis, AxisAlignedBB centroidBounds, AxisAlignedBB enclosingBounds, ConstructionContext& ctx, int& bucketIndex, double& bucketCost, std::vector<Bucket>& buckets);
//
//	static inline int splitSAH(uint32_t startIndex, uint32_t endIndex, AxisAlignedBB centroidBounds, AxisAlignedBB enclosingBounds, int centroidAxis, int enclosingAxis, ConstructionContext& ctx);
//
//	static inline double calculateIntersectProbability(AxisAlignedBB innerBound, AxisAlignedBB outerBound);
//
//	static inline double calculateSAHCost(double leftProbability, double rightProbability, uint32_t leftCount, uint32_t rightCount);
//
//	//static inline int calculateBucketIndex(dvec3 coord, int axis, AxisAlignedBB bounds);
//
//	static inline bool calculateClippedTriangleBounds(int axis, double position, dvec3 v0, dvec3 v1, dvec3 v2, AxisAlignedBB& clippedBound);
//
//	static inline void findOptimalObjectPartition();
//
//	static inline BVHNode* initLeaf(uint32_t startIndex, uint32_t endIndex, uint32_t stackDepth, int primCount, AxisAlignedBB enclosingBounds, ConstructionContext& ctx);
//
//	BVHNode* m_root;
//	PrimitiveReferenceList m_primitiveReferences;
//	BufferNodeList m_linearBuffer;
//
//	Mesh* m_debugMesh;
//	ShaderProgram* m_debugShader;
//};