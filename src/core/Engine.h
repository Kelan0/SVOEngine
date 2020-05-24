#pragma once

#include "core/pch.h"

struct UpdateState {
	uint64_t lastTick; // Nanosecond timestamp of last tick
	double partialTicks; // The interpolation between the previous and next tick
	double tickInterval; // The constant (expected) interval in seconds between ticks. 0 for unlimited
	double deltaTime;  // The time in seconds elapsed between the previous and current tick.
	double smoothedDeltaTime; // Delta time smoothed over time for a less eratic print-out.
	uint32_t tickCount; // The number of ticks counted since this was last reset.
	bool didUpdate;
};

struct Window {
	SDL_Window* handle;
	SDL_GLContext context;
	ivec2 size;
	std::string title;
};

class InputHandler;
class ResourceHandler;
class ScreenRenderer;
class RaytraceRenderer;
class SceneGraph;

class Engine : private NotCopyable {
public:
	static void create(int argc, char** argv);

	static void destroy();

	static Engine* instance();

	static InputHandler* inputHandler();

	static ResourceHandler* resourceHandler();

	static ScreenRenderer* screenRenderer();

	static RaytraceRenderer* raytraceRenderer();

	static SceneGraph* scene();

	bool update();

	std::string getResourceDirectory() const;

	uint64_t getCurrentTime() const;

	uint64_t getStartTime() const;

	double getRunTime() const;

	bool isStopped() const;

	bool isDebugRenderWireframeEnabled() const;

	void setDebugRenderWireframeEnabled(bool enabled);

	bool isDebugRenderLightingEnabled() const;

	void setDebugRenderLightingEnabled(bool enabled);

	bool isDebugRenderVoxelGridEnabled() const;

	void setDebugRenderVoxelGridEnabled(bool enabled);

	bool isImageBasedLightingEnabled() const;

	void setImageBasedLightingEnabled(bool enabled);

	int32_t getDebugRenderGBufferMode() const;

	void setDebugRenderGBufferMode(int32_t mode);

	bool didUpdateTick() const;

	bool didUpdateFrame() const;

	double getPartialTicks() const;

	double getPartialFrame() const;

	double getTickDeltaTime() const;

	double getFrameDeltaTime() const;

	ivec2 getWindowSize() const;

	dvec2 getWindowCenter() const;

	void setWindowSize(ivec2 size);

	double getWindowAspectRatio() const;

	InputHandler* getInputHandler() const;

	ResourceHandler* getResourceHandler() const;

	ScreenRenderer* getScreenRenderer() const;

	RaytraceRenderer* getRaytraceRenderer() const;

	SceneGraph* getScene() const;

	bool hasGLContext() const;
private:
	Engine(int argc, char** argv);

	~Engine();

	bool init();

	bool parseLaunchArgs(int argc, char** argv);

	bool initWindow();

	bool initInputHandler();

	bool initResourceHandler();

	bool initRenderer();

	bool initScene();

	bool initGui();

	bool updateFrame(double dt, double partialTicks);

	bool updateTick(double dt);

	static Engine* s_instance;

	std::string m_resourceDirectory;
	uint64_t m_startTime;
	bool m_stopped;
	bool m_debugRenderWireframe;
	bool m_debugRenderLighting;
	bool m_debugRenderVoxelGrid;
	bool m_debugEnabledImageBasedLighting;
	int32_t m_debugRenderGBufferMode;
	UpdateState m_tickState;
	UpdateState m_frameState;
	UpdateState m_debugState;
	Window m_window;
	InputHandler* m_inputHandler;
	ResourceHandler* m_resourceHandler;
	ScreenRenderer* m_screenRenderer;
	RaytraceRenderer* m_raytraceRenderer;
	SceneGraph* m_scene;
};