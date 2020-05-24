#pragma once

#include "core/pch.h"

#define PROFILE_SCOPE(name) ScopedProfile VAR(profile_)##VAR(__LINE__)(name);
#define PROFILE_EXPR(expr) { PROFILE_SCOPE(#expr); expr; }

class Profiler;
struct Profile;
struct GPUQuery;
struct ProfileLayer;
class ScopedProfile;

enum class ProfileSide {
	CPU, GPU
};

class Profiler {
public:
	using clock = std::chrono::high_resolution_clock;

	static Profiler* startProfiling(std::string name);

	static Profiler* currentProfiler();

	static bool isProfiling();

	static void stopProfiling();

	static uint32_t push(std::string name);

	static uint32_t pop();

	static Profile* currentFrame();

	void renderUI();

private:
	Profiler(std::string name);

	~Profiler();

	uint32_t pushProfile(std::string name);

	uint32_t popProfile();

	Profile* getCurrentFrame();

	double getElapsedTime(Profile* profile, ProfileSide side);

	void buildProfileTree(Profile* root, Profile* profile);

	void renderFrameGraph(int frameCount, int graphHeight, ProfileSide side);

	void renderFrameTimeOverlay(const char* str, double x, double y, double xmin, double ymin, double xmax, double ymax);

	bool renderFrameSegment(Profile* profile, ProfileSide side, double x0, double y0, double x1, double y1, double xmin, double ymin, double xmax, double ymax, double segmentSpacing);

	void initializeProfileLayers(Profile* profile, std::string treePath);

	bool tryFreeGPUQuery(Profile* profile);

	uint32_t createProfile(std::string name, uint32_t parentIndex);

	Profile* getProfile(uint32_t index);

	Profile* getFrame(uint32_t index);

	std::string m_name;
	std::vector<Profile> m_profiles; // Vector of all existing profiles - nothing should ever store a pointer, only an index, since this vector may be reallocated and pointers invalidated.
	std::vector<uint32_t> m_frames; // Index of all root profiles (representing entire frames)
	std::vector<ProfileLayer> m_layers; // All profile layers, referenced internally by all profiles
	std::map<std::string, uint32_t> m_frameLayers; // Map of unique profile tree paths to a layer index
	std::vector<uint32_t> m_unfreedGPUProfiles; // Vector of profilers with unfreed GPU queries - Will get freed as soon as the query result is available
	uint32_t m_currentProfile; // The index of the root profile for the current frame

	static Profiler* s_currentProfiler;
};

struct Profile {
	std::string name; // The name of this profile, displayed in the UI tree
	uint64_t startTimeCPU; // The CPU start time in nanoseconds
	uint64_t endTimeCPU; // The CPU end time in nanoseconds
	uint64_t startTimeGPU; // The GPU start time in nanoseconds - only assigned once the async GPU query result is available
	uint64_t endTimeGPU; // The GPU end time in nanoseconds - only assigned once the async GPU query result is available
	GPUQuery* gpuQuery; // The async GPU Query object - If not NULL, the query has not yet been resolved
	std::vector<uint32_t> children; // The index of all child nodes to this profile
	uint32_t parent; // The index of the parent node of this profile
	uint32_t layerIndex; // The layer index of this profile
};

struct GPUQuery {
	uint32_t startQueryHandle; // The OpenGL query object handle for the start time
	uint32_t endQueryHandle; // The OpenGL query object handle for the end time
};

struct ProfileLayer {
	glm::vec3 colour; // The colour of all profilesi n this profile layer.
	bool expanded; // Is this profile layer expanded, revealing all child profiles
	bool hovering; // Is the mouse hovering over this profile layer
};

class ScopedProfile {
public:
	// When this object is instantiated, a profile is pushed
	ScopedProfile(std::string name);

	// When this object is destroyed (deleted or went out of scope), the profile is popped.
	~ScopedProfile();

private:
	uint32_t m_profile; // The index of the profile
};
