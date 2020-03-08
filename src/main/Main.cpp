#include "core/Engine.h"
#include "core/InputHandler.h"
#include "core/scene/Scene.h"
#include "core/scene/SceneComponents.h"
#include "core/scene/Camera.h"
#include "core/scene/FirstPersonController.h"
#include "core/scene/BoundingVolumeHierarchy.h"
#include "core/renderer/geometry/Mesh.h"
#include "core/renderer/geometry/MeshLoader.h"
#include "core/renderer/ShaderProgram.h"
#include "core/renderer/ScreenRenderer.h"
#include "core/renderer/ShadowMapRenderer.h"
#include "core/renderer/Lights.h"
#include "core/renderer/Texture.h"
#include "core/renderer/Material.h"
#include "core/renderer/CubeMap.h"
#include "core/renderer/EnvironmentMap.h"
#include "core/renderer/VoxelGenerator.h"
#include "core/util/FileUtils.h"

struct VoxelNode {
	uint32_t data;

	VoxelNode(uint8_t pattern, uint32_t firstChild) {
		data = pattern | (firstChild << 8);
	}
};

void divide(std::vector<VoxelNode>& nodes, int parent, uint8_t pattern) {
	nodes[parent] = VoxelNode(pattern, nodes.size());
	nodes.push_back(VoxelNode(0, 0));
	nodes.push_back(VoxelNode(0, 0));
	nodes.push_back(VoxelNode(0, 0));
	nodes.push_back(VoxelNode(0, 0));
}

void buildNodes(std::vector<VoxelNode>& nodes, uint8_t pattern, uint32_t depth = 12) {
	if (depth == 0) {
		return;
	}

	uint32_t nodeCount = nodes.size();
	for (int i = 0; i < nodeCount; i++) {
		if (nodes[i].data == 0) { // leaf
			divide(nodes, i, pattern);
		}
	}

	buildNodes(nodes, pattern, depth - 1);

	//VoxelNode nodes[nodeCount];
	//nodes[0].data = (0b00101011) | (1 << 8);
	//nodes[1].data = 0;
	//nodes[2].data = 0;
	//nodes[3].data = 0;
	//nodes[4].data = (0b00101011) | (5 << 8);
	//nodes[5].data = 0;
	//nodes[6].data = 0;
	//nodes[7].data = 0;
	//nodes[8].data = 0;
}
#define SWAP(t, a, b) { t = a; a = b; b = t; }
#define SORT2(v0, v1, i0, i1) if (v0 > v1) { SWAP(tempVal, v0, v1) SWAP(tempIdx, i0, i1) }
#define SORT8(type, va, ia) { \
	type tempVal; \
	int tempIdx; \
    SORT2(va[0],va[1],ia[0],ia[1])SORT2(va[2],va[3],ia[2],ia[3])SORT2(va[0],va[2],ia[0],ia[2])SORT2(va[1],va[3],ia[1],ia[3]) \
    SORT2(va[1],va[2],ia[1],ia[2])SORT2(va[4],va[5],ia[4],ia[5])SORT2(va[6],va[7],ia[6],ia[7])SORT2(va[4],va[6],ia[4],ia[6]) \
    SORT2(va[5],va[7],ia[5],ia[7])SORT2(va[5],va[6],ia[5],ia[6])SORT2(va[0],va[4],ia[0],ia[4])SORT2(va[1],va[5],ia[1],ia[5]) \
    SORT2(va[1],va[4],ia[1],ia[4])SORT2(va[2],va[6],ia[2],ia[6])SORT2(va[3],va[7],ia[3],ia[7])SORT2(va[3],va[6],ia[3],ia[6]) \
    SORT2(va[2],va[4],ia[2],ia[4])SORT2(va[3],va[5],ia[3],ia[5])SORT2(va[3],va[4],ia[3],ia[4]) \
}

int start_raytracer(int argc, char** argv) {
	try {
		Engine::create(argc, argv);

		Mesh::Builder* builder = new Mesh::Builder();
		//Mesh::reference t0 = builder->addTriangle(vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 1.0, 0.0));
		//Mesh::reference t1 = builder->addTriangle(vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(0.0, 1.0, 1.0));


		uint64_t ta = Engine::instance()->getCurrentTime();
		std::string bunny;
		//if (FileUtils::loadFile(RESOURCE_PATH("models/bunny/bunny.obj"), bunny)) {
		//	uint64_t tb = Engine::instance()->getCurrentTime();
		//	builder->parseOBJ(bunny, 35000);
		//	uint64_t tc = Engine::instance()->getCurrentTime();
		//
		//	printf("%f msec to load file, %f msec to parse OBJ\n", (tb - ta) / 1000000000.0, (tc - tb) / 1000000000.0);
		//}

		Mesh::index v0 = builder->addVertex(-10.0, 0.0, -10.0, 0.0, 1.0, 0.0);
		Mesh::index v1 = builder->addVertex(-10.0, 0.0, +10.0, 0.0, 1.0, 0.0);
		Mesh::index v2 = builder->addVertex(+10.0, 0.0, +10.0, 0.0, 1.0, 0.0);
		Mesh::index v3 = builder->addVertex(+10.0, 0.0, -10.0, 0.0, 1.0, 0.0);
		builder->addTriangle(v0, v1, v2);
		builder->addTriangle(v0, v2, v3);

		info("Builder has %d vertices, %d triangles before optimisation\n", builder->getVertexCount(), builder->getTriangleCount());
		Mesh* mesh = NULL;
		uint64_t a = Engine::instance()->getCurrentTime();
		//builder->optimise();
		uint64_t b = Engine::instance()->getCurrentTime();
		info("Builder has %d vertices, %d triangles after optimisation - took %f msec\n", builder->getVertexCount(), builder->getTriangleCount(), (b - a) / 1000000.0);
		builder->build(&mesh);

		uint32_t fullscreenQuadVAO;
		glGenVertexArrays(1, &fullscreenQuadVAO);

		uint32_t frameTexture;
		glGenTextures(1, &frameTexture);
		glBindTexture(GL_TEXTURE_2D, frameTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1600, 900, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		std::vector<VoxelNode> nodes;
		nodes.reserve(5592405); // 11=5592405, 12=22369621
		nodes.push_back(VoxelNode(0, 0));
		buildNodes(nodes, 0b00101011);

		info("================================================== %d nodes\n", nodes.size());

		uint32_t voxelBuffer;
		glGenBuffers(1, &voxelBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(VoxelNode) * nodes.size(), &nodes[0], GL_DYNAMIC_COPY);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		ShaderProgram* rasterShader = new ShaderProgram();
		rasterShader->addShader(GL_VERTEX_SHADER, "shaders/raster/phong/vert.glsl");
		rasterShader->addShader(GL_FRAGMENT_SHADER, "shaders/raster/phong/frag.glsl");
		rasterShader->addAttribute(0, "vs_vertexPosition");
		rasterShader->addAttribute(1, "vs_vertexNormal");
		rasterShader->addAttribute(2, "vs_vertexTexture");
		rasterShader->completeProgram();

		ShaderProgram* screenShader = new ShaderProgram();
		screenShader->addShader(GL_VERTEX_SHADER, "shaders/raster/screen/vert.glsl");
		screenShader->addShader(GL_FRAGMENT_SHADER, "shaders/raster/screen/frag.glsl");
		screenShader->completeProgram();

		ShaderProgram* raytraceShader = new ShaderProgram();
		raytraceShader->addShader(GL_COMPUTE_SHADER, "shaders/raytrace/voxel/comp.glsl");
		raytraceShader->completeProgram();

		Camera* camera = new Camera(M_PI / 2.0, Engine::instance()->getWindowAspectRatio());
		camera->transform().lookAt(dvec3(0.0, 1.5, -2.8), dvec3(0.0, 1.0, 0.0));
		//camera->transformPosition(0.3, 0.0, 0.0);
		//camera->setRoll(0.01);
		camera->render(0.0, 0.0);

		mat4 modelMatrix = mat4(1.0);

		double distance = 4.0;
		double theta = 0.0;

		mesh->allocateGPU();
		mesh->uploadGPU();
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		while (!Engine::instance()->isStopped()) {
			if (!Engine::instance()->update()) {
				break;
			}

			if (Engine::instance()->didUpdateFrame()) {

				theta = Engine::instance()->getRunTime() * M_PI * 2.0 * 0.1;
				dvec3 center = dvec3(-2.5, -2.5, 2.5);
				dvec3 offset = dvec3(sin(theta) * distance, 1.5, cos(theta) * distance);
				camera->transform().lookAt(center + offset, center);
				camera->render(0.0, 0.0);

				//info("%d %d\n", Engine::instance()->getInputHandler()->getMousePixelMotion().x, Engine::instance()->getInputHandler()->getMousePixelMotion().y);
				glEnable(GL_TEXTURE_2D);

				////modelMatrix = glm::rotate(mat4(1.0), float(Engine::instance()->getRunTime() * M_PI * 2.0 * 0.1), vec3(0.0, 1.0, 0.0));
				//modelMatrix = glm::scale(mat4(1.0), vec3(15.0));
				//ShaderProgram::use(rasterShader);
				//rasterShader->setUniform("modelMatrix", modelMatrix);
				//rasterShader->setUniform("viewProjectionMatrix", camera->getViewProjectionMatrix());
				//mesh->draw();
				//ShaderProgram::use(NULL);

				ShaderProgram::use(raytraceShader);
				raytraceShader->setUniform("cameraRays", camera->getScreenRays());
				raytraceShader->setUniform("cameraPosition", camera->transform().getTranslation());
				raytraceShader->setUniform("nodeCount", (int32_t)nodes.size());
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelBuffer);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, voxelBuffer);
				glBindImageTexture(0, frameTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
				glDispatchCompute((int)ceil(1600.0F / 8), (int)ceil(900.0F / 8), 1);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

				ShaderProgram::use(screenShader);
				glBindTextureUnit(0, frameTexture);
				screenShader->setUniform("screenTexture", 0);
				glBindVertexArray(fullscreenQuadVAO);
				glDrawArrays(GL_TRIANGLES, 0, 3);
				glBindVertexArray(0);
				ShaderProgram::use(NULL);
			}

			if (Engine::instance()->didUpdateTick()) {
				//debug("Tick update %.2f/s\n", 1.0 / Engine::instance()->getTickDeltaTime());
			}
		}

		Engine::destroy();
	} catch (std::exception e) {
		error("Engine threw a fatal exception: %s\n", e.what());
		return -1;
	}
	return 0;
}

int start_rasterizer(int argc, char** argv) {
	try {
		Engine::create(argc, argv);

		Mesh::Builder* builder = new Mesh::Builder();

		Mesh::index v0 = builder->addVertex(-60.0, -0.15, -60.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
		Mesh::index v1 = builder->addVertex(-60.0, -0.15, +60.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 12.0);
		Mesh::index v2 = builder->addVertex(+60.0, -0.15, +60.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 12.0, 12.0);
		Mesh::index v3 = builder->addVertex(+60.0, -0.15, -60.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 12.0, 0.0);
		builder->addTriangle(v0, v1, v2);
		builder->addTriangle(v0, v2, v3);

		Mesh* floorMesh = NULL;
		builder->build(&floorMesh, true);

		// 690 - 799 fps - 1 layer
		// 411 - 530 fps - 3 layer
		// 540 - 617 fps - 3 layer, discard fragments of layer != 0
		ShaderProgram* rasterShader = new ShaderProgram(); 
		rasterShader->addShader(GL_VERTEX_SHADER, "shaders/raster/phong/vert.glsl");
		rasterShader->addShader(GL_GEOMETRY_SHADER, "shaders/raster/phong/geom.glsl");
		rasterShader->addShader(GL_FRAGMENT_SHADER, "shaders/raster/phong/frag.glsl");
		Mesh::addVertexInputs(rasterShader);
		ScreenRenderer::addFragmentOutputs(rasterShader);
		rasterShader->completeProgram();

		Engine::scene()->setSpawnLocation(dvec3(0.0, 1.5, -4.8));


		Resource<Texture2D> uvGridTexture = Engine::resourceHandler()->request<Texture2D>("textures/uv_colorgrid.png", "textures/uv_colorgrid.png");
		if (uvGridTexture != NULL)
			uvGridTexture->setFilterMode(TextureFilter::LINEAR_MIPMAP_NEAREST_PIXEL, TextureFilter::NEAREST_PIXEL, 16.0);



		//SceneObject* bunnyObject = new SceneObject(Transformation(dvec3(-1.8, 0.15, 1.0), dquat(), dvec3(10.0)));
		//bunnyObject->addComponent("bunny_renderer", new RenderComponent("models/bunny/bunny", rasterShader));
		//Engine::scene()->getRoot()->addChild("bunny_object", bunnyObject);

		//CubeMap* cubeMap = NULL;
		//if (!CubeMap::load({
		//	RESOURCE_PATH("environments/ame_desert/desertsky_rt.tga"),
		//	RESOURCE_PATH("environments/ame_desert/desertsky_lf.tga"),
		//	RESOURCE_PATH("environments/ame_desert/desertsky_up.tga"),
		//	RESOURCE_PATH("environments/ame_desert/desertsky_dn.tga"),
		//	RESOURCE_PATH("environments/ame_desert/desertsky_bk.tga"),
		//	RESOURCE_PATH("environments/ame_desert/desertsky_ft.tga"),
		//	}, &cubeMap)) {
		//	return -1;
		//}

		CubemapConfiguration cubemapConfig;
		cubemapConfig.equirectangularFilePath = RESOURCE_PATH("environments/piazza_bologni/piazza_bologni_8k.hdr");
		//cubemapConfig.leftFilePath = RESOURCE_PATH("environments/headPointerTexture/negx.bmp");
		//cubemapConfig.rightFilePath = RESOURCE_PATH("environments/headPointerTexture/posx.bmp");
		//cubemapConfig.bottomFilePath = RESOURCE_PATH("environments/headPointerTexture/negy.bmp");
		//cubemapConfig.topFilePath = RESOURCE_PATH("environments/headPointerTexture/posy.bmp");
		//cubemapConfig.frontFilePath = RESOURCE_PATH("environments/headPointerTexture/posz.bmp");
		//cubemapConfig.backFilePath = RESOURCE_PATH("environments/headPointerTexture/negz.bmp");
		cubemapConfig.floatingPoint = true;
		Engine::scene()->setGlobalEnvironmentMap((new LightProbe(cubemapConfig))->calculateDiffuseIrradianceMap()->calculateSpecularReflectionMap());


		//SceneObject* galleryObject = new SceneObject(Transformation(dvec3(0.0, 0.0, 0.0), dquat(), dvec3(1.0)));
		//galleryObject->addComponent("gallery_renderer", new RenderComponent("models/gallery/gallery.obj", rasterShader));
		//Engine::scene()->getRoot()->addChild("gallery_object", galleryObject);

		//SceneObject* breakfastRoomObject = new SceneObject(Transformation(dvec3(0.0, 0.0, 0.0), dquat(), dvec3(1.0)));
		//breakfastRoomObject->addComponent("breakfastroom_renderer", new RenderComponent("models/breakfast_room/breakfast_room", rasterShader));
		//Engine::scene()->getRoot()->addChild("breakfastroom_object", breakfastRoomObject);

		SceneObject* livingRoomObject = new SceneObject();
		livingRoomObject->addComponent("livingroom_renderer", new RenderComponent("models/living_room/living_room", rasterShader));
		Engine::scene()->getRoot()->addChild("livingroom_object", livingRoomObject);

		//SceneObject* sanMiguelObject = new SceneObject();
		//sanMiguelObject->addComponent("sanmiguel_renderer", new RenderComponent("models/san_miguel/san-miguel", rasterShader));
		//Engine::scene()->getRoot()->addChild("sanmiguel_object", sanMiguelObject);


		//SceneObject* bistroExteriorObject = new SceneObject(Transformation(dvec3(5.0, 0.0, 0.0), dquat(), dvec3(0.01)));
		//bistroExteriorObject->addComponent("bistro_exterior_renderer", new RenderComponent("models/bistro/Exterior/exterior", rasterShader));
		//Engine::scene()->getRoot()->addChild("bistro_exterior_object", bistroExteriorObject);
		//
		//SceneObject* bistroInteriorObject = new SceneObject(Transformation(dvec3(5.0, 0.0, 0.0), dquat(), dvec3(0.01)));
		//bistroInteriorObject->addComponent("bistro_interior_renderer", new RenderComponent("models/bistro/Interior/interior", rasterShader));
		//Engine::scene()->getRoot()->addChild("bistro_interior_object", bistroInteriorObject);

		//Mesh* boxMesh = NULL;
		//builder->clear();
		//builder->createCuboid();
		//builder->build(&boxMesh, true);
		//
		//Mesh* sphereMesh = NULL;
		//builder->clear();
		//builder->createUVSpheroid(dvec3(0.5));
		//builder->calculateVertexTangents();
		//builder->build(&sphereMesh, true);
		//
		//SceneObject* boxObject = new SceneObject(Transformation(dvec3(-7.8, 1.15, -4.0)));
		//MaterialConfiguration boxMaterialConfig;
		////boxMaterialConfig.albedoMap = uvGridTexture;
		////boxMaterialConfig.albedoMap
		//boxObject->addComponent("box_renderer", new RenderComponent(boxMesh, boxMaterialConfig, rasterShader));
		//Engine::scene()->getRoot()->addChild("box_object", boxObject);
		//
		////Material* sphereMaterial = new Material("textures/chipped-paint-metal");
		//for (int i = 0; i < 5; i++) {
		//	for (int j = 0; j < 5; j++) {
		//		MaterialConfiguration sphereMaterial;
		//		sphereMaterial.albedo = dvec3(1.0, 0.4, 0.1);
		//		sphereMaterial.metalness = i / 4.0;
		//		sphereMaterial.roughness = j / 4.0;
		//		SceneObject* sphereObject = new SceneObject(Transformation(dvec3(-3.0 + i * 1.1, 0.7 + j * 1.1, 4.0)));
		//		sphereObject->addComponent("sphere_renderer", new RenderComponent(sphereMesh, sphereMaterial, rasterShader));
		//		Engine::scene()->getRoot()->addChild("sphere_object_" + std::to_string(i) + "_" + std::to_string(j), sphereObject);
		//	}
		//}
		//
		//SceneObject* chippedPaintMetalSphere = new SceneObject(Transformation(dvec3(-5.3, 0.6, -3.0)));
		//chippedPaintMetalSphere->addComponent("sphere_renderer", new RenderComponent(sphereMesh, "textures/chipped-paint-metal", rasterShader));
		//Engine::scene()->getRoot()->addChild("chipped_paint_metal_sphere", chippedPaintMetalSphere);
		//
		//SceneObject* greasyMetalSphere = new SceneObject(Transformation(dvec3(-5.3, 0.6, -1.8)));
		//greasyMetalSphere->addComponent("sphere_renderer", new RenderComponent(sphereMesh, "textures/greasy-metal-pan1", rasterShader));
		//Engine::scene()->getRoot()->addChild("greasy_metal_sphere", greasyMetalSphere);
		//
		//SceneObject* oxidizedCopperSphere = new SceneObject(Transformation(dvec3(-5.3, 0.6, -0.6)));
		//oxidizedCopperSphere->addComponent("sphere_renderer", new RenderComponent(sphereMesh, "textures/oxidized-copper", rasterShader));
		//Engine::scene()->getRoot()->addChild("oxidized_copper_sphere", oxidizedCopperSphere);
		//
		//SceneObject* rustedIronSphere = new SceneObject(Transformation(dvec3(-5.3, 0.6, 0.8)));
		//rustedIronSphere->addComponent("sphere_renderer", new RenderComponent(sphereMesh, "textures/rustediron-streaks2", rasterShader));
		//Engine::scene()->getRoot()->addChild("rusted_iron_sphere", rustedIronSphere);
		//
		//SceneObject* streakedMetalSphere = new SceneObject(Transformation(dvec3(-5.3, 0.6, 2.0)));
		//streakedMetalSphere->addComponent("sphere_renderer", new RenderComponent(sphereMesh, "textures/streaked-metal", rasterShader));
		//Engine::scene()->getRoot()->addChild("streaked_metal_sphere", streakedMetalSphere);
		//
		//SceneObject* floorObject = new SceneObject(Transformation(glm::scale(dmat4(1.0), dvec3(1.0))));
		//MaterialConfiguration floorMaterialConfig;
		//floorMaterialConfig.albedoMap = uvGridTexture;
		//floorObject->addComponent("floor_renderer", new RenderComponent(floorMesh, floorMaterialConfig, rasterShader));
		//Engine::scene()->getRoot()->addChild("floor_object", floorObject);

		Engine::scene()->getRoot()->addComponent("light0", new LightComponent(new PointLight(dvec3(2.0, 0.4, -1.0), dvec3(0.89, 1.0, 0.92) * 24.0, 1.0, 0.0, 1.0/*, new ShadowMap(512, 512)*/)));
		Engine::scene()->getRoot()->addComponent("light1", new LightComponent(new PointLight(dvec3(-3.0, 1.4, -0.3), dvec3(1.0, 0.6, 0.52) * 24.0, 1.0, 0.0, 1.0/*, new ShadowMap(512, 512)*/)));
		Engine::scene()->getRoot()->addComponent("light3", new LightComponent(new PointLight(dvec3(5.0, 5.4, 4.1), dvec3(1.0, 0.93, 0.9) * 24.0, 1.0, 0.0, 1.0/*, new ShadowMap(512, 512)*/)));


		while (!Engine::instance()->isStopped()) {
			if (!Engine::instance()->update()) {
				break;
			}

			if (Engine::instance()->didUpdateFrame()) {
				if (Engine::inputHandler()->keyPressed(SDL_SCANCODE_F1)) {
					Engine::instance()->setDebugRenderWireframeEnabled(!Engine::instance()->isDebugRenderWireframeEnabled());
				}
				if (Engine::inputHandler()->keyPressed(SDL_SCANCODE_F2)) {
					Engine::instance()->setDebugRenderLightingEnabled(!Engine::instance()->isDebugRenderLightingEnabled());
				}
				if (Engine::inputHandler()->keyPressed(SDL_SCANCODE_F3)) {
					if (Engine::inputHandler()->keyDown(SDL_SCANCODE_LSHIFT) || Engine::inputHandler()->keyDown(SDL_SCANCODE_RSHIFT))
						Engine::instance()->setDebugRenderGBufferMode(0);
					else
						Engine::instance()->setDebugRenderGBufferMode((Engine::instance()->getDebugRenderGBufferMode() + 1) % 9);
				}
				if (Engine::inputHandler()->keyPressed(SDL_SCANCODE_F4)) {
					Engine::instance()->setImageBasedLightingEnabled(!Engine::instance()->isImageBasedLightingEnabled());
				}
				if (Engine::inputHandler()->keyPressed(SDL_SCANCODE_F5)) {
					Engine::instance()->setDebugRenderVoxelGridEnabled(!Engine::instance()->isDebugRenderVoxelGridEnabled());
					Engine::instance()->scene()->getVoxelizer()->setDebugOctreeVisualisationLevel(-1);
				}
				if (Engine::inputHandler()->keyPressed(SDL_SCANCODE_EQUALS)) {
					Engine::instance()->scene()->getVoxelizer()->incrDebugOctreeVisualisationLevel(1);
				} else if (Engine::inputHandler()->keyPressed(SDL_SCANCODE_MINUS)) {
					Engine::instance()->scene()->getVoxelizer()->incrDebugOctreeVisualisationLevel(-1);
				}

				if (Engine::inputHandler()->keyDown(SDL_SCANCODE_LALT)) {
					Engine::instance()->scene()->getController()->setWalkSpeed(0.1);
				} else {
					Engine::instance()->scene()->getController()->setWalkSpeed(3.0);
				}
			}

			if (Engine::instance()->didUpdateTick()) {

			}
		}

		Engine::destroy();
	} catch (std::exception e) {
		error("Engine threw a fatal exception: %s\n", e.what());
		return -1;
	}
	return 0;
}

int main(int argc, char** argv) {
	return start_rasterizer(argc, argv);
	//return start_raytracer(argc, argv);
}