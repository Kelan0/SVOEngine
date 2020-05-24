#include "Profiler.h"
#include "core/Engine.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

Profiler* Profiler::s_currentProfiler = NULL;


ScopedProfile::ScopedProfile(std::string name) {
	m_profile = Profiler::push(name);
}

ScopedProfile::~ScopedProfile() {
	assert(Profiler::pop() == m_profile);
}

Profiler::Profiler(std::string name):
	m_name(name) {

	info("Starting profiler \"%s\"\n", m_name.c_str());
}

Profiler::~Profiler() {
	info("Stopping profiler \"%s\"\n", m_name.c_str());
}

uint32_t Profiler::pushProfile(std::string name) {
	uint32_t parentIndex = m_currentProfile;
	uint32_t profileIndex = this->createProfile(name, parentIndex);

	if (parentIndex != 0) {
		// The current profile is not NULL, push this profile as a child
		this->getProfile(parentIndex)->children.emplace_back(profileIndex);
	} else {
		// The current profile was NULL, push this profile as a new root (new frame)
		m_frames.emplace_back(profileIndex);
	}

	m_currentProfile = profileIndex;
	return profileIndex;
}

uint32_t Profiler::popProfile() {
	uint32_t profileIndex = m_currentProfile;
	Profile* profile = this->getProfile(profileIndex);
	profile->endTimeCPU = clock::now().time_since_epoch().count();

	m_currentProfile = profile->parent;

	if (profile->gpuQuery != NULL) {
		glQueryCounter(profile->gpuQuery->endQueryHandle, GL_TIMESTAMP);
		m_unfreedGPUProfiles.push_back(profileIndex);
	}

	return profileIndex;
}

Profile* Profiler::getCurrentFrame() {
	if (m_frames.empty())
		return NULL;

	return this->getProfile(m_frames.back());
}

double Profiler::getElapsedTime(Profile* profile, ProfileSide side) {
	if (side == ProfileSide::CPU)
		return (profile->endTimeCPU - profile->startTimeCPU) / 1000000.0;
	else
		return (profile->endTimeGPU - profile->startTimeGPU) / 1000000.0;
}

void Profiler::buildProfileTree(Profile* root, Profile* profile) {

	Profile* parent = this->getProfile(profile->parent);
	ProfileLayer* layer = &m_layers[profile->layerIndex];

	ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_FramePadding;

	if (layer->hovering) {
		treeNodeFlags |= ImGuiTreeNodeFlags_Framed;

		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			ImGui::SetNextTreeNodeOpen(true);
	}

	if (profile->children.empty())
		treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;

	layer->expanded = ImGui::TreeNodeEx(profile->name.c_str(), treeNodeFlags);

	double elapsedCPU = getElapsedTime(profile, ProfileSide::CPU);
	double elapsedGPU = getElapsedTime(profile, ProfileSide::GPU);

	ImGui::SameLine();
	ImGui::NextColumn(); // CPU time

	ImGui::Text("%.2f msec", elapsedCPU);

	ImGui::NextColumn(); // CPU %
	if (profile != root) {
		ImGui::Text("%.2f %%", elapsedCPU / getElapsedTime(root, ProfileSide::CPU) * 100.0);
	}

	ImGui::NextColumn(); // GPU time

	ImGui::Text("%.2f msec", elapsedGPU);

	ImGui::NextColumn(); // GPU %
	if (profile != root) {
		ImGui::Text("%.2f %%", elapsedGPU / getElapsedTime(root, ProfileSide::CPU) * 100.0);
	}

	ImGui::NextColumn(); // Colour
	ImGui::ColorEdit3(profile->name.c_str(), &layer->colour[0], ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs);

	ImGui::NextColumn();

	if (layer->expanded) {
		for (int i = 0; i < profile->children.size(); i++) {
			buildProfileTree(root, this->getProfile(profile->children[i]));
		}

		ImGui::TreePop();
	}
}

void Profiler::renderFrameGraph(int frameCount, int graphHeight, ProfileSide side) {
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	ImGui::PushItemWidth(-1); // auto calculate width

	int graphWidth = ImGui::CalcItemWidth();
	int padding = 1;
	int margin = 1;
	int segmentSpacing = 1;

	if (frameCount < 0) {
		frameCount = graphWidth;
	}

	ImVec2 cursor = window->DC.CursorPos;

	if (graphHeight < 0) {
		graphHeight = ImGui::GetContentRegionMaxAbs().y - cursor.y;
	}

	ImVec2 bbsize = ImVec2(graphWidth, graphHeight);
	ImVec2 bbmin = ImVec2(cursor.x, cursor.y);
	ImVec2 bbmax = ImVec2(bbmin.x + bbsize.x, bbmin.y + bbsize.y);
	ImVec2 bbminInner = ImVec2(bbmin.x + padding, bbmin.y + padding);
	ImVec2 bbmaxInner = ImVec2(bbmax.x - padding, bbmax.y - padding);
	ImVec2 bbminOuter = ImVec2(bbmin.x - margin, bbmin.y - margin);
	ImVec2 bbmaxOuter = ImVec2(bbmax.x + margin, bbmax.y + margin);

	const ImU32 borderCol = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0F, 1.0F, 1.0F, 1.0F));
	const ImU32 fillCol = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0F, 1.0F, 0.0F, 1.0F));

	ImDrawList* dl = ImGui::GetWindowDrawList();

	dl->AddRect(bbminOuter, bbmaxOuter, borderCol);

	int frameWidth = (int)ceil((double) graphWidth / (double) frameCount);

	double maxFrameTime = 25.0F;

	double frameTimeWeight = 0.05;

	double hoveringFrameTime = -1.0;

	std::vector<ImVec2> rollingFrameAverageLine;

	ImGui::PushClipRect(bbminInner, bbmaxInner, true);
	for (int i = 0; i < frameCount + 50; i++) {
		Profile* profile = this->getFrame(i);

		if (profile == NULL)
			break;

		double elapsed = getElapsedTime(profile, side);

		double x1 = bbmax.x - (i * (frameWidth + segmentSpacing) + padding);
		double x0 = x1 - frameWidth;
		double y1 = bbmax.y;
		double y0 = y1 - elapsed / maxFrameTime * graphHeight;

		if (x1 >= bbminInner.x) { // don't render segment if it is beyond the boundary
			if (this->renderFrameSegment(profile, side, x0, y0, x1, y1, bbmin.x, bbmin.y, bbmax.x, bbmax.y, segmentSpacing)) {
				hoveringFrameTime = elapsed;
			}
		}

		ImVec2 rollingFrameAveragePoint;
		rollingFrameAveragePoint.x = x1;
		rollingFrameAveragePoint.y = y0;

		rollingFrameAverageLine.emplace(rollingFrameAverageLine.begin(), rollingFrameAveragePoint);
	}

	for (int i = 1; i < rollingFrameAverageLine.size(); i++) {
		float& y0 = rollingFrameAverageLine[i - 1].y;
		float& y1 = rollingFrameAverageLine[i].y;
		y1 = y1 * frameTimeWeight + y0 * (1.0 - frameTimeWeight);
	}

	dl->AddPolyline(&rollingFrameAverageLine[0], rollingFrameAverageLine.size(), ImGui::ColorConvertFloat4ToU32(ImVec4(0.8F, 0.8F, 0.8F, 0.8F)), false, 2.0F);

	ImVec2 pos = rollingFrameAverageLine.back();
	double rollingAvgFrameTime = ((bbmax.y - pos.y) / (bbmax.y - bbmin.y)) * maxFrameTime;

	char str[15];

	sprintf_s(str, sizeof(str), "%.2f ms", maxFrameTime);
	renderFrameTimeOverlay(str, pos.x - padding, bbmin.y, bbmin.x, bbmin.y, bbmax.x, bbmax.y);

	sprintf_s(str, sizeof(str), "%.2f ms", rollingAvgFrameTime);
	renderFrameTimeOverlay(str, pos.x - padding, pos.y, bbmin.x, bbmin.y, bbmax.x, bbmax.y);

	if (hoveringFrameTime >= 0.0) {
		ImVec2 mouse = ImGui::GetMousePos();
		sprintf_s(str, sizeof(str), "%.2f ms", hoveringFrameTime);
		renderFrameTimeOverlay(str, mouse.x, mouse.y, bbmin.x, bbmin.y, bbmax.x, bbmax.y);
	}
	

	sprintf_s(str, sizeof(str), "%s TIME", side == ProfileSide::CPU ? "CPU" : "GPU");
	renderFrameTimeOverlay(str, bbmin.x + 3, bbmin.y + 3, bbmin.x, bbmin.y, bbmax.x, bbmax.y);

	ImGui::PopClipRect();
	ImGui::SetCursorScreenPos(ImVec2(bbmin.x, bbmax.y));
}

void Profiler::renderFrameTimeOverlay(const char* str, double x, double y, double xmin, double ymin, double xmax, double ymax) {
	ImVec2 size = ImGui::CalcTextSize(str);

	if (x > (xmax - xmin) / 2)
		x -= size.x;

	y -= (size.y / 2.0);
	y = min(max(y, ymin), ymax - size.y);
	ImGui::SetCursorScreenPos(ImVec2(x, y));
	ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + size.x, y + size.y), 0x88000000);
	ImGui::Text(str);
}

bool Profiler::renderFrameSegment(Profile* profile, ProfileSide side, double x0, double y0, double x1, double y1, double xmin, double ymin, double xmax, double ymax, double segmentSpacing) {
	ProfileLayer* layer = &m_layers[profile->layerIndex];

	bool hoveringSegment = false;
	double h = y1 - y0;
	double y = y1;

	if (!profile->children.empty() && layer->expanded) {

		double tp = getElapsedTime(profile, side);

		for (int i = 0; i < profile->children.size(); i++) {
			Profile* child = this->getProfile(profile->children[i]);

			double tc = getElapsedTime(child, side);

			double frac = tc / tp;
			double t1 = y;
			y -= h * frac;
			double t0 = y;

			if (this->renderFrameSegment(child, side, x0, t0, x1, t1, xmin, ymin, xmax, ymax, segmentSpacing)) {
				hoveringSegment = true;
			}
		}

		if ((int)y == (int)y0) // The children entirely filled the parent.
			return hoveringSegment;
	}

	// Either this is a leaf, or there was remaining space that the children did not fill.
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec4 colour;
	colour.x = layer->colour.r;
	colour.y = layer->colour.g;
	colour.z = layer->colour.b;
	colour.w = 1.0F;
	dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y), ImGui::ColorConvertFloat4ToU32(colour));

	if (ImGui::IsMouseHoveringRect(ImVec2(x0 - segmentSpacing, y0), ImVec2(x1, y), true)) {
		hoveringSegment = true;
		layer->hovering = true;
	}

	return hoveringSegment;
}

void Profiler::initializeProfileLayers(Profile* profile, std::string treePath) {
	typedef std::map<std::string, uint32_t> M;
	std::pair<M::iterator, bool> r = m_frameLayers.insert(M::value_type(treePath, m_layers.size()));
	M::iterator& it = r.first;
	bool& inserted = r.second;

	if (inserted) {
		// If this profile layer did not previously exist, initialize a random colour
		ProfileLayer layer;
		layer.colour.r = (float)rand() / RAND_MAX;
		layer.colour.g = (float)rand() / RAND_MAX;
		layer.colour.b = (float)rand() / RAND_MAX;
		m_layers.emplace_back(layer);
	}

	profile->layerIndex = it->second;
	m_layers[profile->layerIndex].hovering = false;

	for (int i = 0; i < profile->children.size(); i++) {
		// Recurse on child nodes
		Profile* child = this->getProfile(profile->children[i]);
		this->initializeProfileLayers(child, treePath + "." + child->name);
	}
}

bool Profiler::tryFreeGPUQuery(Profile* profile) {
	if (!Engine::instance()->hasGLContext())
		return false; // No GL context, not freed

	if (profile == NULL || profile->gpuQuery == NULL)
		return true; // Profile was already freed

	int flag = GL_FALSE;
	glGetQueryObjectiv(profile->gpuQuery->endQueryHandle, GL_QUERY_RESULT_AVAILABLE, &flag);

	if (flag == GL_TRUE) {
		// The query result is now available, retrieve the result and delete the query
		glGetQueryObjectui64v(profile->gpuQuery->startQueryHandle, GL_QUERY_RESULT, &profile->startTimeGPU);
		glGetQueryObjectui64v(profile->gpuQuery->endQueryHandle, GL_QUERY_RESULT, &profile->endTimeGPU);
		glDeleteQueries(1, &profile->gpuQuery->startQueryHandle);
		glDeleteQueries(1, &profile->gpuQuery->endQueryHandle);
		delete profile->gpuQuery;
		profile->gpuQuery = NULL;
		return true;
	}

	// The query result was not available, so this profile was not freed.
	return false;
}

uint32_t Profiler::createProfile(std::string name, uint32_t parentIndex) {
	// Initialize a new profile
	m_profiles.emplace_back(Profile());
	Profile* profile = &m_profiles.back();

	profile->name = name;
	profile->parent = parentIndex;

	profile->startTimeCPU = clock::now().time_since_epoch().count();

	if (Engine::instance()->hasGLContext()) {
		// Generate the GPU start and end queries, and record the start query timestamp
		profile->gpuQuery = new GPUQuery();
		glGenQueries(1, &profile->gpuQuery->startQueryHandle);
		glGenQueries(1, &profile->gpuQuery->endQueryHandle);
		glQueryCounter(profile->gpuQuery->startQueryHandle, GL_TIMESTAMP);
	}

	return m_profiles.size(); // Return the next profile index, all indices are offset by 1 so that 0 can represent NULL
}

Profile* Profiler::getProfile(uint32_t index) {
	return &m_profiles[index - 1];
}

Profile* Profiler::getFrame(uint32_t index) {
	int32_t backIndex = m_frames.size() - (index + 5); // Offset 5 frames to give GPU queries a chance to be freed - TODO: better solution
	if (backIndex < 0)
		return NULL;

	return this->getProfile(m_frames[backIndex]);
}

void Profiler::renderUI() {
	for (auto it = m_unfreedGPUProfiles.begin(); it != m_unfreedGPUProfiles.end();) {
		if (this->tryFreeGPUQuery(this->getProfile(*it))) {
			it = m_unfreedGPUProfiles.erase(it);
		} else {
			++it;
		}
	}
	
	Profile* frameProfile = this->getFrame(0); // most recent frame

	if (frameProfile == NULL)
		return;

	this->initializeProfileLayers(frameProfile, frameProfile->name);

	ImGui::Begin("Profiler");

	ImGui::BeginColumns("frameGraph-profileTree", 2);
	{
		int availableHeight = ImGui::GetContentRegionMaxAbs().y - ImGui::GetCurrentWindow()->DC.CursorPos.y;

		this->renderFrameGraph(-1, availableHeight / 2, ProfileSide::CPU);
		this->renderFrameGraph(-1, availableHeight / 2, ProfileSide::GPU);

		ImGui::NextColumn();

		ImGui::BeginChild("profileTree");
		ImGui::BeginColumns("profileTree", 6, ImGuiColumnsFlags_NoPreserveWidths);
		{
			if (m_frames.size() <= 5) {
				// Initialize column sizes only on first frame
				float x = ImGui::GetWindowContentRegionMax().x;
				ImGui::SetColumnOffset(6, x); x -= 60;
				ImGui::SetColumnOffset(5, x); x -= 80;
				ImGui::SetColumnOffset(4, x); x -= 100;
				ImGui::SetColumnOffset(3, x); x -= 80;
				ImGui::SetColumnOffset(2, x); x -= 100;
				ImGui::SetColumnOffset(1, x);
			}

			ImGui::Text("Profile Name");
			ImGui::NextColumn();
			ImGui::Text("CPU Time");
			ImGui::NextColumn();
			ImGui::Text("CPU %");
			ImGui::NextColumn();
			ImGui::Text("GPU Time");
			ImGui::NextColumn();
			ImGui::Text("GPU %");
			ImGui::NextColumn();
			ImGui::Text("Colour");
			ImGui::NextColumn();

			this->buildProfileTree(frameProfile, frameProfile);
		}
		ImGui::EndColumns();
		ImGui::EndChild();
	}
	ImGui::EndColumns();

	ImGui::End();
}

Profiler* Profiler::startProfiling(std::string name) {
	assert(!Profiler::isProfiling());

	s_currentProfiler = new Profiler(name);
	return s_currentProfiler;
}

Profiler* Profiler::currentProfiler() {
	return s_currentProfiler;
}

bool Profiler::isProfiling() {
	return s_currentProfiler != NULL;
}

void Profiler::stopProfiling() {
	assert(Profiler::isProfiling());
	delete s_currentProfiler;
	s_currentProfiler = NULL;
}

uint32_t Profiler::push(std::string name) {
	return currentProfiler()->pushProfile(name);
}

uint32_t Profiler::pop() {
	return currentProfiler()->popProfile();
}

Profile* Profiler::currentFrame() {
	return currentProfiler()->getCurrentFrame();
}