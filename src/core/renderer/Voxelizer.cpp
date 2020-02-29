#include "core/renderer/Voxelizer.h"
#include "core/renderer/ShaderProgram.h"
#include "core/renderer/ScreenRenderer.h"
#include "core/renderer/geometry/Mesh.h"
#include "core/scene/Scene.h"
#include "core/scene/Camera.h"
#include "core/Engine.h"

ShaderProgram* Voxelizer::s_voxelGenerationShader = NULL;
ShaderProgram* Voxelizer::s_voxelOctreeFlagShader = NULL;
ShaderProgram* Voxelizer::s_voxelOctreeAllocShader = NULL;
ShaderProgram* Voxelizer::s_voxelOctreeInitShader = NULL;
ShaderProgram* Voxelizer::s_voxelDirectLightInjectionShader = NULL;
ShaderProgram* Voxelizer::s_voxelVisualizationShader = NULL;
ShaderProgram* Voxelizer::s_voxelVisualizationCopyShader = NULL;
ShaderProgram* Voxelizer::s_voxelClearShader = NULL;

Voxelizer::Voxelizer(uint32_t size, double scale):
	m_size(0), m_scale(1.0) {

	glGenBuffers(1, &m_voxelFragmentCounterBuffer);
	glGenTextures(4, m_voxelFragmentListTexture);
	glGenBuffers(4, m_voxelFragmentListTextureBuffer);

	glGenBuffers(1, &m_octreeStorageCounterBuffer);
	glGenTextures(4, m_octreeStorageTexture);
	glGenBuffers(4, m_octreeStorageTextureBuffer);

	glGenTextures(1, &m_voxelVisualizationTexture);

	glGenVertexArrays(1, &m_tempVAO);

	this->setSize(size);
	this->setScale(scale);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_voxelFragmentCounterBuffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(uint32_t), NULL, GL_STATIC_DRAW);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_octreeStorageCounterBuffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(uint32_t), NULL, GL_STATIC_DRAW);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

Voxelizer::~Voxelizer() {
	glDeleteBuffers(1, &m_voxelFragmentCounterBuffer);
	glDeleteTextures(4, m_voxelFragmentListTexture);
	glDeleteBuffers(4, m_voxelFragmentListTextureBuffer);

	glDeleteBuffers(1, &m_octreeStorageCounterBuffer);
	glDeleteTextures(4, m_octreeStorageTexture);
	glDeleteBuffers(4, m_octreeStorageTextureBuffer);

	glDeleteTextures(1, &m_voxelVisualizationTexture);

	glDeleteVertexArrays(1, &m_tempVAO);
}

void Voxelizer::startRender() {
	this->updateAxisProjections();
	// // dont need to clear normal/emissive. Empty voxels are ignored
	// uint32_t zero = 0u;
	// glClearTexImage(m_albedoStorageTextureHandle, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
	// glClearTexImage(m_normalStorageTextureHandle, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
	// 
	// //glClearTexImage(m_emissiveStorageTextureHandle, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
	// 
	// // ShaderProgram* clearShader = Voxelizer::voxelClearShader();
	// // ShaderProgram::use(clearShader);
	// // this->bindAlbedoStorageTextureImage(0);
	// // this->bindNormalStorageTextureImage(1);
	// // this->bindEmissiveStorageTextureImage(2);
	// // glDispatchCompute(m_size / 32, m_size / 32, m_size / 1);
	// // ShaderProgram::use(NULL);

	glViewport(0, 0, m_size, m_size);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	//glEnable(GL_CONSERVATIVE_RASTERIZATION_NV); // TODO: MSAA instead of this
	//glEnable(GL_RASTERIZER_DISCARD);
}

void Voxelizer::finishRender() {
	Engine::instance()->screenRenderer()->initViewport();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	//glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
	//glDisable(GL_RASTERIZER_DISCARD);

	//this->renderDebug();
}

void Voxelizer::renderPass(double dt, double partialTicks) {
	ShaderProgram* generationShader = Voxelizer::voxelGenerationShader(); // generate voxel fragment list
	ShaderProgram* octreeFlagShader = Voxelizer::voxelOctreeFlagShader(); // flag octree nodes for subdivision
	ShaderProgram* octreeAllocShader = Voxelizer::voxelOctreeAllocShader(); // allocate flagged nodes
	ShaderProgram* octreeInitShader = Voxelizer::voxelOctreeInitShader(); // initialize allocated nodes to null
	ShaderProgram* directLightInjectionShader = Voxelizer::voxelDirectLightInjectionShader(); 

	glViewport(0, 0, m_size, m_size);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	//glEnable(GL_CONSERVATIVE_RASTERIZATION_NV); // TODO: MSAA instead of this


	// COUNT VOXELS
	m_voxelizationStage = FRAGMENT_COUNT;
	Engine::scene()->renderDirect(generationShader, dt, partialTicks);
	glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_voxelFragmentCounterBuffer);
	uint32_t* voxelCounterPtr = (uint32_t*) glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(uint32_t), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
	uint32_t voxelCount = *voxelCounterPtr;
	info("counted %d voxels\n", voxelCount);
	this->allocateVoxelFragmentList(voxelCount + 2000);
	*voxelCounterPtr = 0;
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);


	// STORE VOXELS
	m_voxelizationStage = FRAGMENT_STORAGE;
	Engine::scene()->renderDirect(generationShader, dt, partialTicks);


	Engine::instance()->screenRenderer()->initViewport();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	//glDisable(GL_CONSERVATIVE_RASTERIZATION_NV

	// // BUILD OCTREE
	// 
	// // bottom level has at most the number of voxels in teh voxel fragment list.
	// // subsequent levels are counter as if they were full.
	// uint32_t octreeNodeCount = m_voxelFragmentCount;
	// for (int i = m_size / 2; i >= 1; i /= 2) {
	// 	octreeNodeCount += i * i * i;
	// }
	// 
	// this->allocateOctreeStorageList(octreeNodeCount);
	// 
	// for (int i = m_size, level = 0; i >= 1; i /= 2, level++) {
	// 	m_voxelizationStage = OCTREE_FLAG;
	// 
	// 	ShaderProgram::use(octreeFlagShader);
	// 	this->applyUniforms(octreeFlagShader);
	// 	octreeFlagShader->setUniform("octreeLevel", i);
	// 	octreeFlagShader->setUniform("voxelFragmentCount", (int) m_voxelFragmentCount);
	// }
}

void Voxelizer::renderDebug() {
	ShaderProgram* visualizationCopyShader = Voxelizer::voxelVisualizationCopyShader();
	ShaderProgram* visualizationShader = Voxelizer::voxelVisualizationShader();

	info("%d voxel fragments\n", m_voxelFragmentCount);

	m_voxelizationStage = VISUALIZATION_COPY;
	ShaderProgram::use(visualizationCopyShader);
	Engine::scene()->applyUniforms(visualizationCopyShader);
	glDispatchCompute((m_voxelFragmentCount + 31) / 32, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	m_voxelizationStage = VISUALIZATION;
	ShaderProgram::use(visualizationShader);
	Engine::scene()->applyUniforms(visualizationShader);
	glBindVertexArray(m_tempVAO); // ?
	glDrawArrays(GL_POINTS, 0, m_size * m_size * m_size);
	glBindVertexArray(0);
	ShaderProgram::use(NULL);
}

void Voxelizer::applyUniforms(ShaderProgram* shaderProgram) {
	if (m_voxelizationStage == FRAGMENT_COUNT) {
		shaderProgram->setUniform("countVoxels", true);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_voxelFragmentCounterBuffer);
	} else if (m_voxelizationStage == FRAGMENT_STORAGE) {
		shaderProgram->setUniform("countVoxels", false);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_voxelFragmentCounterBuffer);
		glBindImageTexture(1, m_voxelFragmentListTexture[POSITION], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16UI);
		glBindImageTexture(2, m_voxelFragmentListTexture[ALBEDO], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
		glBindImageTexture(3, m_voxelFragmentListTexture[NORMAL], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG16F);
		glBindImageTexture(4, m_voxelFragmentListTexture[EMISSIVE], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGB10_A2);
	}
	//else if (m_voxelizationStage == OCTREE_FLAG) {
	//	glBindImageTexture(1, m_voxelFragmentListTexture[POSITION], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16UI);
	//	glBindImageTexture(5, m_octreeStorageTexture[CHILD_INDEX], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	//} else if (m_voxelizationStage == OCTREE_ALLOCATE) {
	//	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_octreeStorageCounterBuffer);
	//	glBindImageTexture(5, m_octreeStorageTexture[CHILD_INDEX], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	//} else if (m_voxelizationStage == OCTREE_INITIALIZE) {
	//	glBindImageTexture(1, m_octreeStorageTexture[CHILD_INDEX], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16UI);
	//	glBindImageTexture(2, m_octreeStorageTexture[ALBEDO], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
	//	glBindImageTexture(3, m_octreeStorageTexture[NORMAL], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG16F);
	//	glBindImageTexture(4, m_octreeStorageTexture[EMISSIVE], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGB10_A2);
	//} else 
	if (m_voxelizationStage == VISUALIZATION_COPY) {
		glBindImageTexture(0, m_voxelVisualizationTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
		glBindImageTexture(1, m_voxelFragmentListTexture[POSITION], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16UI);
		glBindImageTexture(2, m_voxelFragmentListTexture[ALBEDO], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
		glBindImageTexture(3, m_voxelFragmentListTexture[NORMAL], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG16F);
		glBindImageTexture(4, m_voxelFragmentListTexture[EMISSIVE], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGB10_A2);
	} else if (m_voxelizationStage == VISUALIZATION) {
		glBindImageTexture(0, m_voxelVisualizationTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
	}

	for (int i = 0; i < 3; i++) {
		shaderProgram->setUniform("axisProjections[" + std::to_string(i) + "]", m_axisProjections[i]);
		shaderProgram->setUniform("invAxisProjections[" + std::to_string(i) + "]", inverse(m_axisProjections[i]));
	}

	shaderProgram->setUniform("voxelGridScale", (float)m_scale);
	shaderProgram->setUniform("voxelGridCenter", floor(m_position / m_scale) * m_scale);
	shaderProgram->setUniform("voxelGridSize", (int)m_size);

	shaderProgram->setUniform("voxelFragmentCount", (int) m_voxelFragmentCount);
}

void Voxelizer::updateAxisProjections() {
	dvec3 p = floor(m_position / m_scale) * m_scale;
	double d = m_size * m_scale;
	double r = d * 0.5;
	dmat4 projectionMatrix = glm::ortho(-r, +r, -r, +r, 0.0, d);

	m_axisProjections[0] = projectionMatrix * glm::lookAt(p - dvec3(r, 0.0, 0.0), p, dvec3(0.0, 1.0, 0.0));
	m_axisProjections[1] = projectionMatrix * glm::lookAt(p - dvec3(0.0, r, 0.0), p, dvec3(0.0, 0.0, 1.0));
	m_axisProjections[2] = projectionMatrix * glm::lookAt(p - dvec3(0.0, 0.0, r), p, dvec3(0.0, 1.0, 0.0));
}

void Voxelizer::allocateVoxelFragmentList(uint64_t allocatedVoxels) {
	if (allocatedVoxels > m_voxelFragmentCount) {
		// 20 bytes per voxel - 8 byte position, 4 byte albedo, 4 byte normal, 4 byte emissive
		info("Allocating %d voxel fragments (%.2f MiB)\n", allocatedVoxels, (allocatedVoxels * 20) / 1048576.0);
		m_voxelFragmentCount = allocatedVoxels;

		this->initTextureBufferStorage(allocatedVoxels, GL_RGBA16UI, &m_voxelFragmentListTexture[POSITION], &m_voxelFragmentListTextureBuffer[POSITION]);
		this->initTextureBufferStorage(allocatedVoxels, GL_RGBA8, &m_voxelFragmentListTexture[ALBEDO], &m_voxelFragmentListTextureBuffer[ALBEDO]);
		this->initTextureBufferStorage(allocatedVoxels, GL_RG16F, &m_voxelFragmentListTexture[NORMAL], &m_voxelFragmentListTextureBuffer[NORMAL]);
		this->initTextureBufferStorage(allocatedVoxels, GL_RGB10_A2, &m_voxelFragmentListTexture[EMISSIVE], &m_voxelFragmentListTextureBuffer[EMISSIVE]);
	}
}

void Voxelizer::allocateOctreeStorageList(uint64_t allocatedVoxels) {
	if (allocatedVoxels > m_allocatedOctreeCount) {
		// 20 bytes per voxel - 8 byte position, 4 byte albedo, 4 byte normal, 4 byte emissive
		info("Allocating %d octree nodes (%.2f MiB)\n", allocatedVoxels, (allocatedVoxels * 20) / 1048576.0);
		m_allocatedOctreeCount = allocatedVoxels;

		this->initTextureBufferStorage(allocatedVoxels, GL_R32UI, &m_octreeStorageTexture[CHILD_INDEX], &m_octreeStorageTextureBuffer[CHILD_INDEX]);
		this->initTextureBufferStorage(allocatedVoxels, GL_RGBA8, &m_octreeStorageTexture[ALBEDO], &m_octreeStorageTextureBuffer[ALBEDO]);
		this->initTextureBufferStorage(allocatedVoxels, GL_RG16F, &m_octreeStorageTexture[NORMAL], &m_octreeStorageTextureBuffer[NORMAL]);
		this->initTextureBufferStorage(allocatedVoxels, GL_RGB10_A2, &m_octreeStorageTexture[EMISSIVE], &m_octreeStorageTextureBuffer[EMISSIVE]);
	}
}

void Voxelizer::setSize(uint32_t size) {
	if (size != m_size) {
		m_size = size;

		glBindTexture(GL_TEXTURE_3D, m_voxelVisualizationTexture);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, size, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);


		//glBindTexture(GL_TEXTURE_3D, m_albedoStorageTextureHandle);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		//glTexImage3D(GL_TEXTURE_3D, 0, GL_R32UI, size, size, size, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		//
		//glBindTexture(GL_TEXTURE_3D, m_normalStorageTextureHandle);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		//glTexImage3D(GL_TEXTURE_3D, 0, GL_R32UI, size, size, size, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		//
		//glBindTexture(GL_TEXTURE_3D, m_emissiveStorageTextureHandle);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		//glTexImage3D(GL_TEXTURE_3D, 0, GL_R32UI, size, size, size, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		
		glBindTexture(GL_TEXTURE_3D, 0);
	}
}

void Voxelizer::setScale(double scale) {
	m_scale = scale;
}

void Voxelizer::setPosition(dvec3 position) {
	m_position = position;
}

uint32_t Voxelizer::getSize() const {
	return m_size;
}

double Voxelizer::getScale() const {
	return m_scale;
}

dvec3 Voxelizer::getPosition() const {
	return m_position;
}

ShaderProgram* Voxelizer::voxelGenerationShader() {
	if (s_voxelGenerationShader == NULL) {
		s_voxelGenerationShader = new ShaderProgram();
		s_voxelGenerationShader->addShader(GL_VERTEX_SHADER, "shaders/voxel/generation/vert.glsl");
		s_voxelGenerationShader->addShader(GL_GEOMETRY_SHADER, "shaders/voxel/generation/geom.glsl");
		s_voxelGenerationShader->addShader(GL_FRAGMENT_SHADER, "shaders/voxel/generation/frag.glsl");
		Mesh::addVertexInputs(s_voxelGenerationShader);
		s_voxelGenerationShader->completeProgram();
	}

	return s_voxelGenerationShader;
}

ShaderProgram* Voxelizer::voxelOctreeFlagShader() {
	if (s_voxelOctreeFlagShader == NULL) {
		s_voxelOctreeFlagShader = new ShaderProgram();
		s_voxelOctreeFlagShader->addShader(GL_COMPUTE_SHADER, "shaders/voxel/octreeFlag/comp.glsl");
		s_voxelOctreeFlagShader->completeProgram();
	}

	return s_voxelOctreeFlagShader;
}

ShaderProgram* Voxelizer::voxelOctreeAllocShader() {
	if (s_voxelOctreeAllocShader == NULL) {
		s_voxelOctreeAllocShader = new ShaderProgram();
		s_voxelOctreeAllocShader->addShader(GL_COMPUTE_SHADER, "shaders/voxel/octreeAlloc/comp.glsl");
		s_voxelOctreeAllocShader->completeProgram();
	}

	return s_voxelOctreeAllocShader;
}

ShaderProgram* Voxelizer::voxelOctreeInitShader() {
	if (s_voxelOctreeInitShader == NULL) {
		s_voxelOctreeInitShader = new ShaderProgram();
		s_voxelOctreeInitShader->addShader(GL_COMPUTE_SHADER, "shaders/voxel/octreeInit/comp.glsl");
		s_voxelOctreeInitShader->completeProgram();
	}

	return s_voxelOctreeInitShader;
}

ShaderProgram* Voxelizer::voxelDirectLightInjectionShader() {
	if (s_voxelDirectLightInjectionShader == NULL) {
		s_voxelDirectLightInjectionShader = new ShaderProgram();
		s_voxelDirectLightInjectionShader->addShader(GL_COMPUTE_SHADER, "shaders/voxel/directLightInjection/comp.glsl");
		s_voxelDirectLightInjectionShader->completeProgram();
	}

	return s_voxelDirectLightInjectionShader;
}

ShaderProgram* Voxelizer::voxelVisualizationShader() {
	if (s_voxelVisualizationShader == NULL) {
		s_voxelVisualizationShader = new ShaderProgram();
		s_voxelVisualizationShader->addShader(GL_VERTEX_SHADER, "shaders/voxel/visualization/vert.glsl");
		s_voxelVisualizationShader->addShader(GL_GEOMETRY_SHADER, "shaders/voxel/visualization/geom.glsl");
		s_voxelVisualizationShader->addShader(GL_FRAGMENT_SHADER, "shaders/voxel/visualization/frag.glsl");
		Mesh::addVertexInputs(s_voxelVisualizationShader);
		s_voxelVisualizationShader->completeProgram();
	}

	return s_voxelVisualizationShader;
}

ShaderProgram* Voxelizer::voxelVisualizationCopyShader() {
	if (s_voxelVisualizationCopyShader == NULL) {
		s_voxelVisualizationCopyShader = new ShaderProgram();
		s_voxelVisualizationCopyShader->addShader(GL_COMPUTE_SHADER, "shaders/voxel/visualization/comp.glsl");
		s_voxelVisualizationCopyShader->completeProgram();
	}

	return s_voxelVisualizationCopyShader;
}

ShaderProgram* Voxelizer::voxelClearShader() {
	if (s_voxelClearShader == NULL) {
		s_voxelClearShader = new ShaderProgram();
		s_voxelClearShader->addShader(GL_COMPUTE_SHADER, "shaders/voxel/clear/comp.glsl");
		s_voxelClearShader->completeProgram();
	}

	return s_voxelClearShader;
}

void Voxelizer::initTextureBufferStorage(uint64_t size, uint32_t format, uint32_t* textureHandlePtr, uint32_t* textureBufferHandlePtr) {
	if (*textureHandlePtr != 0) glDeleteTextures(1, textureHandlePtr);
	if (*textureBufferHandlePtr != 0) glDeleteBuffers(1, textureBufferHandlePtr);

	glGenTextures(1, textureHandlePtr);
	glGenBuffers(1, textureBufferHandlePtr);

	glBindBuffer(GL_TEXTURE_BUFFER, *textureBufferHandlePtr);
	glBufferData(GL_TEXTURE_BUFFER, size, 0, GL_DYNAMIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, *textureHandlePtr);
	glTexBuffer(GL_TEXTURE_BUFFER, format, *textureBufferHandlePtr);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}
