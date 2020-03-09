#include "core/renderer/GausianBlurRenderer.h"
#include "core/renderer/ShaderProgram.h"
#include "core/renderer/Texture.h"

ShaderProgram* GausianBlurRenderer::s_gausianBlurShader = NULL;

void GausianBlurRenderer::blur(Texture2D* texture, uvec2 blurRadius) {
	ShaderProgram* blurShader = GausianBlurRenderer::gausianBlurShader();

	ShaderProgram::use(blurShader);

	glBindImageTexture(0, texture->getTextureName(), 0, GL_FALSE, 0, GL_READ_WRITE, Texture::getOpenGLTextureFormat(texture->getFormat()).internalFormat);

	blurShader->setUniform("blurRadius", blurRadius);
	// TODO

	//ShaderProgram::use(NULL);
}

ShaderProgram* GausianBlurRenderer::gausianBlurShader() {
	if (s_gausianBlurShader == NULL) {
		s_gausianBlurShader = new ShaderProgram();
		s_gausianBlurShader->addShader(GL_COMPUTE_SHADER, "shaders/gausianBlur/comp.glsl");
		s_gausianBlurShader->completeProgram();
	}
	return s_gausianBlurShader;
}
