#include "Scene.h"
#include "core/renderer/ScreenRenderer.h"
#include "core/renderer/ShaderProgram.h"
#include "core/renderer/LayeredDepthBuffer.h"
#include "core/renderer/Lights.h"
#include "core/renderer/VoxelGenerator.h"
#include "core/renderer/MaterialManager.h"
#include "core/renderer/ShadowMapRenderer.h"
#include "core/renderer/geometry/GeometryBuffer.h"
#include "core/scene/BoundingVolumeHierarchy.h"
#include "core/scene/FirstPersonController.h"
#include "core/scene/SceneComponents.h"
#include "core/scene/Camera.h"
#include "core/profiler/Profiler.h"
#include "core/Engine.h"


SceneObject::SceneObject(Transformation transform) :
	m_currTransform(transform),
	m_prevTransform(transform),
	m_boundsNeedUpdate(true) {}

SceneObject::~SceneObject() {
	//info("Deleting scene mesh with %d components, %d children\n", m_components.size(), m_children.size());
	for (auto it = m_components.begin(); it != m_components.end(); it++) {
		delete it->second->component;
		delete it->second;
	}

	for (auto it = m_children.begin(); it != m_children.end(); it++) {
		delete it->second->object;
		delete it->second;
	}

	m_components.clear();
	m_children.clear();
}

void SceneObject::updateBoundingTree(TransformChain& parentTransform) {
	// update bounds as if this subtree was at the origin, then bounds are transformed when needed
}

void SceneObject::preRender(TransformChain& parentTransform, double dt, double partialTicks) {
	PROFILE_SCOPE("SceneObject::preRender()");
	dmat4 currTransformation = m_currTransform.getModelMatrix();
	TransformChain currTransform;
	currTransform.previous = &parentTransform;
	currTransform.sceneObject = this;
	currTransform.transformationMatrix = parentTransform.transformationMatrix * currTransformation;

	{
		PROFILE_SCOPE("SceneObject::preRender():RENDER_COMPONENTS");
			for (auto it = m_components.begin(); it != m_components.end(); it++) {
				if (it->second->enabled) {
					it->second->component->preRender(currTransform, dt, partialTicks);
				}
			}
	}

	{
		PROFILE_SCOPE("SceneObject::preRender():RENDER_CHILDREN");
		for (auto it = m_children.begin(); it != m_children.end(); it++) {
			if (it->second->enabled) {
				it->second->object->preRender(currTransform, dt, partialTicks);
			}
		}
	}
}

void SceneObject::render(TransformChain& parentTransform, double dt, double partialTicks) {
	PROFILE_SCOPE("SceneObject::render()");
	dmat4 currTransformation = m_currTransform.getModelMatrix();
	TransformChain currTransform;
	currTransform.previous = &parentTransform;
	currTransform.sceneObject = this;
	currTransform.transformationMatrix = parentTransform.transformationMatrix * currTransformation;

	{
		PROFILE_SCOPE("SceneObject::render():RENDER_COMPONENTS");
			for (auto it = m_components.begin(); it != m_components.end(); it++) {
				if (it->second->enabled) {
					it->second->component->render(currTransform, dt, partialTicks);
				}
			}
	}

	{
		PROFILE_SCOPE("SceneObject::render():RENDER_CHILDREN");
		for (auto it = m_children.begin(); it != m_children.end(); it++) {
			if (it->second->enabled) {
				it->second->object->render(currTransform, dt, partialTicks);
			}
		}
	}
}

void SceneObject::renderDirect(ShaderProgram* shaderProgram, TransformChain& parentTransform, double dt, double partialTicks) {
	PROFILE_SCOPE("SceneObject::renderDirect()");
	dmat4 currTransformation = m_currTransform.getModelMatrix();
	TransformChain currTransform;
	currTransform.previous = &parentTransform;
	currTransform.sceneObject = this;
	currTransform.transformationMatrix = parentTransform.transformationMatrix * currTransformation;

	{
		PROFILE_SCOPE("SceneObject::renderDirect():RENDER_COMPONENTS");
		for (auto it = m_components.begin(); it != m_components.end(); it++) {
			if (it->second->enabled) {
				it->second->component->renderDirect(shaderProgram, currTransform, dt, partialTicks);
			}
		}
	}

	{
		PROFILE_SCOPE("SceneObject::renderDirect():RENDER_CHILDREN");
		for (auto it = m_children.begin(); it != m_children.end(); it++) {
			if (it->second->enabled) {
				it->second->object->renderDirect(shaderProgram, currTransform, dt, partialTicks);
			}
		}
	}
}

void SceneObject::update(TransformChain& parentTransform, double dt) {
	TransformChain currTransform;
	currTransform.previous = &parentTransform;
	currTransform.sceneObject = this;
	currTransform.transformationMatrix = parentTransform.transformationMatrix * m_currTransform.getModelMatrix();

	for (auto it = m_components.begin(); it != m_components.end(); it++) {
		if (it->second->enabled) {
			it->second->component->update(currTransform, dt);
		}
	}

	for (auto it = m_children.begin(); it != m_children.end(); it++) {
		if (it->second->enabled) {
			it->second->object->update(currTransform, dt);
		}
	}
}

void SceneObject::allocateStaticSceneGeometry() {
	for (auto it = m_components.begin(); it != m_components.end(); it++) {
		if (it->second->enabled) {
			it->second->component->allocateStaticSceneGeometry();
		}
	}

	for (auto it = m_children.begin(); it != m_children.end(); it++) {
		if (it->second->enabled) {
			it->second->object->allocateStaticSceneGeometry();
		}
	}
}

void SceneObject::uploadStaticSceneGeometry(TransformChain& parentTransform) {
	TransformChain currTransform;
	currTransform.previous = &parentTransform;
	currTransform.sceneObject = this;
	currTransform.transformationMatrix = parentTransform.transformationMatrix * m_currTransform.getModelMatrix();

	for (auto it = m_components.begin(); it != m_components.end(); it++) {
		if (it->second->enabled) {
			it->second->component->uploadStaticSceneGeometry(currTransform);
		}
	}

	for (auto it = m_children.begin(); it != m_children.end(); it++) {
		if (it->second->enabled) {
			it->second->object->uploadStaticSceneGeometry(currTransform);
		}
	}
}

void SceneObject::markBoundsNeedUpdate() {
	m_boundsNeedUpdate = true;
}

bool SceneObject::hasChildren() {
	return !m_children.empty();
}

bool SceneObject::hasComponents() {
	return !m_components.empty();
}

bool SceneObject::hasChild(std::string name) {
	return m_children.count(name) != 0;
}

bool SceneObject::hasComponent(std::string name) {
	return m_components.count(name) != 0;
}

bool SceneObject::isChildEnabled(std::string name) {
	auto it = m_children.find(name);

	if (it == m_children.end()) {
		return false;
	}

	return it->second->enabled;
}

bool SceneObject::isComponentEnabled(std::string name) {
	auto it = m_components.find(name);

	if (it == m_components.end()) {
		return false;
	}

	return it->second->enabled;
}

void SceneObject::setChildEnabled(std::string name, bool enabled) {
	auto it = m_children.find(name);

	if (it != m_children.end()) {
		it->second->enabled = enabled;
	}
}

void SceneObject::setComponentEnabled(std::string name, bool enabled) {
	auto it = m_components.find(name);

	if (it != m_components.end()) {
		it->second->enabled = enabled;
	}
}

SceneObject* SceneObject::getChild(std::string name) {
	auto it = m_children.find(name);

	if (it == m_children.end()) {
		return NULL;
	}

	return it->second->object;
}

SceneComponent* SceneObject::getComponent(std::string name) {
	auto it = m_components.find(name);

	if (it == m_components.end()) {
		return NULL;
	}

	return it->second->component;
}

bool SceneObject::removeChild(std::string name) {
	auto it = m_children.find(name);

	if (it == m_children.end()) {
		return false;
	}

	delete it->second; // delete container mesh
	m_children.erase(it);
	return true;
}

bool SceneObject::removeComponent(std::string name) {
	auto it = m_components.find(name);

	if (it == m_components.end()) {
		return false;
	}
	it->second->component->onRemoved(this, name);

	delete it->second; // delete container mesh
	m_components.erase(it);
	return true;
}

SceneObject* SceneObject::addChild(std::string name, SceneObject* object, bool enabled) {
	if (this->hasChild(name)) {
		return NULL;
	}

	ChildContainer* container = new ChildContainer();
	container->object = object;
	container->enabled = enabled;
	m_children.insert(std::make_pair(name, container));
	return object;
}

bool SceneObject::addComponent(std::string name, SceneComponent* component, bool enabled) {
	if (this->hasComponent(name)) {
		return NULL;
	}

	ComponentContainer* container = new ComponentContainer();
	container->component = component;
	container->enabled = enabled;
	component->onAdded(this, name);

	m_components.insert(std::make_pair(name, container));
	return component;
}

Transformation& SceneObject::getTransformation() {
	return m_currTransform;
}

void SceneObject::setTransformation(Transformation& transform) {
	m_currTransform = transform;
}

RaycastResult* SceneObject::raycast(TransformChain& parentTransform, dvec3 rayOrigin, dvec3 rayDirection) {
	TransformChain currTransform;
	currTransform.previous = &parentTransform;
	currTransform.sceneObject = this;
	currTransform.transformationMatrix = parentTransform.transformationMatrix * m_currTransform.getModelMatrix();

	RaycastResult* closestResult = NULL;

	// TODO: if ray not intersect bounds, ignore child
	for (auto it = m_components.begin(); it != m_components.end(); it++) {
		if (it->second->enabled) {
			RaycastResult* result = it->second->component->raycast(currTransform, rayOrigin, rayDirection);
			if (result != NULL && (closestResult == NULL || result->distance < closestResult->distance)) {
				closestResult = result;
				closestResult->name = it->first;
			}
		}
	}

	for (auto it = m_children.begin(); it != m_children.end(); it++) {
		if (it->second->enabled) {
			RaycastResult* result = it->second->object->raycast(currTransform, rayOrigin, rayDirection);
			if (result != NULL && (closestResult == NULL || result->distance < closestResult->distance))
				closestResult = result;
		}
	}

	return closestResult;
}



SceneGraph::SceneGraph() {
	m_root = new SceneObject();
	m_camera = new Camera();
	m_voxelizer = new VoxelGenerator(1024, 0.025);
	m_staticGeometryBuffer = new GeometryBuffer();
	m_materialManager = new MaterialManager();
	m_controller = new FirstPersonController();
	m_globalEnvironmentMap = NULL;
	m_controllerEnabled = true;

	m_selection = NULL;
	m_selectionShader = new ShaderProgram();
	m_selectionShader->addShader(GL_VERTEX_SHADER, "res/shaders/raster/selection/vert.glsl");
	m_selectionShader->addShader(GL_FRAGMENT_SHADER, "res/shaders/raster/selection/frag.glsl");
	m_selectionShader->addAttribute(0, "vs_vertexPosition");
	m_selectionShader->addAttribute(1, "vs_vertexNormal");
	m_selectionShader->completeProgram();

	m_voxelizationFrequency = 1.0;// 1.0 / 500.0;
	m_lastVoxelization = 0;

	m_rebuildStaticSceneGeometry = true;

	m_defaultShader = new ShaderProgram();
	m_defaultShader->addShader(GL_VERTEX_SHADER, "shaders/raster/phong/vert.glsl");
	m_defaultShader->addShader(GL_GEOMETRY_SHADER, "shaders/raster/phong/geom.glsl");
	m_defaultShader->addShader(GL_FRAGMENT_SHADER, "shaders/raster/phong/frag.glsl");
	Mesh::addVertexInputs(m_defaultShader);
	ScreenRenderer::addFragmentOutputs(m_defaultShader);
	m_defaultShader->completeProgram();
}

SceneGraph::~SceneGraph() {
	delete m_root;
	delete m_camera;
	delete m_controller;
	delete m_voxelizer;
	delete m_staticGeometryBuffer;
	delete m_materialManager;
	delete m_defaultShader;
}

void SceneGraph::render(double dt, double partialTicks) {
	PROFILE_SCOPE("SceneGraph::render()");
	assert(m_root != NULL);
	assert(m_camera != NULL);
	assert(m_controller != NULL);

	if (m_rebuildStaticSceneGeometry) {
		m_rebuildStaticSceneGeometry = false;
		this->buildStaticSceneGeometry();
	}

	m_materialManager->render(dt, partialTicks);

	TransformChain transform;

	if (epsilonNotEqual(Engine::instance()->getWindowAspectRatio(), m_camera->getAspect(), 1e-6)) {
		m_camera->setAspect(Engine::instance()->getWindowAspectRatio());
	}

	m_camera->preRender(dt, partialTicks);

	if (m_controllerEnabled) {
		m_controller->update(dt, partialTicks);
		m_controller->applyCamera(m_camera);
	}

	m_activeLights.clear();

	m_root->preRender(transform, dt, partialTicks);
	m_camera->render(dt, partialTicks);

	m_transparentRenderPass = false;
	m_root->render(transform, dt, partialTicks);

	//ShaderProgram::use(m_defaultShader);
	//this->applyUniforms(m_defaultShader);
	//m_defaultShader->setUniform("modelMatrix", dmat4(1.0));
	//m_staticGeometryBuffer->getBVH()->getDebugMesh()->draw();
	//ShaderProgram::use(NULL);

	//m_transparentRenderPass = true;
	//m_root->render(transform, dt, partialTicks);
	//
	//m_transparentRenderPass = false;

	// uint64_t now = Engine::instance()->getCurrentTime();
	// if ((now - m_lastVoxelization) / 1000000000.0 >= m_voxelizationFrequency) {
	// 	m_lastVoxelization = now;
	// 
	// 	m_voxelizer->setGridCenter(m_camera->transform().getTranslation());
	// 	m_voxelizer->render(dt, partialTicks);
	// }
	// 
	// if (Engine::instance()->isDebugRenderVoxelGridEnabled()) {
	// 	m_voxelizer->renderDebug();
	// }
}

void SceneGraph::renderDirect(ShaderProgram* shaderProgram, double dt, double partialTicks) {
	assert(m_root != NULL);
	assert(m_camera != NULL);
	
	TransformChain transform;
	
	m_root->preRender(transform, dt, partialTicks);
	m_camera->render(dt, partialTicks);
	m_root->renderDirect(shaderProgram, transform, dt, partialTicks);
}

void SceneGraph::postRender(double dt, double partialTicks) {
	//if (m_selection != NULL && m_selectionShader != NULL) {
	//	ShaderProgram::use(m_selectionShader);
	//	this->applyUniforms(m_selectionShader);
	//	m_selectionShader->setUniform("modelMatrix", m_selection->transform.transformationMatrix);
	//
	//	Engine::screenRenderer()->getDepthTexture()->bind(0);
	//	m_selectionShader->setUniform("depthTexture", 0);
	//
	//	Engine::screenRenderer()->getNormalTexture()->bind(1);
	//	m_selectionShader->setUniform("normalTexture", 1);
	//
	//	m_selection->mesh->render(m_selection->transform, dt, partialTicks);
	//	ShaderProgram::use(NULL);
	//}
}

void SceneGraph::update(double dt) {
	assert(m_root != NULL);

	TransformChain transform;
	//transform.previous = NULL;
	//transform.sceneObject = NULL;
	//transform.transformationMatrix = dmat4(1.0); // identity matrix (no-op)
	m_root->update(transform, dt);
}

void SceneGraph::buildStaticSceneGeometry() {
	TransformChain transform;

	info("Building static scene geometry\n");
	uint64_t t0 = Engine::instance()->getCurrentTime();

	m_staticGeometryBuffer->reset();
	m_root->allocateStaticSceneGeometry();
	m_staticGeometryBuffer->initializeBuffers();
	m_root->uploadStaticSceneGeometry(transform);
	m_staticGeometryBuffer->buildBVH();
	//m_staticGeometryBuffer->getBVH()->buildDebugMesh();

	uint64_t t1 = Engine::instance()->getCurrentTime();
	info("Finished building static scene geometry - Took %.2f msec\n", (t1 - t0) / 1000000.0);
}

void SceneGraph::setSpawnLocation(dvec3 position, dvec3 look) {
	m_controller->transform().lookAt(position, position + look, dvec3(0.0, 1.0, 0.0));
}

LightProbe* SceneGraph::getClosestLightProbe(dvec3 point) {
	return m_globalEnvironmentMap;
}

SceneObject* SceneGraph::getRoot() {
	return m_root;
}

Camera* SceneGraph::getCamera() {
	return m_camera;
}

VoxelGenerator* SceneGraph::getVoxelizer() {
	return m_voxelizer;
}

GeometryBuffer* SceneGraph::getStaticGeometryBuffer() {
	return m_staticGeometryBuffer;
}

MaterialManager* SceneGraph::getMaterialManager() {
	return m_materialManager;
}

FirstPersonController* SceneGraph::getController() {
	return m_controller;
}

LightProbe* SceneGraph::getGlobalEnvironmentMap() {
	return m_globalEnvironmentMap;
}

RaycastResult* SceneGraph::raycast(dvec3 rayOrigin, dvec3 rayDirection) {
	TransformChain transform;
	return m_root->raycast(transform, rayOrigin, rayDirection);
}

RaycastResult* SceneGraph::getSelectedMesh() {
	return m_selection;
}

void SceneGraph::setSelectedMesh(RaycastResult* mesh) {
	m_selection = mesh;
}

bool SceneGraph::isControllerEnabled() {
	return m_controllerEnabled;
}

bool SceneGraph::isSimpleRenderEnabled() {
	return m_simpleRenderEnabled;
}

bool SceneGraph::isTransparentRenderPass() {
	return m_transparentRenderPass;
}

void SceneGraph::setCamera(Camera* camera) {
	if (camera != NULL) {
		m_camera = camera;
	}
}

void SceneGraph::setController(FirstPersonController* controller) {
	if (controller != NULL) {
		m_controller = controller;
	}
}

void SceneGraph::setGlobalEnvironmentMap(LightProbe* envronmentMap) {
	m_globalEnvironmentMap = envronmentMap;
}

void SceneGraph::setControllerEnabled(bool controllerEnabled) {
	m_controllerEnabled = controllerEnabled;
}

void SceneGraph::setSimpleRenderEnabled(bool simpleRender) {
	m_simpleRenderEnabled = simpleRender;
}

#define MAX_LIGHTS 16

void SceneGraph::applyUniforms(ShaderProgram* shaderProgram) {
	assert(shaderProgram != NULL);
	assert(m_camera != NULL);

	if (m_transparentRenderPass) {
		glDepthMask(GL_FALSE);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	} else {
		glDepthMask(GL_TRUE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	m_camera->applyUniforms(shaderProgram);
	m_voxelizer->applyUniforms(shaderProgram);
	m_materialManager->applyUniforms(shaderProgram);
	m_staticGeometryBuffer->applyUniforms(shaderProgram);
	Engine::screenRenderer()->applyUniforms(shaderProgram);
	// Engine::screenRenderer()->getLayeredDepthBuffer()->applyUniforms(shaderProgram);
	shaderProgram->setUniform("screenSize", Engine::instance()->getWindowSize().x, Engine::instance()->getWindowSize().y);
	shaderProgram->setUniform("aspectRatio", (float) Engine::instance()->getWindowAspectRatio());
	shaderProgram->setUniform("lightingEnabled", Engine::instance()->isDebugRenderLightingEnabled());
	shaderProgram->setUniform("imageBasedLightingEnabled", Engine::instance()->isImageBasedLightingEnabled());
	shaderProgram->setUniform("transparentRenderPass", m_transparentRenderPass);
	shaderProgram->setUniform("materialCount", (int) m_materialManager->getMaterialCount());

	float r0 = rand() / float(RAND_MAX);
	float r1 = rand() / float(RAND_MAX);
	float r2 = rand() / float(RAND_MAX);
	float r3 = rand() / float(RAND_MAX);
	shaderProgram->setUniform("randomInterval", fvec4(r0, r1, r2, r3));

	int32_t pointLightIndex = 0;
	int32_t directionLightIndex = 0;
	int32_t spotLightIndex = 0;

	// sort lights with a position based on distance ?

	for (int i = 0; i < m_activeLights.size(); i++) {
		Light* light = m_activeLights[i];
		if (light == NULL) {
			continue;
		} else if (light->getLightType() == POINT_LIGHT && pointLightIndex < MAX_LIGHTS) {
			light->applyUniforms("pointLights[" + std::to_string(pointLightIndex++) + "]", shaderProgram);
		} else if (light->getLightType() == DIRECTION_LIGHT && directionLightIndex < MAX_LIGHTS) {
			light->applyUniforms("directionLights[" + std::to_string(directionLightIndex++) + "]", shaderProgram);
		} else if (light->getLightType() == SPOT_LIGHT && spotLightIndex < MAX_LIGHTS) {
			light->applyUniforms("spotLights[" + std::to_string(spotLightIndex++) + "]", shaderProgram);
		}
	}

	shaderProgram->setUniform("pointLightCount", pointLightIndex);
	shaderProgram->setUniform("directionLightCount", directionLightIndex);
	shaderProgram->setUniform("spotLightCount", spotLightIndex);

	// Upload lighting/shadow map etc
}

void SceneGraph::addActiveLight(Light* light) {
	m_activeLights.push_back(light);
}