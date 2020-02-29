#pragma once

#include "core/pch.h"

class Framebuffer {
public:
	Framebuffer();

	~Framebuffer();

	void bind(uint32_t width = 0, uint32_t height = 0);

	void unbind();

	void setDrawBuffer(uint32_t mode);

	void setReadBuffer(uint32_t mode);

	void setDrawBuffers(uint32_t count, uint32_t buffers[]);

	void createColourTextureAttachment2D(uint32_t attachment, uint32_t texture, uint32_t target, uint32_t level = 0);

	void createDepthTextureAttachment2D(uint32_t texture, uint32_t target, uint32_t level = 0);

	void createColourTextureAttachment(uint32_t attachment, uint32_t texture, uint32_t level = 0);

	void createDepthTextureAttachment(uint32_t texture, uint32_t level = 0);

	void createColourBufferAttachment(int32_t width, int32_t height, uint32_t colourbuffer, uint32_t attachment, uint32_t format);

	void createDepthBufferAttachment(int32_t width, int32_t height, uint32_t depthBuffer);

	void createColourBufferAttachmentMultisample(int32_t width, int32_t height, uint32_t colourbuffer, uint32_t attachment, uint32_t format, uint32_t samples);

	void createDepthBufferAttachmentMultisample(int32_t width, int32_t height, uint32_t depthBuffer, uint32_t samples);

	bool checkStatus(bool printSuccess = false);

	uint32_t genRenderBuffers();

private:
	uint32_t m_handle;
};

