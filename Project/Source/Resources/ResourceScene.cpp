#include "ResourceScene.h"

#include "GameObject.h"
#include "Application.h"
#include "Scene.h"
#include "Utils/Logging.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleFiles.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

#include "Utils/Leaks.h"

void ResourceScene::Load() {
	// Timer to measure loading a scene
	MSTimer timer;
	timer.Start();

	const char* filePath = GetResourceFilePath().c_str();
	LOG("Loading scene from path: \"%s\".", filePath);
	
	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) return;

	// Parse document from file
	rapidjson::Document document;
	document.ParseInsitu<rapidjson::kParseNanAndInfFlag>(buffer.Data());
	if (document.HasParseError()) {
		LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document.GetParseError()), document.GetErrorOffset());
		return;
	}

	// Create scene
	JsonValue jScene = JsonValue(document, document);
	scene = new Scene(10000);
	scene->Load(jScene);

	unsigned timeMs = timer.Stop();
	LOG("Scene loaded in %ums.", timeMs);
}

void ResourceScene::FinishLoading() {
	scene->Init();
}

void ResourceScene::Unload() {
	RELEASE(scene);
}

Scene* ResourceScene::GetScene() {
	return scene;
}

Scene* ResourceScene::TransferScene() {
	Scene* currentScene = scene;
	scene = nullptr;
	return currentScene;
}
