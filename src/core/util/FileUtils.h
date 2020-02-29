#pragma once

#include "core/pch.h"

#define RESOURCE_PATH(path) (Engine::instance()->getResourceDirectory() + "/" + std::string(path))

struct PNGFile {
	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t channels = 0;
	uint8_t channelBitDepth = 0;
	uint8_t* data = NULL;
	uint32_t dataLength = 0;
	bool hasAlpha = false;

	~PNGFile() {
		//delete[] data;
		if (data != NULL) {
			free(data);
		}
	}
};

struct Image {
	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t channels = 0;
	uint8_t* data = NULL;
	uint32_t dataLength = 0;

	~Image() {
		if (data != NULL) {
			stbi_image_free(data);
		}
	}
};

namespace FileUtils {
	bool loadPNG(std::string file, PNGFile& dest, bool verticalFlip = true);

	bool loadImage(std::string file, Image& dest, bool verticalFlip = true, bool floatingPoint = false);

	bool loadFile(std::string file, std::string& dest, bool logError = true);

	bool loadFileAttemptPaths(std::vector<std::string> paths, std::string& dest, bool logError = true);
}

