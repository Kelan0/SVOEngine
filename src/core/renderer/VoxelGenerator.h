#pragma once
#include "core/pch.h"

class ShaderProgram;
class Framebuffer;

class VoxelGenerator {
public:
	enum VoxelStorage {
		POSITION = 0,
		CHILD_INDEX = 0,
		DATA_STORAGE = 1,
		ALBEDO = 1,
		NORMAL = 2,
		EMISSIVE = 3,
		LEAF_HEAD_INDEX = 4
	};

	enum VoxelizationStage {
		FRAGMENT_COUNT = 0,
		FRAGMENT_STORAGE = 1,
		OCTREE_FLAG = 2,
		OCTREE_ALLOCATE = 3,
		OCTREE_INITIALIZE = 4,
		LIGHT_INJECTION = 5,
		VISUALIZATION_COPY = 6,
		VISUALIZATION = 6,
	};

	VoxelGenerator(uint32_t gridSize, double gridScale);

	~VoxelGenerator();

	void render(double dt, double partialTicks);

	void renderDebug();

	void applyUniforms(ShaderProgram* shaderProgram);

	dvec3 getGridCenter() const;

	void setGridCenter(dvec3 gridCenter);

	uint32_t getGridSize() const;

	void setGridSize(uint32_t gridSize);

	double getGridScale() const;

	void setGridScale(double gridScale);

	uint32_t getOctreeDepth() const;

	int32_t getDebugOctreeVisualisationLevel() const;

	void setDebugOctreeVisualisationLevel(int32_t visualisationLevel);

	void incrDebugOctreeVisualisationLevel(int32_t visualisationLevelIncr);

private:
	void voxelFragmentCountPass(double dt, double partialTicks);

	void voxelFragmentStoragePass(double dt, double partialTicks);

	void octreeConstructionPass(double dt, double partialTicks);

	void countOctreeNodes();

	void octreeNodeFlagPass();

	void octreeNodeAllocPass(uint32_t levelTileCount);

	void octreeNodeInitPass(uint32_t childTileAllocCount);

	void updateVoxelVisualizationTexture(bool clear = false);

	void deleteVoxelVisualizationTexture();

	void updateAxisProjections();

	void allocateTextureBuffer(uint64_t size, uint32_t format, uint32_t* textureHandlePtr, uint32_t* textureBufferHandlePtr);

	void allocateVoxelFragmentBuffers(bool shrinkToFit);

	void allocateOctreeNodeBuffers();

	void bindVoxelPositionTexture(uint32_t unit, uint32_t textures[5]);

	void bindVoxelChildIndexTexture(uint32_t unit, uint32_t textures[5]);

	void bindVoxelDataStorageTexture(uint32_t unit, uint32_t textures[5]);

	void bindVoxelAlbedoTexture(uint32_t unit, uint32_t textures[5]);

	void bindVoxelNormalTexture(uint32_t unit, uint32_t textures[5]);

	void bindVoxelEmissiveTexture(uint32_t unit, uint32_t textures[5]);

	void bindVoxelLeafHeadIndexTexture(uint32_t unit, uint32_t textures[5]);

	uint32_t readAndClearAtomicCounter(uint32_t atomicCounterHandle);

	uint32_t readAtomicCounter(uint32_t atomicCounterHandle);

	uint32_t readAndSetAtomicCounter(uint32_t atomicCounterHandle, uint32_t value);

	uint32_t setAtomicCounter(uint32_t atomicCounterHandle, uint32_t value);

	void clearAtomicCounter(uint32_t atomicCounterHandle);

	ShaderProgram* voxelFragmentGenerationShader();

	ShaderProgram* voxelOctreeNodeFlagShader();

	ShaderProgram* voxelOctreeNodeAllocShader();

	ShaderProgram* voxelOctreeNodeInitShader();

	ShaderProgram* voxelVisualizationCopyShader();

	ShaderProgram* voxelFragmentVisualizationShader();

	ShaderProgram* voxelOctreeVisualizationShader();

	Framebuffer* m_viewportFramebuffer;

	dvec3 m_gridCenter;
	uint32_t m_gridSize;
	double m_gridScale;

	dmat4 m_axisProjections[3];

	uint32_t m_voxelFragmentTexture[5];
	uint32_t m_voxelFragmentTextureBuffer[5];
	uint32_t m_voxelFragmentCounterBuffer;
	uint32_t m_voxelFragmentAllocatedCount;
	uint32_t m_voxelFragmentCount;

	uint32_t m_octreeBrickTexture;
	uint32_t m_octreeNodeTexture[5];
	uint32_t m_octreeNodeTextureBuffer[5];
	uint32_t m_octreeConstructionDataBuffer;
	uint32_t m_octreeIndirectComputeDispachBuffer;
	uint32_t m_octreeNodeAllocatedCount;
	uint32_t m_octreeNodeCount;
	uint32_t m_octreeDepth;

	uint32_t m_axisProjectionMap;

	uint32_t m_resetBuffer;

	uint32_t m_voxelVisualizationTexture;
	uint32_t m_voxelVisualizationTextureSize;
	uint32_t m_visualizationVAO;
	int32_t m_debugOctreeVisualisationLevel;

	VoxelizationStage m_voxelizationStage;

	ShaderProgram* m_voxelFragmentGenerationShader;
	ShaderProgram* m_voxelOctreeNodeFlagShader;
	ShaderProgram* m_voxelOctreeNodeAllocShader;
	ShaderProgram* m_voxelOctreeNodeInitShader;
	ShaderProgram* m_voxelVisualizationCopyShader;
	ShaderProgram* m_voxelFragmentVisualizationShader;
	ShaderProgram* m_voxelOctreeVisualizationShader;
};

