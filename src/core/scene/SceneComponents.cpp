#include "core/scene/SceneComponents.h"
#include "core/scene/Bounding.h"
#include "core/scene/BoundingVolumeHierarchy.h"
#include "core/renderer/geometry/GeometryBuffer.h"
#include "core/renderer/geometry/MeshLoader.h"
#include "core/renderer/geometry/Mesh.h"
#include "core/renderer/CubeMap.h"
#include "core/renderer/EnvironmentMap.h"
#include "core/renderer/Lights.h"
#include "core/renderer/Material.h"
#include "core/renderer/MaterialManager.h"
#include "core/renderer/ShaderProgram.h"
#include "core/renderer/ShadowMapRenderer.h"
#include "core/renderer/Texture.h"
#include "core/util/FileUtils.h"
#include "core/Engine.h"
#include "core/InputHandler.h"
//#include "core/scene/Scene.h"

RaycastResult* raycastTransformedMesh(Mesh* mesh, TransformChain& parentTransform, dvec3 rayOrigin, dvec3 rayDirection) {
	RaycastResult* result = NULL;

	// Transform ray from world-space to model-space
	dmat4 modelToWorld = parentTransform.transformationMatrix;
	dmat4 worldToModel = inverse(modelToWorld);
	dvec3 modelRayOrigin = dvec3(worldToModel * dvec4(rayOrigin, 1.0));
	dvec3 modelRayDirection = dvec3(worldToModel * dvec4(rayDirection, 0.0));

	double closestHitDistance = INFINITY;
	dvec3 closestHitBarycentric;
	Mesh::index closestHitTriangleIndex;
	if (mesh->getRayIntersection(modelRayOrigin, modelRayDirection, closestHitDistance, closestHitBarycentric, closestHitTriangleIndex)) {
		//dvec3 hitPoint = dvec3(modelToWorld * dvec4(modelRayOrigin + modelRayDirection * closestHitDistance, 1.0));

		result = new RaycastResult();
		result->distance = closestHitDistance;// distance(rayOrigin, hitPoint);
		result->barycentric = closestHitBarycentric;
		result->triangleIndex = closestHitTriangleIndex;
		result->mesh = mesh;
		result->transform = parentTransform;
	}

	return result;
}

RenderComponent::RenderComponent(Mesh* mesh, ShaderProgram* shaderProgram) {
	m_mesh = mesh;
	m_geometryRegion = NULL;
	m_material = NULL;
	m_shaderProgram = shaderProgram;
	m_ownsMesh = false;
	m_ownsMaterial = false;
}

RenderComponent::RenderComponent(UnloadedMesh mesh, ShaderProgram* shaderProgram) {
	m_mesh = NULL;
	m_geometryRegion = NULL;
	m_material = NULL;
	m_shaderProgram = shaderProgram;
	m_ownsMesh = true;
	m_ownsMaterial = false;
	this->loadMesh(mesh);
}

RenderComponent::RenderComponent(Mesh* mesh, MaterialConfiguration material, ShaderProgram* shaderProgram) {
	m_mesh = mesh;
	m_geometryRegion = NULL;
	m_material = NULL;
	m_shaderProgram = shaderProgram;
	m_ownsMesh = false;
	this->loadMaterial(material);
}

RenderComponent::RenderComponent(UnloadedMesh mesh, MaterialConfiguration material, ShaderProgram* shaderProgram) {
	m_mesh = NULL;
	m_geometryRegion = NULL;
	m_material = NULL;
	m_shaderProgram = shaderProgram;
	this->loadMesh(mesh);
	this->loadMaterial(material);
}

RenderComponent::RenderComponent(Mesh* mesh, Material* material, ShaderProgram* shaderProgram) {
	m_mesh = mesh;
	m_geometryRegion = NULL;
	m_material = material;
	m_shaderProgram = shaderProgram;
	m_ownsMesh = false;
	m_ownsMaterial = false;
}

RenderComponent::RenderComponent(UnloadedMesh mesh, Material* material, ShaderProgram* shaderProgram) {
	m_mesh = NULL;
	m_geometryRegion = NULL;
	m_material = material;
	m_shaderProgram = shaderProgram;
	m_ownsMaterial = false;
	this->loadMesh(mesh);
}

RenderComponent::~RenderComponent() {
	//info("Deleting render component\n");
	if (m_ownsMesh) delete m_mesh;
	if (m_ownsMaterial) delete m_material;
	m_mesh = NULL;
	m_geometryRegion = NULL;
	m_material = NULL;
	m_shaderProgram = NULL;
}

RenderComponent* RenderComponent::transferMesh() {
	m_ownsMesh = true;
	return this;
}

RenderComponent* RenderComponent::transferMaterial() {
	m_ownsMaterial = true;
	return this;
}

void RenderComponent::render(TransformChain& parentTransform, double dt, double partialTicks) {
	this->renderDirect(m_shaderProgram, parentTransform, dt, partialTicks);
}

void RenderComponent::renderDirect(ShaderProgram* shaderProgram, TransformChain& parentTransform, double dt, double partialTicks) {
	if (shaderProgram != NULL && m_geometryRegion != NULL) {
		ShaderProgram::use(shaderProgram);
		Engine::instance()->getScene()->applyUniforms(shaderProgram);
		shaderProgram->setUniform("modelMatrix", dmat4(1.0));// parentTransform.transformationMatrix);

		bool simpleRender = Engine::scene()->isSimpleRenderEnabled();

		LightProbe* lightProbe = NULL;
		if (!simpleRender && Engine::instance()->isDebugRenderLightingEnabled()) {
			lightProbe = Engine::scene()->getClosestLightProbe(parentTransform.sceneObject->getTransformation().getTranslation());
		}

		if (lightProbe != NULL) {
			lightProbe->bindDiffuseIrradianceMap(20);
			lightProbe->bindSpecularReflectionMap(21);
			shaderProgram->setUniform("hasLightProbe", true);
			shaderProgram->setUniform("diffuseIrradianceTexture", 20);
			shaderProgram->setUniform("specularReflectionTexture", 21);
		} else {
			shaderProgram->setUniform("hasLightProbe", false);
		}

		//if (m_material == NULL) {
		//	shaderProgram->setUniform("hasMaterial", false);
		//} else {
		//	m_material->bind(shaderProgram, "material");
		//	shaderProgram->setUniform("hasMaterial", true);
		//}

		Engine::scene()->getMaterialManager()->bindMaterialBuffer(3);

		Engine::scene()->getStaticGeometryBuffer()->draw(*m_geometryRegion);

		//m_mesh->draw();

		//if (m_material != NULL) {
		//	m_material->unbind();
		//}

		// ShaderProgram::use(NULL);



		//m_bvh->getDebugMesh()->draw();
	}
}

void RenderComponent::allocateStaticSceneGeometry() {
	Engine::scene()->getStaticGeometryBuffer()->allocate(m_mesh->getVertexCount(), m_mesh->getTriangleCount());
}

void RenderComponent::uploadStaticSceneGeometry(TransformChain& parentTransform) {
	m_geometryRegion = new GeometryRegion();
	Engine::scene()->getStaticGeometryBuffer()->upload(m_mesh->getVertices(), m_mesh->getTriangles(), m_geometryRegion, parentTransform.transformationMatrix);
}

void RenderComponent::onAdded(SceneObject* object, std::string name) {
	MeshComponent* meshComponent = new MeshComponent(m_mesh);
	object->addComponent(name + "_mesh", meshComponent);
}

void RenderComponent::onRemoved(SceneObject* object, std::string name) {
	object->removeComponent(name + "_mesh"); // find better way of doing this...
}

RaycastResult* RenderComponent::raycast(TransformChain& parentTransform, dvec3 rayOrigin, dvec3 rayDirection) {
	return raycastTransformedMesh(m_mesh, parentTransform, rayOrigin, rayDirection);
}

Mesh* RenderComponent::getMesh() {
	return m_mesh;
}

Material* RenderComponent::getMaterial() {
	return m_material;
}

ShaderProgram* RenderComponent::getShaderProgram() {
	return m_shaderProgram;
}

void RenderComponent::loadMesh(UnloadedMesh mesh) {
	m_ownsMesh = true;
	MeshLoader::OBJ* obj = MeshLoader::OBJ::load(std::string(mesh.modelFilePath));

	obj->initializeMaterialIndices();
	m_mesh = obj->createMesh(false, false);

	//MaterialManager* materialManager = Engine::scene()->getMaterialManager();
	
	//m_triangleOffset = obj->upload();
	//m_triangleCount = obj->getTriangleCount();

	delete obj;
}

void RenderComponent::loadMaterial(MaterialConfiguration material) {
	m_ownsMaterial = true;
	m_material = new Material(material);
}




MultiRenderComponent::MultiRenderComponent(Mesh* mesh, ShaderProgram* shaderProgram) {
	m_shaderProgram = shaderProgram;
	this->loadMesh(mesh);
}

MultiRenderComponent::MultiRenderComponent(UnloadedMesh mesh, ShaderProgram* shaderProgram) {
	m_shaderProgram = shaderProgram;
	this->loadMesh(mesh);
}

MultiRenderComponent::MultiRenderComponent(Mesh* mesh, std::vector<UnloadedMeshMaterialSection> materials, ShaderProgram* shaderProgram) {
	m_shaderProgram = shaderProgram;
	this->loadMesh(mesh);
	this->loadMaterials(materials);
}

MultiRenderComponent::MultiRenderComponent(UnloadedMesh mesh, std::vector<UnloadedMeshMaterialSection> materials, ShaderProgram* shaderProgram) {
	m_shaderProgram = shaderProgram;
	this->loadMesh(mesh);
	this->loadMaterials(materials);
}

MultiRenderComponent::MultiRenderComponent(Mesh* mesh, std::vector<MeshMaterialSection> materials, ShaderProgram* shaderProgram) {
	m_shaderProgram = shaderProgram;
	this->loadMesh(mesh);
	this->loadMaterials(materials);
}

MultiRenderComponent::MultiRenderComponent(UnloadedMesh mesh, std::vector<MeshMaterialSection> materials, ShaderProgram* shaderProgram) {
	m_shaderProgram = shaderProgram;
	this->loadMesh(mesh);
	this->loadMaterials(materials);
}

MultiRenderComponent::~MultiRenderComponent() {
	if (m_ownsMesh) delete m_mesh;
	for (int i = 0; i < m_materials.size(); i++) if (m_materials[i].ownsMaterial) delete m_materials[i].materialSection.material;
	m_mesh = NULL;
	m_materials.clear();
	m_shaderProgram = NULL;
}
MultiRenderComponent* MultiRenderComponent::transferMesh() {
	m_ownsMesh = true;
	return this;
}

void MultiRenderComponent::render(TransformChain& parentTransform, double dt, double partialTicks) {
	this->renderDirect(m_shaderProgram, parentTransform, dt, partialTicks);
}

void MultiRenderComponent::renderDirect(ShaderProgram* shaderProgram, TransformChain& parentTransform, double dt, double partialTicks) {
	if (shaderProgram != NULL && m_geometryRegion != NULL) {
		ShaderProgram::use(shaderProgram);
		Engine::instance()->getScene()->applyUniforms(shaderProgram);
		shaderProgram->setUniform("modelMatrix", glm::scale(dmat4(1.0), dvec3(0.5)));// parentTransform.transformationMatrix);

		bool simpleRender = Engine::scene()->isSimpleRenderEnabled();

		if (Engine::instance()->isDebugRenderLightingEnabled()) {
			LightProbe* lightProbe = Engine::scene()->getClosestLightProbe(parentTransform.sceneObject->getTransformation().getTranslation());

			if (lightProbe != NULL) {
				lightProbe->bindDiffuseIrradianceMap(20);
				lightProbe->bindSpecularReflectionMap(21);
				shaderProgram->setUniform("hasLightProbe", true);
				shaderProgram->setUniform("diffuseIrradianceTexture", 20);
				shaderProgram-> setUniform("specularReflectionTexture", 21);
			} else {
				shaderProgram->setUniform("hasLightProbe", false);
			}
		}

		if (simpleRender || m_materials.empty()) {
			shaderProgram->setUniform("hasMaterial", false);
			shaderProgram->setUniform("hasLightProbe", false);
			Engine::scene()->getStaticGeometryBuffer()->draw(*m_geometryRegion);
		} else {
			for (int i = 0; i < m_materials.size(); i++) {
				Material* material = m_materials[i].materialSection.material;
				std::vector<std::pair<uint32_t, uint32_t>> sections = m_materials[i].materialSection.sections;

				glEnable(GL_CULL_FACE);

				if (material == NULL) {
					if (Engine::scene()->isTransparentRenderPass())
						continue; // No material, therefor no transparent pixels, don't render on transparent pass

					shaderProgram->setUniform("hasMaterial", false);
				} else {
					if (material->isTransparent() && !Engine::scene()->isTransparentRenderPass()) {
						continue; // Transparent material on opaque render pass - skip this
					}

					if (!material->isTransparent() && Engine::scene()->isTransparentRenderPass()) {
						continue; // Opaque material on transparent render pass - skip this
					}

					material->bind(shaderProgram, "material");
					shaderProgram->setUniform("hasMaterial", true);
				}

				// TODO one single indirect draw call, not many individual draws
				for (int j = 0; j < sections.size(); j++) {
					uint32_t startTriangleIndex = sections[j].first;
					uint32_t endTriangleIndex = sections[j].second;
					GeometryRegion region;
					region.vertexOffset = m_geometryRegion->vertexOffset;
					region.triangleOffset = m_geometryRegion->triangleOffset + startTriangleIndex;
					region.triangleCount = endTriangleIndex - startTriangleIndex;
					Engine::scene()->getStaticGeometryBuffer()->draw(region);
					//m_mesh->draw(startTriangleIndex * 3, (endTriangleIndex - startTriangleIndex) * 3);
				}

				//if (material != NULL) {
				//	material->unbind();
				//}
			}
		}

		glEnable(GL_CULL_FACE);

		// ShaderProgram::use(NULL);
	}
}
void MultiRenderComponent::allocateStaticSceneGeometry() {
	Engine::scene()->getStaticGeometryBuffer()->allocate(m_mesh->getVertexCount(), m_mesh->getTriangleCount());
}

void MultiRenderComponent::uploadStaticSceneGeometry(TransformChain& parentTransform) {
	m_geometryRegion = new GeometryRegion();
	Engine::scene()->getStaticGeometryBuffer()->upload(m_mesh->getVertices(), m_mesh->getTriangles(), m_geometryRegion, parentTransform.transformationMatrix);
}

void MultiRenderComponent::onAdded(SceneObject* object, std::string name) {
	for (auto it = m_objectSections.begin(); it != m_objectSections.end(); it++) {
		std::string objectName = it->first;
		std::vector<std::pair<uint32_t, uint32_t>> sections = it->second;

		MeshComponent* meshComponent = new MeshComponent(m_mesh, sections);
		object->addComponent(name + ":" + objectName + "_mesh", meshComponent);
	}
}

void MultiRenderComponent::onRemoved(SceneObject* object, std::string name) {
	for (auto it = m_objectSections.begin(); it != m_objectSections.end(); it++) {
		std::string objectName = it->first;
		object->removeComponent(name + ":" + objectName);
	}
}

void MultiRenderComponent::loadMesh(UnloadedMesh mesh) {
	if (m_mesh != NULL && m_ownsMesh)
		delete m_mesh;

	MeshLoader::OBJ* obj = MeshLoader::OBJ::load(std::string(mesh.modelFilePath));

	// TODO: move this to MeshLoader class
	if (obj != NULL) {
		obj->initializeMaterialIndices();

		std::map<std::string, std::vector<MeshLoader::OBJ::Object*>> materialObjectMap = obj->createMaterialObjectMap();
		std::vector<Mesh::vertex> vertices;
		std::vector<Mesh::triangle> triangles;
		vertices.reserve(obj->getVertexCount());
		triangles.reserve(obj->getTriangleCount());

		m_objectSections.clear();

		std::vector<UnloadedMeshMaterialSection> materials;
		materials.reserve(obj->getMaterialCount());

		for (auto it = materialObjectMap.begin(); it != materialObjectMap.end(); it++) {
			std::string materialName = it->first;
			std::vector<MeshLoader::OBJ::Object*>& objects = it->second;

			uint32_t startIndex = triangles.size();
			for (int i = 0; i < objects.size(); i++) {
				MeshLoader::OBJ::Object* object = objects[i];
				std::pair<uint32_t, uint32_t> section = std::make_pair(object->getTriangleBeginIndex(), object->getTriangleEndIndex());
				std::string key = object->getObjectName();
				if (key == "default") key = object->getGroupName();

				object->fillMeshBuffers(vertices, triangles);
				m_objectSections[key].push_back(section);
			}
			
			uint32_t endIndex = triangles.size();
			MaterialConfiguration* materialConfig = obj->createMaterialConfiguration(materialName);
			UnloadedMeshMaterialSection materialSection;
			materialSection.material = *materialConfig;
			materialSection.sections.push_back(std::make_pair(startIndex, endIndex));
			materialSection.name = materialName;
			materials.push_back(materialSection);
			delete materialConfig;
		}

		// TODO: dont delete this, move it to MeshLoader as a useful function

		//std::vector<Mesh::vertex> vertices = obj->getVertices();
		//std::vector<Mesh::triangle> triangles = obj->getTriangles();
		//
		//std::vector<UnloadedMeshMaterialSection> materials;
		//std::map<std::string, UnloadedMeshMaterialSection> materialMap;
		//
		//for (int i = 0; i < obj->getObjectCount(); i++) {
		//	MeshLoader::OBJ::Object* mesh = obj->getObject(i);
		//
		//	if (mesh != NULL) {
		//		MaterialConfiguration* config = mesh->createMaterialConfiguration();
		//		std::pair<uint32_t, uint32_t> section = std::make_pair(mesh->getTriangleBeginIndex(), mesh->getTriangleEndIndex());
		//
		//		UnloadedMeshMaterialSection& materialSection = materialMap[mesh->getMaterialName()];
		//		materialSection.material = *config;
		//		materialSection.sections.push_back(section);
		//		delete config;
		//	}
		//}
		//
		//materials.reserve(materialMap.size());
		//uint32_t drawCalls = 0;
		//
		//for (auto it0 = materialMap.begin(); it0 != materialMap.end(); it0++) {
		//	UnloadedMeshMaterialSection& materialSection = it0->second;
		//
		//	std::sort(materialSection.sections.begin(), materialSection.sections.end()); // sorts by pair::startTriangleIndex, then pair::second
		//	uint32_t startCount = materialSection.sections.size();
		//	uint32_t overlapping = 0;
		//	for (auto it1 = materialSection.sections.begin(); it1 != materialSection.sections.end();) {
		//		if ((it1 + 1) == materialSection.sections.end())
		//			break;
		//
		//		std::pair<uint32_t, uint32_t>& t0 = *(it1 + 0);
		//		std::pair<uint32_t, uint32_t>& t1 = *(it1 + 1);
		//		if (t1.startTriangleIndex <= t0.second) { // t0 and t1 overlap. Merge them and erase one
		//			overlapping++;
		//			t1.startTriangleIndex = t0.startTriangleIndex;
		//			it1 = materialSection.sections.erase(it1);
		//
		//			//if (it1 == materialSection.sections.end()) {
		//			//	break;
		//			//}
		//		} else {
		//			it1++;
		//		}
		//	}
		//
		//	if (overlapping > 0 || materialSection.sections.size() != startCount) {
		//		//info("Material had %d overlapping sections - reduced from %d to %d sections\n", overlapping, startCount, materialSection.sections.size());
		//	}
		//	materialSection.sections.shrink_to_fit();
		//	drawCalls += materialSection.sections.size();
		//	materials.push_back(materialSection);
		//}
		//
		//info("Loaded OBJ model - %d objects, %d materials, %d draw calls\n", obj->getObjectCount(), obj->getMaterialCount(), drawCalls);

		info("Loaded OBJ model with %d triangles, %d vertices, %d object sections\n", obj->getTriangleCount(), obj->getVertexCount(), m_objectSections.size());

		this->loadMaterials(materials);

		m_mesh = new Mesh(vertices, triangles);
		//m_mesh->allocateGPU();
		//m_mesh->uploadGPU();
		////m_mesh->deallocateCPU();
	}


	m_ownsMesh = true;
	delete obj;
}

void MultiRenderComponent::loadMesh(Mesh* mesh) {
	if (m_mesh != NULL && m_ownsMesh)
		delete m_mesh;

	m_mesh = mesh;
	m_ownsMesh = false;
}

void MultiRenderComponent::loadMaterials(std::vector<UnloadedMeshMaterialSection> materials) {
	for (int i = 0; i < m_materials.size(); i++) {
		if (m_materials[i].ownsMaterial)
			delete m_materials[i].materialSection.material;
	}
	m_materials.clear();

	//std::map<std::string, uint64_t> mem;

	for (int i = 0; i < materials.size(); i++) {
		OwnableMeshMaterialSection material;
		Material* m = new Material(materials[i].material);
		material.materialSection.material = m;
		material.materialSection.sections = materials[i].sections;
		material.materialSection.name = materials[i].name;
		material.ownsMaterial = true;
		m_materials.push_back(material);

		//if (m->getAlbedoMap().exists()) mem[materials[i].name] += m->getAlbedoMap()->getMemorySize();
		//if (m->getNormalMap().exists()) mem[materials[i].name] += m->getNormalMap()->getMemorySize();
		//if (m->getRoughnessMap().exists()) mem[materials[i].name] += m->getRoughnessMap()->getMemorySize();
		//if (m->getMetalnessMap().exists()) mem[materials[i].name] += m->getMetalnessMap()->getMemorySize();
		//if (m->getAmbientOcclusionMap().exists()) mem[materials[i].name] += m->getAmbientOcclusionMap()->getMemorySize();
	}

	//uint64_t totalMem = 0;
	//for (auto it = mem.begin(); it != mem.end(); it++) {
	//	totalMem += it->second;
	//	double d = it->second;
	//	std::string suffix = "B";
	//	if (d >= 1024.0) {
	//		d /= 1024.0, suffix = "KiB";
	//		if (d >= 1024.0) {
	//			d /= 1024.0, suffix = "MiB-";
	//		}
	//	}
	//	info("%.3f %s\t -\t%s\n", d, suffix.c_str(), it->startTriangleIndex.c_str());
	//}
	//
	//info("TOTAL MATERIAL MEMORY: %.2f MiB\n", totalMem / (1024.0 * 1024.0));
}

void MultiRenderComponent::loadMaterials(std::vector<MeshMaterialSection> materials) {
	for (int i = 0; i < m_materials.size(); i++) {
		if (m_materials[i].ownsMaterial)
			delete m_materials[i].materialSection.material;
	}
	m_materials.clear();

	for (int i = 0; i < materials.size(); i++) {
		OwnableMeshMaterialSection material;
		material.materialSection.material = materials[i].material;
		material.materialSection.sections = materials[i].sections;
		material.ownsMaterial = false;
		m_materials.push_back(material);
	}
}




LightComponent::LightComponent(Light* light) :
	m_light(light) {
}

LightComponent::~LightComponent() {

}

void LightComponent::preRender(TransformChain& parentTransform, double dt, double partialTicks) {
	Engine::scene()->addActiveLight(m_light);
	//if (m_shadowMapRenderer != NULL) {
	//	PointLight* pointLight = dynamic_cast<PointLight*>(m_light);
	//	if (pointLight != NULL) {
	//		m_shadowMapRenderer->renderShadow(pointLight, dt, partialTicks);
	//	}
	//}

	m_light->render(dt, partialTicks);
}

Light* LightComponent::getLight() {
	return m_light;
}




MeshComponent::MeshComponent(Mesh* mesh, uint32_t firstTriangle, uint32_t lastTriangle) {
	this->loadMesh(mesh);
	this->loadSections(std::vector<std::pair<uint32_t, uint32_t>> { std::make_pair(firstTriangle, lastTriangle) });
	this->calculateBounds();
}

MeshComponent::MeshComponent(Mesh* mesh, std::pair<uint32_t, uint32_t> triangleRange) {
	this->loadMesh(mesh);
	this->loadSections(std::vector<std::pair<uint32_t, uint32_t>> { triangleRange });
	this->calculateBounds();
}

MeshComponent::MeshComponent(Mesh* mesh, std::vector<std::pair<uint32_t, uint32_t>> triangleSections) {
	this->loadMesh(mesh);
	this->loadSections(triangleSections);
	this->calculateBounds();
}

MeshComponent::MeshComponent(UnloadedMesh mesh, uint32_t firstTriangle, uint32_t lastTriangle) {
	this->loadMesh(mesh);
	this->loadSections(std::vector<std::pair<uint32_t, uint32_t>> { std::make_pair(firstTriangle, lastTriangle) });
	this->calculateBounds();
}

MeshComponent::MeshComponent(UnloadedMesh mesh, std::pair<uint32_t, uint32_t> triangleRange) {
	this->loadMesh(mesh);
	this->loadSections(std::vector<std::pair<uint32_t, uint32_t>> { triangleRange });
	this->calculateBounds();
}

MeshComponent::MeshComponent(UnloadedMesh mesh, std::vector<std::pair<uint32_t, uint32_t>> triangleSections) {
	this->loadMesh(mesh);
	this->loadSections(triangleSections);
	this->calculateBounds();
}

void MeshComponent::render(TransformChain& parentTransform, double dt, double partialTicks) {
}

MeshComponent* MeshComponent::transferMesh() {
	m_ownsMesh = true;
	return this;
}

Mesh* MeshComponent::getMesh() const {
	return m_mesh;
}

AxisAlignedBB MeshComponent::getEnclosingBounds() const {
	return m_enclosingBounds;
}

std::vector<std::pair<uint32_t, uint32_t>> MeshComponent::getSections() const {
	return m_sections;
}

std::pair<uint32_t, uint32_t> MeshComponent::getSection(uint32_t index) const {
	assert(index < m_sections.size());
	return m_sections[index];
}

uint32_t MeshComponent::getSectionCount() const {
	return m_sections.size();
}

uint32_t MeshComponent::getTriangleCount() const {
	return m_mesh->getTriangleCount();
}

uint32_t MeshComponent::getVertexCount() const {
	return m_mesh->getVertexCount();
}

RaycastResult* MeshComponent::raycast(TransformChain& parentTransform, dvec3 rayOrigin, dvec3 rayDirection) {
	return raycastTransformedMesh(m_mesh, parentTransform, rayOrigin, rayDirection);
}

void MeshComponent::updateBounds() {
	//m_enclosingBounds = AxisAlignedBB();
	//
	//for (Mesh::index i = 0; i < m_mesh->getTriangleCount(); i++) {
	//	const Mesh::triangle& tri = m_mesh->getTriangle(i);
	//	const Mesh::vertex& v0 = m_mesh->getVertex(tri.i0);
	//	const Mesh::vertex& v1 = m_mesh->getVertex(tri.i1);
	//	const Mesh::vertex& v2 = m_mesh->getVertex(tri.i2);
	//
	//	m_enclosingBounds = m_enclosingBounds.extend({ v0.position, v1.position, v2.position });
	//}
}

void MeshComponent::loadMesh(Mesh* mesh) {
	if (m_mesh != NULL && m_ownsMesh)
		delete m_mesh;

	m_mesh = mesh;
	m_ownsMesh = false;
}

void MeshComponent::loadMesh(UnloadedMesh mesh) {
	if (m_mesh != NULL && m_ownsMesh)
		delete m_mesh;

	MeshLoader::OBJ* obj = MeshLoader::OBJ::load(std::string(mesh.modelFilePath));

	if (obj == NULL) {
		m_mesh = NULL;
	} else {
		m_mesh = obj->createMesh(true, true);
		m_ownsMesh = true;

		delete obj;
	}
}

void MeshComponent::loadSections(std::vector<std::pair<uint32_t, uint32_t>> triangleSections) {
	if (m_mesh == NULL || m_mesh->getTriangleCount() == 0) {
		m_sections = {};
		return;
	}

	for (auto it = triangleSections.begin(); it != triangleSections.end();) {
		std::pair<uint32_t, uint32_t>& section = *it;
		if (section.first > section.second)
			std::swap(section.first, section.second);

		if (section.first == section.second || section.first > m_mesh->getTriangleCount()) {
			it = triangleSections.erase(it);
		} else {
			if (section.second > m_mesh->getTriangleCount()) {
				section.second = m_mesh->getTriangleCount();
			}
			it++;
		}
	}
	m_sections = triangleSections;
}

void MeshComponent::calculateBounds() {
	if (m_mesh != NULL && !m_sections.empty()) {
		m_sectionBounds.clear();
		m_sectionBounds.resize(m_sections.size());

		dvec3 minEnclosingBound = dvec3(+INFINITY);
		dvec3 maxEnclosingBound = dvec3(-INFINITY);

		for (int i = 0; i < m_sections.size(); i++) {
			std::pair<uint32_t, uint32_t> section = m_sections[i];
			uint32_t firstIndex = section.first;
			uint32_t lastIndex = section.second;

			dvec3 minSectionBound = dvec3(+INFINITY);
			dvec3 maxSectionBound = dvec3(-INFINITY);

			for (int j = firstIndex; j < lastIndex; j++) {
				Mesh::triangle tri = m_mesh->getTriangle(j);
				for (int k = 0; k < 3; k++) {
					Mesh::vertex vtx = m_mesh->getVertex(tri.indices[k]);
					dvec3 p = vtx.position;
					minSectionBound = glm::min(minSectionBound, p);
					maxSectionBound = glm::max(maxSectionBound, p);
					minEnclosingBound = glm::min(minEnclosingBound, p);
					maxEnclosingBound = glm::max(maxEnclosingBound, p);
				}
			}

			m_sectionBounds[i] = AxisAlignedBB(minSectionBound, maxSectionBound);
		}

		m_enclosingBounds = AxisAlignedBB(minEnclosingBound, maxEnclosingBound);
	}
}
