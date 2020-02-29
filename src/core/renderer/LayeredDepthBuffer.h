#pragma once
#include "core/pch.h"

class ShaderProgram;

class LayeredDepthBuffer {
public:
	LayeredDepthBuffer(uint32_t width, uint32_t height, uint32_t nodeSize);

	~LayeredDepthBuffer();

	void startRender();

	void finishRender();

	void applyUniforms(ShaderProgram* shaderProgram);

	void setSize(uint32_t width, uint32_t height, uint32_t nodeSize);

	void setAllocatedNodes(uint32_t allocatedNodes, uint32_t nodeSize);

	uint32_t getHeadTextureHandle() const;

	uint32_t getStorageBufferHandle() const;

	uint32_t getNodeCounterBufferHandle() const;

	uint32_t getNodeCount() const;

	void bindHeadTexture(uint32_t unit = 0);

	void bindNodeStorageBuffer(uint32_t index = 0);

	void bindNodeCounterBuffer(uint32_t index = 0);

private:
	uint32_t m_headTextureHandle; // Texture storing the head reference of each pixel linked list
	uint32_t m_nodeStorageBufferHandle; // Buffer storing all nodes of all pixel linked lists.
	uint32_t m_nodeCounterBufferHandle; // Counter storing the number of buffers.
	//uint32_t m_clearBuffer; // Empty buffer to copy into the head texture to clear it

	uint32_t m_nodeCount; // The node count of the previous render.
	uint32_t m_allocatedNodes; // The number of nodes currently allocated.

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_nodeSize;
};

