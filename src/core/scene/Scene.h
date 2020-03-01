#pragma once

#include "core/pch.h"
#include "core/scene/Transformation.h"
#include "core/scene/Bounding.h"
#include "core/renderer/geometry/Mesh.h"

class Transformation;
class AxisAlignedBB;
class ShaderProgram;
class Camera;
class LightProbe;
class FirstPersonController;

class SceneComponent;
class RenderComponent;
class SceneObject;
class SceneGraph;
class Light;
class ShadowMap;
class VoxelGenerator;
class GeometryBuffer;
class BoundingVolumeHierarchy;
class MeshComponent;
class MaterialManager;

/**
 * Linked list of the built up chain of transformations while traversing the scene tree.
 */
struct TransformChain {
	TransformChain* previous = NULL;
	SceneObject* sceneObject = NULL;
	dmat4 transformationMatrix = dmat4(1.0); // identity matrix (no-op)
};

struct RaycastResult {
	double distance = INFINITY;
	MeshComponent* mesh = NULL;
	TransformChain transform;
	std::string name = "";
};

class SceneComponent {
public:
	SceneComponent() {}

	virtual ~SceneComponent() = default;

	virtual void preRender(TransformChain& parentTransform, double dt, double partialTicks) {};

	virtual void render(TransformChain& parentTransform, double dt, double partialTicks) {};

	virtual void renderDirect(ShaderProgram* shaderProgram, TransformChain& parentTransform, double dt, double partialTicks) {};

	virtual void update(TransformChain& parentTransform, double dt) {};

	virtual void allocateStaticSceneGeometry() {};

	virtual void uploadStaticSceneGeometry(TransformChain& parentTransform) {};

	virtual void onAdded(SceneObject* object, std::string name) {};

	virtual void onRemoved(SceneObject* object, std::string name) {};
};

class SceneObject {
	friend class SceneGraph;
public:
	SceneObject(Transformation transform = Transformation());

	~SceneObject();

	void updateBoundingTree();

	void preRender(TransformChain& parentTransform, double dt, double partialTicks);

	void render(TransformChain& parentTransform, double dt, double partialTicks);

	void renderDirect(ShaderProgram* shaderProgram, TransformChain& parentTransform, double dt, double partialTicks);

	void update(TransformChain& parentTransform, double dt);

	void allocateStaticSceneGeometry();

	void uploadStaticSceneGeometry(TransformChain& parentTransform);

	void markBoundsNeedUpdate();

	bool hasChildren();

	bool hasComponents();

	bool hasChild(std::string name);

	bool hasComponent(std::string name);

	bool isChildEnabled(std::string name);

	bool isComponentEnabled(std::string name);

	void setChildEnabled(std::string name, bool enabled);

	void setComponentEnabled(std::string name, bool enabled);

	SceneObject* getChild(std::string name);

	SceneComponent* getComponent(std::string name);

	template <typename T>
	inline T* getComponent(std::string name);

	bool removeChild(std::string name);

	bool removeComponent(std::string name);

	SceneObject* addChild(std::string name, SceneObject* object, bool enabled = true);

	bool addComponent(std::string name, SceneComponent* component, bool enabled = true);

	Transformation& getTransformation();

	void setTransformation(Transformation& transform);

private:
	bool updateBounds();

	RaycastResult* raycast(TransformChain& parentTransform, dvec3 rayOrigin, dvec3 rayDirection);

	struct ChildContainer {
		SceneObject* object;
		bool enabled;
	};

	struct ComponentContainer {
		SceneComponent* component;
		bool enabled;
	};

	bool m_boundsNeedUpdate;
	AxisAlignedBB m_localBounds; // Bounds of this mesh with no transformation applied. This only changes when the shape of the mesh changes.
	AxisAlignedBB m_transformedBounds; // Bounds of the mesh updated whenever the objects transformation changes.
	Transformation m_currTransform; // The current transformation of this mesh.
	Transformation m_prevTransform; // The transformation of this mesh in the previous frame.
	std::map<std::string, ChildContainer*> m_children;
	std::map<std::string, ComponentContainer*> m_components;
};

class SceneGraph {
public:
	SceneGraph();

	~SceneGraph();

	void render(double dt, double partialTicks);

	void renderDirect(ShaderProgram* shaderProgram, double dt, double partialTicks);

	void postRender(double dt, double partialTicks);

	void update(double dt);

	void buildStaticSceneGeometry();

	void setSpawnLocation(dvec3 position, dvec3 look = dvec3(0.0, 0.0, -1.0));

	LightProbe* getClosestLightProbe(dvec3 point);

	SceneObject* getRoot();

	Camera* getCamera();

	VoxelGenerator* getVoxelizer();

	GeometryBuffer* getStaticGeometryBuffer();

	MaterialManager* getMaterialManager();

	FirstPersonController* getController();

	LightProbe* getGlobalEnvironmentMap();

	RaycastResult* raycast(dvec3 rayOrigin, dvec3 rayDirection);

	RaycastResult* getSelectedMesh();

	void setSelectedMesh(RaycastResult* mesh);

	bool isControllerEnabled();

	bool isSimpleRenderEnabled();

	bool isTransparentRenderPass();

	void setCamera(Camera* camera);

	void setController(FirstPersonController* controller);

	void setGlobalEnvironmentMap(LightProbe* envronmentMap);

	void setControllerEnabled(bool controllerEnabled);

	void setSimpleRenderEnabled(bool simpleRender);

	void applyUniforms(ShaderProgram* shaderProgram);

	void addActiveLight(Light* light);

private:
	SceneObject* m_root;
	Camera* m_camera;
	VoxelGenerator* m_voxelizer;
	GeometryBuffer* m_staticGeometryBuffer;
	MaterialManager* m_materialManager;
	FirstPersonController* m_controller;
	LightProbe* m_globalEnvironmentMap;

	ShaderProgram* m_defaultShader;

	RaycastResult* m_selection;
	ShaderProgram* m_selectionShader;

	bool m_controllerEnabled;
	bool m_simpleRenderEnabled;
	bool m_transparentRenderPass;

	bool m_rebuildStaticSceneGeometry;

	std::vector<Light*> m_activeLights;

	double m_voxelizationFrequency;
	uint64_t m_lastVoxelization;
};

template <typename T>
inline T* SceneObject::getComponent(std::string name) {
	return dynamic_cast<T*>(this->getComponent(name));
}