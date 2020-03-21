#include "core/Engine.h"
#include "core/InputHandler.h"
#include "core/scene/Scene.h"
#include "core/scene/SceneComponents.h"
#include "core/scene/Camera.h"
#include "core/scene/FirstPersonController.h"
#include "core/scene/BoundingVolumeHierarchy.h"
#include "core/renderer/geometry/Mesh.h"
#include "core/renderer/geometry/MeshLoader.h"
#include "core/renderer/MaterialManager.h"
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

int main(int argc, char** argv) {
	try {
		Engine::create(argc, argv);

		while (!Engine::instance()->isStopped()) {
			if (!Engine::instance()->update()) {
				break;
			}

			if (Engine::instance()->didUpdateFrame()) {

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