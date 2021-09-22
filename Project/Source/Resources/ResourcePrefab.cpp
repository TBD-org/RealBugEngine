#include "ResourcePrefab.h"

#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleTime.h"
#include "Utils/Logging.h"
#include "Utils/MSTimer.h"

#include "rapidjson/error/en.h"

#define JSON_TAG_ROOT "Root"
#define JSON_TAG_NAME "Name"
#define JSON_TAG_PARENT_INDEX "ParentIndex"

void ResourcePrefab::Load() {
	// Timer to measure bulding a prefab
	MSTimer timer;
	timer.Start();

	std::string filePath = GetResourceFilePath();
	LOG("Building prefab from path: \"%s\".", filePath.c_str());

	// Read from file
	Buffer<char> buffer = App->files->Load(filePath.c_str());
	if (buffer.Size() == 0) return;

	// Parse document from file
	document = new rapidjson::Document();
	document->Parse<rapidjson::kParseNanAndInfFlag>(buffer.Data());
	if (document->HasParseError()) {
		LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document->GetParseError()), document->GetErrorOffset());
		return;
	}

	// Create auxiliary scene
	prefabScene = new Scene(100);

	JsonValue jPrefab(*document, *document);

	// Load GameObjects
	JsonValue jRoot = jPrefab[JSON_TAG_ROOT];
	UID gameObjectId = GenerateUID();
	GameObject* gameObject = prefabScene->gameObjects.Obtain(gameObjectId);
	prefabScene->root = gameObject;
	gameObject->scene = prefabScene;
	gameObject->id = gameObjectId;
	gameObject->SetParent(nullptr);
	gameObject->LoadPrefab(jRoot);

	unsigned timeMs = timer.Stop();
	LOG("Prefab loaded in %ums.", timeMs);
}

void ResourcePrefab::FinishLoading() {
	prefabScene->root->Init();
}

void ResourcePrefab::Unload() {
	RELEASE(document);
	RELEASE(prefabScene);
}

UID ResourcePrefab::BuildPrefab(GameObject* parent) {
	JsonValue jPrefab(*document, *document);

	// Load GameObjects
	Scene* scene = parent->scene;
	JsonValue jRoot = jPrefab[JSON_TAG_ROOT];
	UID gameObjectId = GenerateUID();
	GameObject* gameObject = scene->gameObjects.Obtain(gameObjectId);
	gameObject->scene = scene;
	gameObject->id = gameObjectId;
	gameObject->SetParent(parent);
	gameObject->LoadPrefab(jRoot);
	gameObject->Init();

	if (App->time->HasGameStarted()) {
		gameObject->Start();
	}

	return gameObjectId;
}