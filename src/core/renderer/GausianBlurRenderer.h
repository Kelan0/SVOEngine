#pragma once

#include "core/pch.h"

class ShaderProgram;
class Texture2D;

class GausianBlurRenderer {
public:
	static void blur(Texture2D* texture, uvec2 blurRadius);
private:
	static ShaderProgram* gausianBlurShader();

	static ShaderProgram* s_gausianBlurShader;
};