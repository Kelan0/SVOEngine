#pragma once

#include "core/pch.h"
#include "core/scene/Scene.h"
#include "core/renderer/Material.h"

//class SceneComponent;
class ShaderProgram;
class Light;
class ShadowMap;
class AxisAlignedBB;

class RenderComponent;
class MultiRenderComponent;
class LightComponent;
class MeshComponent;
class BoundingVolumeHierarchy;
struct GeometryRegion;

struct UnloadedMesh {
	const char* modelFilePath;
	uint32_t reservedVertices = 0;
	uint32_t reservedTriangles = 0;

	UnloadedMesh(const char* objFilePath) : 
		modelFilePath(objFilePath) {
	}

	UnloadedMesh(const char* objFilePath, uint32_t reservedVertices, uint32_t reservedTriangles) :
		modelFilePath(objFilePath), 
		reservedVertices(reservedVertices),
		reservedTriangles(reservedTriangles) {
	}
};

// Material applied to all polygons in the mesh specified between the indices in each section
struct MeshMaterialSection {
	std::vector<std::pair<uint32_t, uint32_t>> sections;
	Material* material;
	std::string name;
};

struct UnloadedMeshMaterialSection {
	std::vector<std::pair<uint32_t, uint32_t>> sections;
	MaterialConfiguration material;
	std::string name;
};



class RenderComponent : public SceneComponent {
public:
	RenderComponent(Mesh* mesh, ShaderProgram* shaderProgram);

	RenderComponent(UnloadedMesh mesh, ShaderProgram* shaderProgram);

	RenderComponent(Mesh* mesh, MaterialConfiguration material, ShaderProgram* shaderProgram);

	RenderComponent(UnloadedMesh mesh, MaterialConfiguration material, ShaderProgram* shaderProgram);

	RenderComponent(Mesh* mesh, Material* material, ShaderProgram* shaderProgram);

	RenderComponent(UnloadedMesh mesh, Material* material, ShaderProgram* shaderProgram);

	~RenderComponent();

	RenderComponent* transferMesh();

	RenderComponent* transferMaterial();

	void render(TransformChain& parentTransform, double dt, double partialTicks) override;

	void renderDirect(ShaderProgram* shaderProgram, TransformChain& parentTransform, double dt, double partialTicks) override;

	void allocateStaticSceneGeometry() override;

	void uploadStaticSceneGeometry(TransformChain& parentTransform) override;

	void onAdded(SceneObject* object, std::string name) override;

	void onRemoved(SceneObject* object, std::string name) override;

	Mesh* getMesh();

	Material* getMaterial();

	ShaderProgram* getShaderProgram();

private:
	void loadMesh(UnloadedMesh mesh);

	void loadMaterial(MaterialConfiguration material);

	Mesh* m_mesh;
	GeometryRegion* m_geometryRegion;
	Material* m_material;
	ShaderProgram* m_shaderProgram;

	bool m_ownsMesh;
	bool m_ownsMaterial;
	double m_renderTime;
};



class MultiRenderComponent : public SceneComponent {
public:
	MultiRenderComponent(Mesh* mesh, ShaderProgram* shaderProgram);
	
	MultiRenderComponent(UnloadedMesh mesh, ShaderProgram* shaderProgram);

	MultiRenderComponent(Mesh* mesh, std::vector<UnloadedMeshMaterialSection> material, ShaderProgram* shaderProgram);

	MultiRenderComponent(UnloadedMesh mesh, std::vector<UnloadedMeshMaterialSection> material, ShaderProgram* shaderProgram);

	MultiRenderComponent(Mesh* mesh, std::vector<MeshMaterialSection> material, ShaderProgram* shaderProgram);

	MultiRenderComponent(UnloadedMesh mesh, std::vector<MeshMaterialSection> material, ShaderProgram* shaderProgram);

	~MultiRenderComponent();

	MultiRenderComponent* transferMesh();

	void render(TransformChain& parentTransform, double dt, double partialTicks) override;

	void renderDirect(ShaderProgram* shaderProgram, TransformChain& parentTransform, double dt, double partialTicks) override;

	void allocateStaticSceneGeometry() override;

	void uploadStaticSceneGeometry(TransformChain& parentTransform) override;

	virtual void onAdded(SceneObject* object, std::string name) override;

	virtual void onRemoved(SceneObject* object, std::string name) override;

private:
	struct OwnableMeshMaterialSection {
		MeshMaterialSection materialSection;
		bool ownsMaterial;
	};

	void loadMesh(UnloadedMesh mesh);

	void loadMesh(Mesh* mesh);

	void loadMaterials(std::vector<UnloadedMeshMaterialSection> materials);

	void loadMaterials(std::vector<MeshMaterialSection> materials);

	Mesh* m_mesh;
	GeometryRegion* m_geometryRegion;
	std::vector<OwnableMeshMaterialSection> m_materials;
	std::map<std::string, std::vector<std::pair<uint32_t, uint32_t>>> m_objectSections;
	ShaderProgram* m_shaderProgram;

	bool m_ownsMesh;
	double m_renderTime;
};



class LightComponent : public SceneComponent {
public:
	LightComponent(Light* light);

	~LightComponent();

	void preRender(TransformChain& parentTransform, double dt, double partialTicks) override;

	Light* getLight();

private:
	Light* m_light;
};



class MeshComponent : public SceneComponent {
public:
	MeshComponent(Mesh* mesh, uint32_t firstTriangle = 0, uint32_t lastTriangle = -1);

	MeshComponent(Mesh* mesh, std::pair<uint32_t, uint32_t> triangleRange);

	MeshComponent(Mesh* mesh, std::vector<std::pair<uint32_t, uint32_t>> triangleSections);

	MeshComponent(UnloadedMesh mesh, uint32_t firstTriangle = 0, uint32_t lastTriangle = -1);

	MeshComponent(UnloadedMesh mesh, std::pair<uint32_t, uint32_t> triangleRange);

	MeshComponent(UnloadedMesh mesh, std::vector<std::pair<uint32_t, uint32_t>> triangleSections);

	void render(TransformChain& parentTransform, double dt, double partialTicks) override;

	MeshComponent* transferMesh();

	Mesh* getMesh() const;

	AxisAlignedBB getEnclosingBounds() const;

	std::vector<std::pair<uint32_t, uint32_t>> getSections() const;

	std::pair<uint32_t, uint32_t> getSection(uint32_t index) const;

	uint32_t getSectionCount() const;

	uint32_t getTriangleCount() const;

	uint32_t getVertexCount() const;

	RaycastResult* raycast(TransformChain& parentTransform, dvec3 rayOrigin, dvec3 rayDirection);

private:
	void loadMesh(Mesh* mesh);

	void loadMesh(UnloadedMesh mesh);

	void loadSections(std::vector<std::pair<uint32_t, uint32_t>> triangleSections);

	void calculateBounds();

	bool m_ownsMesh;
	Mesh* m_mesh;
	AxisAlignedBB m_enclosingBounds;
	std::vector<AxisAlignedBB> m_sectionBounds; // List of bounding boxes for each triangle section
	std::vector<std::pair<uint32_t, uint32_t>> m_sections; // List of sections of triangle indices
};