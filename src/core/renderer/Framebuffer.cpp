#include "Framebuffer.h"

Framebuffer::Framebuffer() {
	m_handle = 0;
	glGenFramebuffers(1, &m_handle);
}

Framebuffer::~Framebuffer() {
	glDeleteFramebuffers(1, &m_handle);
	m_handle = 0;
}

void Framebuffer::bind(uint32_t width, uint32_t height) {
	glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
	if (width > 0 && height > 0) {
		glViewport(0, 0, width, height);
	}
}

void Framebuffer::unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::setDrawBuffer(uint32_t mode) {
	glDrawBuffer(mode);
}

void Framebuffer::setReadBuffer(uint32_t mode) {
	glReadBuffer(mode);
}

void Framebuffer::setDrawBuffers(uint32_t count, uint32_t buffers[]) {
	glDrawBuffers(count, buffers);
}

void Framebuffer::createColourTextureAttachment2D(uint32_t attachment, uint32_t texture, uint32_t target, uint32_t level) {
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, target, texture, level);
}

void Framebuffer::createDepthTextureAttachment2D(uint32_t texture, uint32_t target, uint32_t level) {
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, texture, level);
}

void Framebuffer::createColourTextureAttachment(uint32_t attachment, uint32_t texture, uint32_t level) {
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, texture, level);
}

void Framebuffer::createDepthTextureAttachment(uint32_t texture, uint32_t level) {
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture, level);
}

void Framebuffer::createColourBufferAttachment(int32_t width, int32_t height, uint32_t colourbuffer, uint32_t attachment, uint32_t format) {
	glBindRenderbuffer(GL_RENDERBUFFER, colourbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, GL_RENDERBUFFER, colourbuffer);
}

void Framebuffer::createDepthBufferAttachment(int32_t width, int32_t height, uint32_t depthBuffer) {
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
}

void Framebuffer::createColourBufferAttachmentMultisample(int32_t width, int32_t height, uint32_t colourbuffer, uint32_t attachment, uint32_t format, uint32_t samples) {
	glBindRenderbuffer(GL_RENDERBUFFER, colourbuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, GL_RENDERBUFFER, colourbuffer);
}

void Framebuffer::createDepthBufferAttachmentMultisample(int32_t width, int32_t height, uint32_t depthBuffer, uint32_t samples) {
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
}

bool Framebuffer::checkStatus(bool printSuccess) {
	this->bind();
	uint32_t status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	bool valid = true;
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		valid = false;

		error("Framebuffer failed validation with error \"%s\"\n",
			status == GL_FRAMEBUFFER_UNDEFINED ? "GL_FRAMEBUFFER_UNDEFINED" :
			status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT ? "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" :
			status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT ? "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" :
			status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER ? "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" :
			status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER ? "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" :
			status == GL_FRAMEBUFFER_UNSUPPORTED ? "GL_FRAMEBUFFER_UNSUPPORTED" :
			status == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE ? "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" :
			status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS ? "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS" :
			"UNKNOWN_ERROR"
		);

	} else if (printSuccess) {
		info("Framebuffer successfully validated\n");
	}

	this->unbind();
	return valid;
}

uint32_t Framebuffer::genRenderBuffers() {
	uint32 renderBuffer; // Add render buffer to internal array for later cleanup? It was created here, so this class should be responsible for deleting it.
	glGenRenderbuffers(1, &renderBuffer);

	return renderBuffer;
}
