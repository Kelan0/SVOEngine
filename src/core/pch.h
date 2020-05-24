#pragma once

#define _CRT_SECURE_NO_WARNINGS
//#define GLM_SWIZZLE

#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <inttypes.h>
#include <typeindex>
#include <chrono>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <stack>
#include <thread>
#include <set>
#include <map>
#include <unordered_map>
#include <stb_image.h>
#include <SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
//#include <glm/gtx/vec_swizzle.hpp>

#define VAR(x) x

#define log(fmt, ...) printf(fmt, ##__VA_ARGS__)

#define info(fmt, ...) log(fmt, ##__VA_ARGS__)
#define warn(fmt, ...) log(fmt, ##__VA_ARGS__)
#define error(fmt, ...) log(fmt, ##__VA_ARGS__)
#define debug(fmt, ...) log(fmt, ##__VA_ARGS__)

using namespace glm;

// Mesh typedef available in any header including this PCH. Avoids having to include the mesh header.
#ifndef MESH
#define MESH
template <typename V, typename I, qualifier Q>
class _Mesh;
typedef _Mesh<float, uint32_t, highp> Mesh; // Mesh with single precision floating point vertices and 32bit unsigned integer indices
#endif

class NotCopyable {
public:
	NotCopyable(const NotCopyable&) = delete; // Delete copy constructor
	NotCopyable& operator= (const NotCopyable&) = delete; // Delete copy assignment
protected:
	NotCopyable() = default;
	~NotCopyable() = default;
};
