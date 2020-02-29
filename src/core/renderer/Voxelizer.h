#pragma once
#include "core/pch.h"

class ShaderProgram;

class Voxelizer {
	enum VoxelStorage {
		POSITION = 0,
		CHILD_INDEX = 0,
		ALBEDO = 1,
		NORMAL = 2,
		EMISSIVE = 3
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

public:
	Voxelizer(uint32_t size, double scale);

	~Voxelizer();

	void startRender();

	void finishRender();

	void renderPass(double dt, double partialTicks);

	void renderDebug();

	void applyUniforms(ShaderProgram* shaderProgram);

	void updateAxisProjections();

	void allocateVoxelFragmentList(uint64_t allocatedVoxels);

	void allocateOctreeStorageList(uint64_t allocatedVoxels);

	void setSize(uint32_t size);

	void setScale(double scale);

	void setPosition(dvec3 position);

	uint32_t getSize() const;

	double getScale() const;

	dvec3 getPosition() const;

	static ShaderProgram* voxelGenerationShader();

	static ShaderProgram* voxelOctreeFlagShader();

	static ShaderProgram* voxelOctreeAllocShader();

	static ShaderProgram* voxelOctreeInitShader();

	static ShaderProgram* voxelDirectLightInjectionShader();

	static ShaderProgram* voxelVisualizationShader();

	static ShaderProgram* voxelVisualizationCopyShader();

	static ShaderProgram* voxelClearShader();

private:
	void initTextureBufferStorage(uint64_t size, uint32_t format, uint32_t* textureHandlePtr, uint32_t* textureBufferHandlePtr);

	uint32_t m_size;
	double m_scale;
	dvec3 m_position;

	bool m_countVoxelPass;
	uint32_t m_voxelFragmentCount;
	uint32_t m_allocatedOctreeCount;

	dmat4 m_axisProjections[3];

	uint32_t m_voxelFragmentCounterBuffer;
	uint32_t m_voxelFragmentListTexture[4];
	uint32_t m_voxelFragmentListTextureBuffer[4];

	uint32_t m_octreeStorageCounterBuffer;
	uint32_t m_octreeStorageTexture[4];
	uint32_t m_octreeStorageTextureBuffer[4];

	uint32_t m_voxelVisualizationTexture;

	uint32_t m_tempVAO;

	VoxelizationStage m_voxelizationStage;

	static ShaderProgram* s_voxelGenerationShader;
	static ShaderProgram* s_voxelOctreeFlagShader;
	static ShaderProgram* s_voxelOctreeAllocShader;
	static ShaderProgram* s_voxelOctreeInitShader;
	static ShaderProgram* s_voxelDirectLightInjectionShader;
	static ShaderProgram* s_voxelVisualizationShader;
	static ShaderProgram* s_voxelVisualizationCopyShader;
	static ShaderProgram* s_voxelClearShader;
};

