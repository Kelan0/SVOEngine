#include "core/Engine.h"
#include "core/InputHandler.h"
#include "core/ResourceHandler.h"
#include "core/profiler/Profiler.h"
#include "core/renderer/ScreenRenderer.h"
#include "core/renderer/RaytraceRenderer.h"
#include "core/renderer/LayeredDepthBuffer.h"
#include "core/scene/Scene.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_sdl.h>

// TODO: window resize

Engine* Engine::s_instance = NULL;

void Engine::create(int argc, char** argv) {
	info("Initializing engine\n");
	if (Engine::s_instance != NULL) {
		throw new std::runtime_error("Cannot instantiate multiple engine instances");
;	}

	Engine::s_instance = new Engine(argc, argv);

	if (!Engine::instance()->init()) {
		error("Failed to initialize engine. Shutting down\n");
		Engine::destroy();
		throw std::runtime_error("Error during engine initialization");
	}
}

void Engine::destroy() {
	info("Destroying engine instance\n");
	delete Engine::s_instance;
	Engine::s_instance = NULL;
}

Engine* Engine::instance() {
	return Engine::s_instance;
}

InputHandler* Engine::inputHandler() {
	return Engine::instance()->getInputHandler();
}

ResourceHandler* Engine::resourceHandler() {
	return Engine::instance()->getResourceHandler();
}

ScreenRenderer* Engine::screenRenderer() {
	return Engine::instance()->getScreenRenderer();
}

RaytraceRenderer* Engine::raytraceRenderer() {
	return Engine::instance()->getRaytraceRenderer();
}

SceneGraph* Engine::scene() {
	return Engine::instance()->getScene();
}



Engine::Engine(int argc, char** argv) {
	m_stopped = false;
	m_debugRenderLighting = true;
	m_debugRenderVoxelGrid = false;
	m_debugEnabledImageBasedLighting = true;
	m_debugRenderWireframe = false;
	m_debugRenderGBufferMode = 0;
	m_tickState.tickInterval = 1.0/60.0;
	m_frameState.tickInterval = 0.0;
	m_debugState.tickInterval = 1.0; // once per second
	m_window.size.x = 1600;
	m_window.size.y = 900;
	m_window.title = "Test Window";

	if (!this->parseLaunchArgs(argc, argv)) {
		throw std::runtime_error("Failed to parse engine launch arguments\n");
	}
	Profiler::startProfiling("Runtime");
}

Engine::~Engine() {
	info("Destroying GUI\n");
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	info("Deleting OpenGL context\n");
	SDL_GL_DeleteContext(m_window.context);

	info("Destroying SDL window\n");
	SDL_DestroyWindow(m_window.handle);
	SDL_Quit();
	Profiler::stopProfiling();
}

bool Engine::init() {
	if (!this->initWindow()) {
		error("Failed to initialize game window\n");
		return false;
	}

	if (!this->initInputHandler()) {
		error("Failed to initialize input handler\n");
		return false;
	}

	if (!this->initResourceHandler()) {
		error("Failed to initialize resource handler\n");
		return false;
	}

	if (!this->initRenderer()) {
		error("Failed to initialize renderer\n");
		return false;
	}

	if (!this->initScene()) {
		error("Failed to initialize scene\n");
		return false;
	}

	if (!this->initGui()) {
		error("Failed to initialize GUI\n");
		return false;
	}

	m_startTime = this->getCurrentTime();
	SDL_ShowWindow(m_window.handle);

	return true;
}

bool Engine::parseLaunchArgs(int argc, char** argv) {
#define BETWEEN(min, max) \
	if (argval < min || argval >= max) { \
		error("Launch argument %s was out of range. Parsed value %d is not between " #min " and " #max "\n", name, argval); \
		return false; \
	}

#define TO_INTEGER int32_t argval = std::stoi(argv[i]);
#define TO_STRING const char* argval = argv[i];

#define READ_ARG(handler, constraint, argname, assignment) \
	if (std::strcmp(argv[i], "--" #argname) == 0) { \
		const char* name = #argname; \
		if (++i >= argc) { \
			error("Launch argument '" #argname "' was not specified\n"); \
			return false; \
		} \
		handler; \
		constraint; \
		assignment; \
		info("Loaded launch argument " #argname " as %s\n", argv[i]); \
	}


	for (int i = 1; i < argc; i++) {
		READ_ARG(TO_INTEGER, BETWEEN(0, 100000), updaterate, m_tickState.tickInterval = argval <= 0 ? 0.0 : 1.0 / argval);
		READ_ARG(TO_INTEGER, BETWEEN(0, 100000), framerate, m_frameState.tickInterval = argval <= 0 ? 0.0 : 1.0 / argval);
		READ_ARG(TO_INTEGER, BETWEEN(0, 7680), width, m_window.size.x = argval);
		READ_ARG(TO_INTEGER, BETWEEN(0, 7680), height, m_window.size.y = argval);
		READ_ARG(TO_STRING,, resourcedir, m_resourceDirectory = std::string(argval));
	}

	return true;
}

bool Engine::initWindow() {
	// ........this is messy, why did I do this?
	info("Initializing game window...\n");

#define QUIT_AND_RETURN(ret) SDL_Quit(); return ret;
#define DO_NOTHING ;

#define SDL_CHECK(exp, fail, msg, ...) {								\
		info("" msg, ##__VA_ARGS__);									\
		if ((exp) != 0) {												\
			const char* errStr = SDL_GetError();						\
			if (errStr != NULL && errStr[0] != '\0')					\
				error("SDL error occurred at " #exp ": %s\n", errStr);	\
			else error("SDL error occurred at " #exp "\n");				\
			SDL_ClearError(); fail;										\
		}																\
	}


	SDL_CHECK(SDL_Init(SDL_INIT_VIDEO), QUIT_AND_RETURN(false), "Initializing SDL\n");
	SDL_CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4), QUIT_AND_RETURN(false), "Setting SDL-OpenGL context version attribute to OpenGL 4.3\n");
	SDL_CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3), QUIT_AND_RETURN(false));
	SDL_CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE), QUIT_AND_RETURN(false), "Setting SDL-OpenGL context profile attribute to CORE\n");
	SDL_CHECK(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE), DO_NOTHING, "Setting SDL-OpenGL context doublebuffer enabled attribute to TRUE\n");
	SDL_CHECK(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24), DO_NOTHING, "Setting SDL-OpenGL context depth buffer size attribute to 24 bits per pixel\n");

	SDL_Window* window = SDL_CreateWindow(m_window.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_window.size.x, m_window.size.y, SDL_WINDOW_OPENGL);
	SDL_CHECK(window == NULL, QUIT_AND_RETURN(false), "Creating SDL window handle\n");
	m_window.handle = window;

	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_CHECK(context == NULL, QUIT_AND_RETURN(false), "Creating SDL-OpenGL context\n");
	m_window.context = context;

	SDL_CHECK(SDL_GL_SetSwapInterval(0), DO_NOTHING);

#undef QUIT_AND_RETURN
#undef DO_NOTHING
#undef SDL_CHECK

	return true;
}

bool Engine::initInputHandler() {
	info("Initializing input handler\n");
	m_inputHandler = new InputHandler(m_window.handle);
	// init keybind handler
	// register default binds

	return true;
}

bool Engine::initResourceHandler() {
	info("Initializing resource handler\n");
	m_resourceHandler = new ResourceHandler(m_resourceDirectory);
	m_resourceHandler->setLoader<std::string>(new ResourceLoaders::TextLoader());
	m_resourceHandler->setLoader<Image>(new ResourceLoaders::ImageLoader());
	m_resourceHandler->setLoader<Texture2D>(new ResourceLoaders::Texture2DLoader());

	return true;
}

bool Engine::initRenderer() {
	info("Initializing renderers\n");
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		error("Failed to initialize GLEW: %s\n", glewGetErrorString(err));
		return false;
	}

	m_screenRenderer = new ScreenRenderer(m_window.size.x, m_window.size.y);
	m_raytraceRenderer = new RaytraceRenderer(m_window.size.x, m_window.size.y);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	//glEnable(GL_RASTERIZER_DISCARD); // headPointerTexture if bottlenecked by vertex processing
	return true;
}

bool Engine::initScene() {
	info("Initializing scene\n");
	m_scene = new SceneGraph();
	return true;
}

bool Engine::initGui() {
	info("Initializing GUI\n");

	ImGui::CreateContext();
	ImGui_ImplSDL2_InitForOpenGL(m_window.handle, m_window.context);
	ImGui_ImplOpenGL3_Init("#version 140");

	return true;
}


bool Engine::update() {
	static const double smoothingFactor = 0.1;
	uint64_t now = this->getCurrentTime();

	m_tickState.partialTicks = (now - m_tickState.lastTick) / (m_tickState.tickInterval * 1000000000.0);
	m_frameState.partialTicks = (now - m_frameState.lastTick) / (m_frameState.tickInterval * 1000000000.0);
	m_debugState.partialTicks = (now - m_debugState.lastTick) / (m_debugState.tickInterval * 1000000000.0);

	m_tickState.didUpdate = false;
	m_frameState.didUpdate = false;
	m_debugState.didUpdate = false;

	if (m_tickState.partialTicks >= 1.0) {
		double prevDeltaTime = m_tickState.deltaTime;
		m_tickState.deltaTime = (now - m_tickState.lastTick) / 1000000000.0;
		m_tickState.smoothedDeltaTime = prevDeltaTime * (1.0 - smoothingFactor) + m_tickState.deltaTime * smoothingFactor;
		m_tickState.tickCount++;
		m_tickState.partialTicks -= 1.0;
		m_tickState.lastTick = now;
		m_tickState.didUpdate = true;
		
		if (!this->updateTick(m_tickState.deltaTime)) {
			return false;
		}
	}

	if (m_frameState.partialTicks >= 1.0) {
		double prevDeltaTime = m_frameState.deltaTime;
		m_frameState.deltaTime = (now - m_frameState.lastTick) / 1000000000.0;
		m_frameState.smoothedDeltaTime = prevDeltaTime * (1.0 - smoothingFactor) + m_frameState.deltaTime * smoothingFactor;
		m_frameState.tickCount++;
		m_frameState.partialTicks = 0.0;
		m_frameState.lastTick = now;
		m_frameState.didUpdate = true;

		if (!this->updateFrame(m_frameState.deltaTime, m_tickState.partialTicks)) {
			return false;
		}
	}

	if (m_debugState.partialTicks >= 1.0) {
		m_debugState.deltaTime = (now - m_debugState.lastTick) / 1000000000.0;
		m_debugState.tickCount = 1;
		m_debugState.partialTicks = 0.0;
		m_debugState.lastTick = now;
		m_debugState.didUpdate = true;

		char dataStr[50];
		sprintf(dataStr, "%d fps, %d ups", m_frameState.tickCount, m_tickState.tickCount);
		SDL_SetWindowTitle(m_window.handle, (m_window.title + " [" + dataStr + "]").c_str());

		m_frameState.tickCount = 0;
		m_tickState.tickCount = 0;
	}

	//SDL_Delay(1);
	return true;
}

std::string Engine::getResourceDirectory() const {
	return m_resourceDirectory;
}

uint64_t Engine::getCurrentTime() const {
	return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

uint64_t Engine::getStartTime() const {
	return m_startTime;
}

double Engine::getRunTime() const {
	return (this->getCurrentTime() - this->getStartTime()) / 1000000000.0;
}

bool Engine::updateFrame(double dt, double partialTicks) {
	PROFILE_SCOPE("Frame");

	PROFILE_EXPR(SDL_GL_SwapWindow(m_window.handle));

	m_inputHandler->update();

	{
		PROFILE_SCOPE("SDL_PollEvent(&event)");

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);

			if (event.type == SDL_QUIT) {
				return false;
			}

			m_inputHandler->processEvent(event);
		}
	}


	{
		PROFILE_SCOPE("ImGui::NewFrame()");
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(m_window.handle);
		ImGui::NewFrame();
	}

	{
		PROFILE_SCOPE("RenderStage");
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		//glDepthMask(GL_FALSE);
		//glDepthFunc(GL_ALWAYS); // all fragments pass
		glPolygonMode(GL_FRONT_AND_BACK, m_debugRenderWireframe ? GL_LINE : GL_FILL);
		//glClearColor(0.25F, 0.5F, 0.75F, 1.0F);

		m_screenRenderer->bindFramebuffer();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//m_screenRenderer->getLayeredDepthBuffer()->startRender();
		m_scene->render(dt, partialTicks);
		glDepthMask(GL_TRUE);
		//m_screenRenderer->getLayeredDepthBuffer()->finishRender();
		m_screenRenderer->unbindFramebuffer();
		m_raytraceRenderer->render(dt, partialTicks);
		m_screenRenderer->render(dt, partialTicks);
		m_scene->postRender(dt, partialTicks);
		//m_renderer->drawScene(m_scene);
	}

	{
		PROFILE_SCOPE("ImGui::Render()");
		glViewport(0, 0, m_window.size.x, m_window.size.y);

		Profiler::currentProfiler()->renderUI();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	return true;
}

bool Engine::updateTick(double dt) {
	m_scene->update(dt);
	return true;
}

bool Engine::isStopped() const {
	return m_stopped;
}

bool Engine::isDebugRenderWireframeEnabled() const {
	return m_debugRenderWireframe;
}

void Engine::setDebugRenderWireframeEnabled(bool enabled) {
	m_debugRenderWireframe = enabled;
}

bool Engine::isDebugRenderLightingEnabled() const {
	return m_debugRenderLighting;
}

void Engine::setDebugRenderLightingEnabled(bool enabled) {
	m_debugRenderLighting = enabled;
}

bool Engine::isDebugRenderVoxelGridEnabled() const {
	return m_debugRenderVoxelGrid;
}

void Engine::setDebugRenderVoxelGridEnabled(bool enabled) {
	m_debugRenderVoxelGrid = enabled;
}

bool Engine::isImageBasedLightingEnabled() const {
	return m_debugEnabledImageBasedLighting;
}

void Engine::setImageBasedLightingEnabled(bool enabled) {
	m_debugEnabledImageBasedLighting = enabled;
}

int32_t Engine::getDebugRenderGBufferMode() const {
	return m_debugRenderGBufferMode;
}

void Engine::setDebugRenderGBufferMode(int32_t mode) {
	m_debugRenderGBufferMode = mode;
}

bool Engine::didUpdateTick() const {
	return m_tickState.didUpdate;
}

bool Engine::didUpdateFrame() const {
	return m_frameState.didUpdate;
}

double Engine::getPartialTicks() const {
	return m_tickState.partialTicks;
}

double Engine::getPartialFrame() const {
	return m_frameState.partialTicks;
}

double Engine::getTickDeltaTime() const {
	return m_tickState.deltaTime;
}

double Engine::getFrameDeltaTime() const {
	return m_frameState.deltaTime;
}

ivec2 Engine::getWindowSize() const {
	return m_window.size;
}

dvec2 Engine::getWindowCenter() const {
	return dvec2(m_window.size) * 0.5;
}

void Engine::setWindowSize(ivec2 size) {
	if (size.x > 0 && size.y > 0 && size != m_window.size) {
		SDL_SetWindowSize(m_window.handle, size.x, size.y);
		m_window.size = size;
	}
}

double Engine::getWindowAspectRatio() const {
	return double(m_window.size.x) / double(m_window.size.y);
}

InputHandler* Engine::getInputHandler() const {
	return m_inputHandler;
}

ResourceHandler* Engine::getResourceHandler() const {
	return m_resourceHandler;
}

ScreenRenderer* Engine::getScreenRenderer() const {
	return m_screenRenderer;
}

RaytraceRenderer* Engine::getRaytraceRenderer() const {
	return m_raytraceRenderer;
}

SceneGraph* Engine::getScene() const {
	return m_scene;
}

bool Engine::hasGLContext() const {
	return m_window.context != NULL;
}
