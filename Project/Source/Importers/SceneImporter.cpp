#include "SceneImporter.h"

#include "Application.h"
#include "GameObject.h"
#include "Utils/PathUtils.h"
#include "Utils/Logging.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleTime.h"
#include "Resources/ResourceScene.h"
#include "ImporterCommon.h"
#include "Scene.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

#include "Utils/Leaks.h"

bool SceneImporter::ImportScene(const char* filePath, JsonValue jMeta) {
	// Timer to measure importing a scene
	MSTimer timer;
	timer.Start();
	LOG("Importing scene from path: \"%s\".", filePath);

	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error loading scene %s", filePath);
		return false;
	}

	// Parse document from file
	rapidjson::Document document;
	document.ParseInsitu<rapidjson::kParseNanAndInfFlag>(buffer.Data());
	if (document.HasParseError()) {
		LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document.GetParseError()), document.GetErrorOffset());
		return false;
	}

	// Write document to buffer
	rapidjson::StringBuffer stringBuffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator, rapidjson::kWriteNanAndInfFlag> writer(stringBuffer);
	document.Accept(writer);

	// Create scene resource
	unsigned resourceIndex = 0;
	std::unique_ptr<ResourceScene> scene = ImporterCommon::CreateResource<ResourceScene>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(scene.get());
	if (!saved) {
		LOG("Failed to save scene resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(scene->GetResourceFilePath().c_str(), stringBuffer.GetString(), stringBuffer.GetSize());
	if (!saved) {
		LOG("Failed to save state machine resource file.");
		return false;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(scene);

	unsigned timeMs = timer.Stop();
	LOG("Scene imported in %ums", timeMs);
	return true;
}

Scene* SceneImporter::LoadScene(const char* filePath) {
	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);

	if (buffer.Size() == 0) return nullptr;

	// Parse document from file
	rapidjson::Document document;
	document.ParseInsitu<rapidjson::kParseNanAndInfFlag>(buffer.Data());
	if (document.HasParseError()) {
		LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document.GetParseError()), document.GetErrorOffset());
		return nullptr;
	}
	JsonValue jScene(document, document);

	Scene* scene = new Scene(10000);
	scene->Load(jScene);
	scene->Init();
	return scene;
}

bool SceneImporter::SaveScene(Scene* scene, const char* filePath) {
	// Create document
	rapidjson::Document document;
	document.SetObject();
	JsonValue jScene(document, document);

	scene->Save(jScene);

	// Write document to buffer
	rapidjson::StringBuffer stringBuffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator, rapidjson::kWriteNanAndInfFlag> writer(stringBuffer);
	document.Accept(writer);

	// Save to file
	return App->files->Save(filePath, stringBuffer.GetString(), stringBuffer.GetSize());
}