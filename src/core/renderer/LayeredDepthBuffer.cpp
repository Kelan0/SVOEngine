#include "LayeredDepthBuffer.h"
#include "core/renderer/ShaderProgram.h"

LayeredDepthBuffer::LayeredDepthBuffer(uint32_t width, uint32_t height, uint32_t nodeSize) {
	glGenTextures(1, &m_headTextureHandle);
	glGenBuffers(1, &m_nodeStorageBufferHandle);
	glGenBuffers(1, &m_nodeCounterBufferHandle);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_nodeCounterBufferHandle);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(uint32_t), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	this->setSize(width, height, nodeSize);
}

LayeredDepthBuffer::~LayeredDepthBuffer() {
	glDeleteTextures(1, &m_headTextureHandle);
	glDeleteBuffers(1, &m_nodeStorageBufferHandle);
	glDeleteBuffers(1, &m_nodeCounterBufferHandle);
}

void LayeredDepthBuffer::startRender() {
	uint32_t zero = 0;
	uint8_t ff = 0xFF;
	uint32_t nullNode = 0xFFFFFFFF;
	uint32_t bufferLength = 0;
	glClearTexSubImage(m_headTextureHandle, 0, 0, 0, 0, m_width, m_height, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &nullNode);
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_nodeStorageBufferHandle);
	//glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_allocatedNodes * m_nodeSize, NULL);
	//glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &zero); // set all bytes to 0xFF

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_nodeCounterBufferHandle);
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(uint32_t), &bufferLength);
	glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(uint32_t), &zero);
	
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);

	if (bufferLength > m_allocatedNodes) {
		info("Reallocating nodes, %d to %d (%d extra)\n", m_allocatedNodes, bufferLength, bufferLength - m_allocatedNodes);
		this->setAllocatedNodes(bufferLength, m_nodeSize);
	}

	//info("%d\n", bufferLength);
}

void LayeredDepthBuffer::finishRender() {
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glFinish();
}

void LayeredDepthBuffer::applyUniforms(ShaderProgram* shaderProgram) {
	glBindImageTexture(0, m_headTextureHandle, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_nodeStorageBufferHandle);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, m_nodeCounterBufferHandle);

	shaderProgram->setUniform("allocatedNodes", (int) m_allocatedNodes);
}

void LayeredDepthBuffer::setSize(uint32_t width, uint32_t height, uint32_t nodeSize) {
	if (width != m_width || height != m_height || nodeSize != m_nodeSize) {
		if (width != m_width || height != m_height) {
			m_width = width;
			m_height = height;
			glBindTexture(GL_TEXTURE_2D, m_headTextureHandle);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		this->setAllocatedNodes(1 * width * height, nodeSize);
	}
}

void LayeredDepthBuffer::setAllocatedNodes(uint32_t allocatedNodes, uint32_t nodeSize) {
	if (allocatedNodes != m_allocatedNodes || nodeSize != m_nodeSize) {
		m_allocatedNodes = allocatedNodes;
		m_nodeSize = nodeSize;

		uint64_t bufferSize = (uint64_t)allocatedNodes * (uint64_t)nodeSize;
		info("Allocating space for %d nodes (%d bytes per node, %f MiB allocated)\n", allocatedNodes, nodeSize, bufferSize / 1048576.0);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_nodeStorageBufferHandle);
		glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}

uint32_t LayeredDepthBuffer::getHeadTextureHandle() const {
	return m_headTextureHandle;
}

uint32_t LayeredDepthBuffer::getStorageBufferHandle() const {
	return m_nodeStorageBufferHandle;
}

uint32_t LayeredDepthBuffer::getNodeCounterBufferHandle() const {
	return m_nodeCounterBufferHandle;
}

uint32_t LayeredDepthBuffer::getNodeCount() const {
	return m_nodeCount;
}

void LayeredDepthBuffer::bindHeadTexture(uint32_t unit) {
	glBindImageTexture(unit, m_headTextureHandle, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
}

void LayeredDepthBuffer::bindNodeStorageBuffer(uint32_t index) {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_nodeStorageBufferHandle);
}

void LayeredDepthBuffer::bindNodeCounterBuffer(uint32_t index) {
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, index, m_nodeCounterBufferHandle);
}
