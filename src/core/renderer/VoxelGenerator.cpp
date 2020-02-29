#include "VoxelGenerator.h"
#include "core/renderer/ShaderProgram.h"
#include "core/renderer/Framebuffer.h"
#include "core/renderer/ScreenRenderer.h"
#include "core/renderer/geometry/Mesh.h"
#include "core/scene/Scene.h"
#include "core/scene/Camera.h"
#include "core/Engine.h"

struct VoxelLinkedListNode {
	uint32_t albedo; // rgba8
	uint32_t normal; // rg16f
	uint32_t emissive; // rgb10_a2
	uint32_t next;
};


struct OctreeConstructionData {
	uint32_t flagPassWorkgroupSizeX;
	uint32_t flagPassWorkgroupSizeY;
	uint32_t flagPassWorkgroupSizeZ;
	uint32_t allocPassWorkgroupSizeX;
	uint32_t allocPassWorkgroupSizeY;
	uint32_t allocPassWorkgroupSizeZ;
	uint32_t initPassWorkgroupSizeX;
	uint32_t initPassWorkgroupSizeY;
	uint32_t initPassWorkgroupSizeZ;
	int32_t passLevelTileCount;
	int32_t passLevelTileStart;
	int32_t passTotalAllocCount;

	vec4 voxelGridCenter; // must be aligned to 16 bytes
	int32_t voxelFragmentCount;
	int32_t voxelGridSize;
	int32_t octreeLevel;
	int32_t levelTileCount;
	int32_t levelTileStart;
	int32_t childTileAllocCount;
	int32_t childTileAllocStart;
	int32_t stage;

	uint32_t levelTileAllocCount;
	uint32_t levelTileTotalCount;
	uint32_t totalTileAllocCount;
	uint32_t leafLinkedListCounter;
};

VoxelGenerator::VoxelGenerator(uint32_t gridSize, double gridScale):
	m_gridCenter(0.0),
	m_gridSize(0),
	m_gridScale(1.0) {

	int32_t resetBuffer[10] = {0,1,2,3,4,5,6,7,8,9};

	this->setGridSize(gridSize);
	this->setGridScale(gridScale);

	info("Initializing voxel generator\n");
	m_viewportFramebuffer = new Framebuffer();
	m_viewportFramebuffer->bind();
	m_viewportFramebuffer->createColourBufferAttachment(gridSize, gridSize, m_viewportFramebuffer->genRenderBuffers(), 0, GL_RGBA8);
	m_viewportFramebuffer->checkStatus(true);
	m_viewportFramebuffer->unbind();

	glGenTextures(1, &m_axisProjectionMap);
	glBindTexture(GL_TEXTURE_2D, m_axisProjectionMap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gridSize, gridSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(5, m_voxelFragmentTexture);
	glGenBuffers(5, m_voxelFragmentTextureBuffer);
	glGenBuffers(1, &m_voxelFragmentCounterBuffer);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_voxelFragmentCounterBuffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(uint32_t), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	glGenTextures(1, &m_octreeBrickTexture);
	glGenTextures(5, m_octreeNodeTexture);
	glGenBuffers(5, m_octreeNodeTextureBuffer);
	glGenBuffers(1, &m_octreeConstructionDataBuffer);
	glGenBuffers(1, &m_octreeIndirectComputeDispachBuffer);
	glGenBuffers(1, &m_resetBuffer);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeConstructionDataBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(OctreeConstructionData), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_octreeIndirectComputeDispachBuffer);
	glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(uvec3) * 3, NULL, GL_STREAM_READ);
	glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);

	glBindBuffer(GL_COPY_READ_BUFFER, m_resetBuffer);
	glBufferData(GL_COPY_READ_BUFFER, sizeof(int32_t) * 10, resetBuffer, GL_STATIC_COPY);
	glBindBuffer(GL_COPY_READ_BUFFER, 0);

	glGenVertexArrays(1, &m_visualizationVAO);
	m_voxelVisualizationTextureSize = 0;

	/* 
	UBO Data - 11 * 4 = 44 bytes
	sizeof(uint32_t) * 7 + sizeof(float) * 4

	int octreeLevel
	int passLevelTileCount
	int passLevelTileStart
	int childTileAllocCount
	int childTileAllocStart
	float voxelGridScale
	fvec3 voxelGridCenter
	int voxelGridSize
	int voxelFragmentCount
	*/

	// Initialize shaders
	voxelFragmentGenerationShader();
	voxelOctreeNodeFlagShader();
	voxelOctreeNodeAllocShader();
	voxelOctreeNodeInitShader();
	voxelVisualizationCopyShader();
	voxelFragmentVisualizationShader();
	voxelOctreeVisualizationShader();
}

VoxelGenerator::~VoxelGenerator() {
	glDeleteTextures(5, m_voxelFragmentTexture);
	glDeleteBuffers(5, m_voxelFragmentTextureBuffer);
	glDeleteBuffers(1, &m_voxelFragmentCounterBuffer);
	glDeleteTextures(1, &m_octreeBrickTexture);
	glDeleteTextures(5, m_octreeNodeTexture);
	glDeleteBuffers(5, m_octreeNodeTextureBuffer);
	glDeleteBuffers(1, &m_octreeConstructionDataBuffer);
	glDeleteBuffers(1, &m_octreeIndirectComputeDispachBuffer);
	glDeleteBuffers(1, &m_resetBuffer);

	glDeleteVertexArrays(1, &m_visualizationVAO);
	this->deleteVoxelVisualizationTexture();
}

void VoxelGenerator::render(double dt, double partialTicks) {
	//glFinish();
	//uint64_t a = Engine::instance()->getCurrentTime();

	glClearTexImage(m_axisProjectionMap, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	this->updateAxisProjections();

	m_viewportFramebuffer->bind(m_gridSize, m_gridSize);
	//glViewport(0, 0, m_gridSize, m_gridSize);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	//glEnable(GL_CONSERVATIVE_RASTERIZATION_NV); // TODO: MSAA instead of this


	this->voxelFragmentCountPass(dt, partialTicks);
	this->allocateVoxelFragmentBuffers(false);
	this->voxelFragmentStoragePass(dt, partialTicks);
	this->countOctreeNodes(); // TODO: more exact count
	this->allocateOctreeNodeBuffers(); // TODO: expand buffer to fit required size without erasing existing data
	this->octreeConstructionPass(dt, partialTicks);

	// this->octreeNodeMipmapPass();
	ShaderProgram::use(NULL);

	Engine::instance()->screenRenderer()->bindFramebuffer();
	//Engine::instance()->screenRenderer()->initViewport();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	//glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);

	//glFinish();
	//uint64_t b = Engine::instance()->getCurrentTime();
	//info("Took %.2f msec to voxelize scene\n", (b - a) / 1000000.0);
}

void VoxelGenerator::renderDebug() {
	info("%d %d\n", m_voxelFragmentCount, m_octreeNodeAllocatedCount);

#if 1
	ShaderProgram* voxelFragmentVisualizationShader = this->voxelFragmentVisualizationShader();
	ShaderProgram::use(voxelFragmentVisualizationShader);
	Engine::instance()->scene()->applyUniforms(voxelFragmentVisualizationShader);
	voxelFragmentVisualizationShader->setUniform("fullGrid", true);
	voxelFragmentVisualizationShader->setUniform("octreeLevel", (int)m_debugOctreeVisualisationLevel);
	voxelFragmentVisualizationShader->setUniform("octreeDepth", (int)m_octreeDepth);
	this->bindVoxelPositionTexture(0, m_voxelFragmentTexture);
	this->bindVoxelDataStorageTexture(1, m_voxelFragmentTexture);
	this->bindVoxelChildIndexTexture(2, m_octreeNodeTexture);
	this->bindVoxelDataStorageTexture(3, m_octreeNodeTexture);
	//this->bindVoxelAlbedoTexture(1, m_voxelFragmentTexture);
	//this->bindVoxelNormalTexture(2, m_voxelFragmentTexture);
	//this->bindVoxelEmissiveTexture(3, m_voxelFragmentTexture);
	glBindVertexArray(m_visualizationVAO);
	glDrawArrays(GL_POINTS, 0, m_voxelFragmentCount);
	glBindVertexArray(0);
	
	ShaderProgram::use(NULL);
#else
	ShaderProgram* voxelOctreeVisualizationShader = this->voxelOctreeVisualizationShader();
	ShaderProgram::use(voxelOctreeVisualizationShader);
	Engine::instance()->scene()->applyUniforms(voxelOctreeVisualizationShader);
	voxelOctreeVisualizationShader->setUniform("octreeLevel", (int) m_debugOctreeVisualisationLevel);
	voxelOctreeVisualizationShader->setUniform("octreeDepth", (int) m_octreeDepth);
	voxelOctreeVisualizationShader->setUniform("voxelGridScale", (float)m_gridScale);
	voxelOctreeVisualizationShader->setUniform("voxelGridCenter", m_gridCenter);// floor(m_gridCenter / m_gridScale)* m_gridScale);
	voxelOctreeVisualizationShader->setUniform("voxelGridSize", (int)m_gridSize);
	this->bindVoxelPositionTexture(0, m_voxelFragmentTexture);
	this->bindVoxelChildIndexTexture(1, m_octreeNodeTexture);
	this->bindVoxelDataStorageTexture(2, m_octreeNodeTexture);
	//this->bindVoxelAlbedoTexture(2, m_octreeNodeTexture);
	//this->bindVoxelNormalTexture(3, m_octreeNodeTexture);
	//this->bindVoxelEmissiveTexture(4, m_octreeNodeTexture);
	glBindImageTexture(5, m_axisProjectionMap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
	glBindVertexArray(m_visualizationVAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
	ShaderProgram::use(NULL);
#endif
}

void VoxelGenerator::applyUniforms(ShaderProgram* shaderProgram) {
	// CHECK IF WE ARE VOXELIZING THE SCENE TO AVOID THIS ON EVERY RENDERED OBJECT
	if (m_voxelizationStage == FRAGMENT_COUNT) {
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_voxelFragmentCounterBuffer);
		this->bindVoxelPositionTexture(0, m_voxelFragmentTexture);
		this->bindVoxelDataStorageTexture(1, m_voxelFragmentTexture);
		//this->bindVoxelAlbedoTexture(1, m_voxelFragmentTexture);
		//this->bindVoxelNormalTexture(2, m_voxelFragmentTexture);
		//this->bindVoxelEmissiveTexture(3, m_voxelFragmentTexture);
		this->bindVoxelLeafHeadIndexTexture(4, m_voxelFragmentTexture);
		shaderProgram->setUniform("countVoxels", true);
	} else if (m_voxelizationStage == FRAGMENT_STORAGE) {
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_voxelFragmentCounterBuffer);
		this->bindVoxelPositionTexture(0, m_voxelFragmentTexture);
		this->bindVoxelDataStorageTexture(1, m_voxelFragmentTexture);
		//this->bindVoxelAlbedoTexture(1, m_voxelFragmentTexture);
		//this->bindVoxelNormalTexture(2, m_voxelFragmentTexture);
		//this->bindVoxelEmissiveTexture(3, m_voxelFragmentTexture);
		this->bindVoxelLeafHeadIndexTexture(4, m_voxelFragmentTexture);

		shaderProgram->setUniform("countVoxels", false);
	}

	glBindImageTexture(5, m_axisProjectionMap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

	for (int i = 0; i < 3; i++) {
		shaderProgram->setUniform("axisProjections[" + std::to_string(i) + "]", m_axisProjections[i]);
		shaderProgram->setUniform("invAxisProjections[" + std::to_string(i) + "]", inverse(m_axisProjections[i]));
	}

	shaderProgram->setUniform("voxelGridScale", (float) m_gridScale);
	shaderProgram->setUniform("voxelGridCenter", floor(m_gridCenter / m_gridScale) * m_gridScale);
	shaderProgram->setUniform("voxelGridSize", (int) m_gridSize);

	shaderProgram->setUniform("voxelFragmentCount", (int) m_voxelFragmentCount);
}

dvec3 VoxelGenerator::getGridCenter() const {
	return m_gridCenter;
}

void VoxelGenerator::setGridCenter(dvec3 gridCenter) {
	m_gridCenter = gridCenter;
}

uint32_t VoxelGenerator::getGridSize() const {
	return m_gridSize;
}

void VoxelGenerator::setGridSize(uint32_t gridSize) {
	m_gridSize = gridSize;
	m_octreeDepth = (uint32_t)log2f(gridSize);
}

double VoxelGenerator::getGridScale() const {
	return m_gridScale;
}

void VoxelGenerator::setGridScale(double gridScale) {
	m_gridScale = gridScale;
}

uint32_t VoxelGenerator::getOctreeDepth() const {
	return m_octreeDepth;
}

int32_t VoxelGenerator::getDebugOctreeVisualisationLevel() const {
	return m_debugOctreeVisualisationLevel;
}

void VoxelGenerator::setDebugOctreeVisualisationLevel(int32_t visualisationLevel) {
	int32_t depth = m_octreeDepth;
	m_debugOctreeVisualisationLevel = (((visualisationLevel) % depth) + depth) % depth;
}

void VoxelGenerator::incrDebugOctreeVisualisationLevel(int32_t visualisationLevelIncr) {
	this->setDebugOctreeVisualisationLevel(m_debugOctreeVisualisationLevel + visualisationLevelIncr);
}

void VoxelGenerator::voxelFragmentCountPass(double dt, double partialTicks) {
	m_voxelizationStage = FRAGMENT_COUNT;
	ShaderProgram* generationShader = this->voxelFragmentGenerationShader();
	ShaderProgram::use(generationShader);
	Engine::scene()->renderDirect(generationShader, dt, partialTicks);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	m_voxelFragmentCount = this->readAndClearAtomicCounter(m_voxelFragmentCounterBuffer);
}

void VoxelGenerator::voxelFragmentStoragePass(double dt, double partialTicks) {
	m_voxelizationStage = FRAGMENT_STORAGE;
	ShaderProgram* generationShader = this->voxelFragmentGenerationShader();
	ShaderProgram::use(generationShader);
	Engine::scene()->renderDirect(generationShader, dt, partialTicks);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	m_voxelFragmentCount = this->readAndClearAtomicCounter(m_voxelFragmentCounterBuffer);
}

void VoxelGenerator::octreeConstructionPass(double dt, double partialTicks) {
	ShaderProgram* nodeFlagShader = this->voxelOctreeNodeFlagShader();
	ShaderProgram* nodeAllocShader = this->voxelOctreeNodeAllocShader();
	ShaderProgram* nodeInitShader = this->voxelOctreeNodeInitShader();

	const int DATA_SIZE = 64;
	const int LOCAL_SIZE_X = 64;
	const int LOCAL_SIZE_Y = 16;
	OctreeConstructionData constructionData;
	constructionData.passLevelTileCount = 1;
	constructionData.passLevelTileStart = 0;
	constructionData.passTotalAllocCount = 1;
	constructionData.flagPassWorkgroupSizeX = DATA_SIZE / LOCAL_SIZE_X;
	constructionData.flagPassWorkgroupSizeY = ((m_voxelFragmentCount + (DATA_SIZE - 1)) / DATA_SIZE + (LOCAL_SIZE_Y - 1)) / LOCAL_SIZE_Y;
	constructionData.flagPassWorkgroupSizeZ = 1;
	constructionData.allocPassWorkgroupSizeX = DATA_SIZE / LOCAL_SIZE_X;
	constructionData.allocPassWorkgroupSizeY = 1;
	constructionData.allocPassWorkgroupSizeZ = 1;
	constructionData.initPassWorkgroupSizeX = DATA_SIZE / LOCAL_SIZE_X;
	constructionData.initPassWorkgroupSizeY = 1;
	constructionData.initPassWorkgroupSizeZ = 1;

	constructionData.octreeLevel = 0;
	constructionData.voxelFragmentCount = (int)m_voxelFragmentCount;
	constructionData.voxelGridSize = (int)m_gridSize;
	//constructionData.voxelGridScale = (float)m_gridScale;
	constructionData.voxelGridCenter = fvec4(floor(m_gridCenter / m_gridScale) * m_gridScale, 0.0F);
	constructionData.stage = 0;

	constructionData.levelTileAllocCount = 0;
	constructionData.levelTileTotalCount = 0;
	constructionData.totalTileAllocCount = 1;


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeConstructionDataBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(OctreeConstructionData), &constructionData);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_octreeConstructionDataBuffer);

	glBindBuffer(GL_COPY_READ_BUFFER, m_resetBuffer);

	this->bindVoxelChildIndexTexture(0, m_octreeNodeTexture);
	this->bindVoxelDataStorageTexture(1, m_octreeNodeTexture);
	//this->bindVoxelAlbedoTexture(1, m_octreeNodeTexture);
	//this->bindVoxelNormalTexture(2, m_octreeNodeTexture);
	//this->bindVoxelEmissiveTexture(3, m_octreeNodeTexture);
	this->bindVoxelLeafHeadIndexTexture(3, m_voxelFragmentTexture);

	this->bindVoxelPositionTexture(4, m_voxelFragmentTexture);
	this->bindVoxelDataStorageTexture(5, m_voxelFragmentTexture);
	//this->bindVoxelAlbedoTexture(5, m_voxelFragmentTexture);
	//this->bindVoxelNormalTexture(6, m_voxelFragmentTexture);
	//this->bindVoxelEmissiveTexture(7, m_voxelFragmentTexture);

	ShaderProgram::use(nodeFlagShader);
	nodeFlagShader->setUniform("maxLeafVoxelListLength", 10);

	//glFinish();
	//uint32_t a = Engine::instance()->getCurrentTime();

	glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_octreeConstructionDataBuffer); // first 9 integers are the workgroup sizes for each pass. This avoids cvopying to a dedicated indirect dispach buffer.
	for (int i = 0; i < m_octreeDepth; i++) {
		// FLAG PASS
		//nodeFlagShader->setUniform("stage", 0);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, sizeof(int32_t) * 0, offsetof(OctreeConstructionData, stage), sizeof(int32_t));
		glDispatchComputeIndirect(sizeof(uvec3) * 0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		// ALLOC PASS
		//nodeFlagShader->setUniform("stage", 1);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, sizeof(int32_t) * 1, offsetof(OctreeConstructionData, stage), sizeof(int32_t));
		glDispatchComputeIndirect(sizeof(uvec3) * 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

		// INIT PASS
		//nodeFlagShader->setUniform("stage", 2);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, sizeof(int32_t) * 2, offsetof(OctreeConstructionData, stage), sizeof(int32_t));
		glDispatchComputeIndirect(sizeof(uvec3) * 2);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
	}

	// LEAF NODE LIST CONSTRUCTION PASS
	//nodeFlagShader->setUniform("stage", 3);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, sizeof(int32_t) * 3, offsetof(OctreeConstructionData, stage), sizeof(int32_t));
	glDispatchComputeIndirect(sizeof(uvec3) * 0);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// LEAF NODE LIST AVERAGE PASS
	//nodeFlagShader->setUniform("stage", 4);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, sizeof(int32_t) * 4, offsetof(OctreeConstructionData, stage), sizeof(int32_t));
	glDispatchComputeIndirect(sizeof(uvec3) * 0);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//nodeFlagShader->setUniform("stage", 5);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, sizeof(int32_t) * 5, offsetof(OctreeConstructionData, stage), sizeof(int32_t));
	for (int i = 0; i < m_octreeDepth - 1; i++) {
		glDispatchComputeIndirect(sizeof(uvec3) * 0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	//glFinish();
	//uint32_t b = Engine::instance()->getCurrentTime();
	//info("Took %.2f msec to build octree\n", (b - a) / 1000000.0);

	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);

	////TODO: do this periodically, every few hundred milliseconds, and adjust allocations accordingly
	//OctreeConstructionData data;
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeConstructionDataBuffer);
	//glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(OctreeConstructionData), &data);
	//
	////m_octreeNodeCount = data.totalTileAllocCount * 8;// this->readAtomicCounter(m_octreeTotalTileAllocCounterBuffer) * 8; // why is this needed?
	//uint32_t totalUsed = data.totalTileAllocCount * 8;
	//double frac = (double)totalUsed / (double)m_octreeNodeAllocatedCount;
	//if (frac >= 1.0) {
	//	info("%d used / %d allocated (%.2f %%)\n", totalUsed, m_octreeNodeAllocatedCount, 100.0 * frac);
	//}
}

void VoxelGenerator::countOctreeNodes() {
	//uint32_t nodeCount = 0;
	//
	//for (int i = m_gridSize; i >= 1; i /= 2) {
	//	nodeCount += i * i * i;
	//}
	//
	//if (m_octreeNodeCount != nodeCount) {
	//	info("%d octree nodes\n", nodeCount);
	//	m_octreeNodeCount = nodeCount;
	//}

	m_octreeNodeCount = (uint32_t) (m_voxelFragmentCount * 2.5); // TODO: better way of predicting the required octree nodes...
}

void VoxelGenerator::octreeNodeFlagPass() {
}

void VoxelGenerator::octreeNodeAllocPass(uint32_t levelTileCount) {
}

void VoxelGenerator::octreeNodeInitPass(uint32_t childTileAllocCount) {
}

void VoxelGenerator::updateVoxelVisualizationTexture(bool clear) {
	bool allocate = false;
	if (m_voxelVisualizationTexture == 0) {
		glGenTextures(1, &m_voxelVisualizationTexture);
		allocate = true;
	}
	if (allocate || m_voxelVisualizationTextureSize != m_gridSize) {
		m_voxelVisualizationTextureSize = m_gridSize;
		glBindTexture(GL_TEXTURE_3D, m_voxelVisualizationTexture);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, m_gridSize, m_gridSize, m_gridSize, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_3D, 0);
	} else if (clear) {
		glClearTexImage(m_voxelVisualizationTexture, 0, GL_RGBA, GL_FLOAT, NULL);
	}
}

void VoxelGenerator::deleteVoxelVisualizationTexture() {
	glDeleteTextures(1, &m_voxelVisualizationTexture);
	m_voxelVisualizationTexture = 0;
	m_voxelVisualizationTextureSize = 0;
}

void VoxelGenerator::updateAxisProjections() {
	// TODO: check if needs update - center, size or scale changed?

	dvec3 p = floor(m_gridCenter / (m_gridScale * 2)) * (m_gridScale * 2);
	double d = m_gridSize * m_gridScale;
	double r = d * 0.5;
	dmat4 projectionMatrix = glm::ortho(-r, +r, -r, +r, 0.0, d);

	m_axisProjections[0] = projectionMatrix * glm::lookAt(p - dvec3(r, 0.0, 0.0), p, dvec3(0.0, 1.0, 0.0));
	m_axisProjections[1] = projectionMatrix * glm::lookAt(p - dvec3(0.0, r, 0.0), p, dvec3(0.0, 0.0, -1.0));
	m_axisProjections[2] = projectionMatrix * glm::lookAt(p - dvec3(0.0, 0.0, r), p, dvec3(0.0, 1.0, 0.0));
}

void VoxelGenerator::allocateTextureBuffer(uint64_t size, uint32_t format, uint32_t* textureHandlePtr, uint32_t* textureBufferHandlePtr) {
	glBindBuffer(GL_TEXTURE_BUFFER, *textureBufferHandlePtr);
	glBufferData(GL_TEXTURE_BUFFER, size, 0, GL_DYNAMIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, *textureHandlePtr);
	glTexBuffer(GL_TEXTURE_BUFFER, format, *textureBufferHandlePtr);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

void VoxelGenerator::allocateVoxelFragmentBuffers(bool shrinkToFit) {
	uint64_t allocationCount = 0;

	if (m_voxelFragmentCount > m_voxelFragmentAllocatedCount) {
		allocationCount = m_voxelFragmentCount + 2000; // allocate space for 2000 more voxels
	}

	if (allocationCount > 0) {
		info("Allocating %d voxel fragments (%.2f MiB)\n", allocationCount, (allocationCount * (8 + 16 + 4 + sizeof(VoxelLinkedListNode))) / (1024.0 * 1024.0));
		m_voxelFragmentAllocatedCount = allocationCount;
		this->allocateTextureBuffer(allocationCount * 8, GL_RGBA16UI, &m_voxelFragmentTexture[POSITION], &m_voxelFragmentTextureBuffer[POSITION]); // POSITION
		this->allocateTextureBuffer(allocationCount * 16, GL_RGBA32UI, &m_voxelFragmentTexture[DATA_STORAGE], &m_voxelFragmentTextureBuffer[DATA_STORAGE]); // DATA STORAGE
		//this->allocateTextureBuffer(allocationCount * 4, GL_RGBA8, &m_voxelFragmentTexture[ALBEDO], &m_voxelFragmentTextureBuffer[ALBEDO]); // ALBEDO
		//this->allocateTextureBuffer(allocationCount * 4, GL_RG16F, &m_voxelFragmentTexture[NORMAL], &m_voxelFragmentTextureBuffer[NORMAL]); // NORMAL
		//this->allocateTextureBuffer(allocationCount * 4, GL_RGB10_A2, &m_voxelFragmentTexture[EMISSIVE], &m_voxelFragmentTextureBuffer[EMISSIVE]); // EMISSIVE
		this->allocateTextureBuffer(allocationCount * 4, GL_R32UI, &m_voxelFragmentTexture[LEAF_HEAD_INDEX], &m_voxelFragmentTextureBuffer[LEAF_HEAD_INDEX]); // LEAF HEAD INDEX

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_octreeConstructionDataBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(OctreeConstructionData) + sizeof(VoxelLinkedListNode) * allocationCount, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}

void VoxelGenerator::allocateOctreeNodeBuffers() {
	uint64_t allocationCount = 0;

	if (m_octreeNodeCount > m_octreeNodeAllocatedCount) {
		allocationCount = m_octreeNodeCount;
	}

	if (allocationCount > 0) {
		info("Allocating %d octree nodes (%.2f MiB)\n", allocationCount, (allocationCount * (4 + 16)) / (1024.0 * 1024.0));
		m_octreeNodeAllocatedCount = allocationCount;
		this->allocateTextureBuffer(allocationCount * 4, GL_R32UI, &m_octreeNodeTexture[CHILD_INDEX], &m_octreeNodeTextureBuffer[CHILD_INDEX]); // CHILD INDEX
		this->allocateTextureBuffer(allocationCount * 16, GL_RGBA32UI, &m_octreeNodeTexture[DATA_STORAGE], &m_octreeNodeTextureBuffer[DATA_STORAGE]); // DATA STORAGE
		//this->allocateTextureBuffer(allocationCount * 4, GL_RGBA8, &m_octreeNodeTexture[ALBEDO], &m_octreeNodeTextureBuffer[ALBEDO]); // ALBEDO
		//this->allocateTextureBuffer(allocationCount * 4, GL_RG16F, &m_octreeNodeTexture[NORMAL], &m_octreeNodeTextureBuffer[NORMAL]); // NORMAL
		//this->allocateTextureBuffer(allocationCount * 4, GL_RGB10_A2, &m_octreeNodeTexture[EMISSIVE], &m_octreeNodeTextureBuffer[EMISSIVE]); // EMISSIVE
	}
}

void VoxelGenerator::bindVoxelPositionTexture(uint32_t unit, uint32_t textures[5]) {
	glBindImageTexture(unit, textures[POSITION], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16UI);
}

void VoxelGenerator::bindVoxelChildIndexTexture(uint32_t unit, uint32_t textures[5]) {
	glBindImageTexture(unit, textures[CHILD_INDEX], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
}

void VoxelGenerator::bindVoxelDataStorageTexture(uint32_t unit, uint32_t textures[5]) {
	glBindImageTexture(unit, textures[DATA_STORAGE], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32UI);
}

void VoxelGenerator::bindVoxelAlbedoTexture(uint32_t unit, uint32_t textures[5]) {
	glBindImageTexture(unit, textures[ALBEDO], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
}

void VoxelGenerator::bindVoxelNormalTexture(uint32_t unit, uint32_t textures[5]) {
	glBindImageTexture(unit, textures[NORMAL], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG16F);
}

void VoxelGenerator::bindVoxelEmissiveTexture(uint32_t unit, uint32_t textures[5]) {
	glBindImageTexture(unit, textures[EMISSIVE], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGB10_A2);
}

void VoxelGenerator::bindVoxelLeafHeadIndexTexture(uint32_t unit, uint32_t textures[5]) {
	glBindImageTexture(unit, textures[LEAF_HEAD_INDEX], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
}

uint32_t VoxelGenerator::readAndClearAtomicCounter(uint32_t atomicCounterHandle) {
	return this->readAndSetAtomicCounter(atomicCounterHandle, 0);
}

uint32_t VoxelGenerator::readAtomicCounter(uint32_t atomicCounterHandle) {
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounterHandle);
	//uint32_t* data = (uint32_t*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(uint32_t), GL_MAP_READ_BIT);
	uint32_t count = 0;// *data;
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(uint32_t), &count);
	//glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	return count;
}

uint32_t VoxelGenerator::readAndSetAtomicCounter(uint32_t atomicCounterHandle, uint32_t value) {
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounterHandle);
	uint32_t* data = (uint32_t*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(uint32_t), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
	uint32_t count = *data;
	//memset(uniformData, value, sizeof(uint32_t));
	*data = value;
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	return count;
}

uint32_t VoxelGenerator::setAtomicCounter(uint32_t atomicCounterHandle, uint32_t value) {
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounterHandle);
	uint32_t* data = (uint32_t*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(uint32_t), GL_MAP_WRITE_BIT);
	//memset(uniformData, value, sizeof(uint32_t));
	*data = value;
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	return value;
}

void VoxelGenerator::clearAtomicCounter(uint32_t atomicCounterHandle) {
	glBindBuffer(GL_COPY_READ_BUFFER, m_resetBuffer);
	glBindBuffer(GL_COPY_WRITE_BUFFER, atomicCounterHandle);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(uint32_t));

	//uint32_t zero = 0;
	//glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounterHandle);
	//glInvalidateBufferData(GL_ATOMIC_COUNTER_BUFFER);
	//glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
}

ShaderProgram* VoxelGenerator::voxelFragmentGenerationShader() {
	if (m_voxelFragmentGenerationShader == NULL) {
		m_voxelFragmentGenerationShader = new ShaderProgram();
		m_voxelFragmentGenerationShader->addShader(GL_VERTEX_SHADER, "shaders/voxel/generation/vert.glsl");
		m_voxelFragmentGenerationShader->addShader(GL_GEOMETRY_SHADER, "shaders/voxel/generation/geom.glsl");
		m_voxelFragmentGenerationShader->addShader(GL_FRAGMENT_SHADER, "shaders/voxel/generation/frag.glsl");
		Mesh::addVertexInputs(m_voxelFragmentGenerationShader);
		m_voxelFragmentGenerationShader->completeProgram();
	}

	return m_voxelFragmentGenerationShader;
}

ShaderProgram* VoxelGenerator::voxelOctreeNodeFlagShader() {
	if (m_voxelOctreeNodeFlagShader == NULL) {
		m_voxelOctreeNodeFlagShader = new ShaderProgram();
		m_voxelOctreeNodeFlagShader->addShader(GL_COMPUTE_SHADER, "shaders/voxel/octreeFlag/comp.glsl");
		m_voxelOctreeNodeFlagShader->completeProgram();
	}

	return m_voxelOctreeNodeFlagShader;
}

ShaderProgram* VoxelGenerator::voxelOctreeNodeAllocShader() {
	if (m_voxelOctreeNodeAllocShader == NULL) {
		m_voxelOctreeNodeAllocShader = new ShaderProgram();
		m_voxelOctreeNodeAllocShader->addShader(GL_COMPUTE_SHADER, "shaders/voxel/octreeAlloc/comp.glsl");
		m_voxelOctreeNodeAllocShader->completeProgram();
	}

	return m_voxelOctreeNodeAllocShader;
}

ShaderProgram* VoxelGenerator::voxelOctreeNodeInitShader() {
	if (m_voxelOctreeNodeInitShader == NULL) {
		m_voxelOctreeNodeInitShader = new ShaderProgram();
		m_voxelOctreeNodeInitShader->addShader(GL_COMPUTE_SHADER, "shaders/voxel/octreeInit/comp.glsl");
		m_voxelOctreeNodeInitShader->completeProgram();
	}

	return m_voxelOctreeNodeInitShader;
}

ShaderProgram* VoxelGenerator::voxelVisualizationCopyShader() {
	if (m_voxelVisualizationCopyShader == NULL) {
		m_voxelVisualizationCopyShader = new ShaderProgram();
		m_voxelVisualizationCopyShader->addShader(GL_COMPUTE_SHADER, "shaders/voxel/fragmentVisualization/comp.glsl");
		m_voxelVisualizationCopyShader->completeProgram();
	}

	return m_voxelVisualizationCopyShader;
}

ShaderProgram* VoxelGenerator::voxelFragmentVisualizationShader() {
	// Renders each voxel in the voxel fragment list directly as GL_POINTS expanded to cubes in geometry shader.
	if (m_voxelFragmentVisualizationShader == NULL) {
		m_voxelFragmentVisualizationShader = new ShaderProgram();
		m_voxelFragmentVisualizationShader->addShader(GL_VERTEX_SHADER, "shaders/voxel/fragmentVisualization/vert.glsl");
		m_voxelFragmentVisualizationShader->addShader(GL_GEOMETRY_SHADER, "shaders/voxel/fragmentVisualization/geom.glsl");
		m_voxelFragmentVisualizationShader->addShader(GL_FRAGMENT_SHADER, "shaders/voxel/fragmentVisualization/frag.glsl");
		Mesh::addVertexInputs(m_voxelFragmentVisualizationShader);
		m_voxelFragmentVisualizationShader->completeProgram();
	}

	return m_voxelFragmentVisualizationShader;
}

ShaderProgram* VoxelGenerator::voxelOctreeVisualizationShader() {
	// Renders a fullscreen quad and raytraces through the octree.
	if (m_voxelOctreeVisualizationShader == NULL) {
		m_voxelOctreeVisualizationShader = new ShaderProgram();
		m_voxelOctreeVisualizationShader->addShader(GL_VERTEX_SHADER, "shaders/voxel/octreeVisualization/vert.glsl");
		m_voxelOctreeVisualizationShader->addShader(GL_FRAGMENT_SHADER, "shaders/voxel/octreeVisualization/frag.glsl");
		Mesh::addVertexInputs(m_voxelOctreeVisualizationShader);
		m_voxelOctreeVisualizationShader->completeProgram();
	}

	return m_voxelOctreeVisualizationShader;
}
