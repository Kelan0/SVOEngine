#include "core/util/FileUtils.h"
#include <png.h>

bool FileUtils::loadPNG(std::string file, PNGFile& dest, bool verticalFlip) {
	// Copied straight from https://blog.nobel-joergensen.com/2010/11/07/loading-a-png-as-texture-in-opengl-using-libpng/
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	int color_type, interlace_type;
	FILE* fp;

	if ((fp = fopen(file.c_str(), "rb")) == NULL) {
		return false;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL) {
		fclose(fp);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fclose(fp);
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return false;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return false;
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, sig_read);
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, NULL);

	png_uint_32 width, height;
	int bit_depth;
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
    //png_read_update_info(png_ptr, info_ptr);
	info("%d\n", sizeof(png_byte));
	dest.width = width;
	dest.height = height;
	dest.hasAlpha = (color_type & PNG_COLOR_MASK_ALPHA);
	dest.channels = (color_type & PNG_COLOR_MASK_COLOR) ? 3 : 1;
	dest.channelBitDepth = bit_depth;

	unsigned int row_bytes = png_get_rowbytes(png_ptr, info_ptr);
	dest.data = (unsigned char*)malloc(row_bytes * height * sizeof(png_byte));
	dest.dataLength = row_bytes * height;

	if (dest.data == NULL) {
		debug("Failed to allocate memory while loading texture %s\n", file.c_str());
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return false;
	}

	png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);

	for (int i = 0; i < height; i++) {
		int y = verticalFlip ? (height - (i + 1)) : i;
		memcpy(dest.data + row_bytes * y, row_pointers[i], row_bytes);
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	fclose(fp);
	return true;
}

bool FileUtils::loadImage(std::string file, Image& dest, bool verticalFlip, bool floatingPoint) {
	int w, h, c = 4;
	stbi_set_flip_vertically_on_load(verticalFlip);
	void* data;
	if (floatingPoint)
		data = stbi_loadf(file.c_str(), &w, &h, NULL, STBI_rgb_alpha);
	else
		data = stbi_load(file.c_str(), &w, &h, NULL, STBI_rgb_alpha);
	if (!data) {
		error("Failed to load image file \"%s\" - %s\n", file.c_str(), stbi_failure_reason());
		return false;
	}

	//uint32_t len = (w + 3) * h * c;
	//dest.data = reinterpret_cast<uint8_t*>(malloc(len));
	//if (dest.data == NULL) {
	//	return false;
	//}

	dest.data = reinterpret_cast<uint8_t*>(data);
	dest.width = w;
	dest.height = h;
	dest.channels = c;
	dest.dataLength = w * h * c;
	return true;
}

bool FileUtils::loadFile(std::string file, std::string& dest, bool logError) {
	std::ifstream fileStream(file.c_str(), std::ios::in);

	if (!fileStream.is_open()) {
		if (logError) {
			warn("Failed to load file: %s\n", file.c_str());
		}
		return false;
	}
	dest = std::string();

	while (!fileStream.eof()) {
		std::string temp = "";
		getline(fileStream, temp);
		dest.append(temp + "\n");
	}

	fileStream.close();
	return true;
}

bool FileUtils::loadFileAttemptPaths(std::vector<std::string> paths, std::string& dest, bool logError) {
	if (paths.empty()) {
		if (logError) {
			error("Could not load file with no specified paths to search\n");
		}
		return false;
	}

	paths = std::vector<std::string>(paths); // copy

	for (int i = 0; i < paths.size(); i++) {

		if (loadFile(paths[i], dest, false)) {
			return true;
		}
	}

	if (logError) {
		warn("Failed to load file %s\n", paths[0].c_str());

		for (int i = 1; i < paths.size(); i++) {
			warn("Searched in directory %s\n", paths[i].c_str());
		}
	}

	return false;
}