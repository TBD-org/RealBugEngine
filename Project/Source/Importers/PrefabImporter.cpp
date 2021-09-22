#include "PrefabImporter.h"

#include "Application.h"
#include "GameObject.h"
#include "Utils/Logging.h"
#include "Utils/MSTimer.h"
#include "Utils/PathUtils.h"
#include "Resources/ResourcePrefab.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleFiles.h"
#include "ImporterCommon.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

#include "Utils/Leaks.h"

#define JSON_TAG_ROOT "Root"

bool PrefabImporter::ImportPrefab(const char* filePath, JsonValue jMeta) {
	// Timer to measure importing a prefab
	MSTimer timer;
	timer.Start();
	LOG("Importing prefab from path: \"%s\".", filePath);

	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error loading prefab %s", filePath);
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

	// Create prefab resource
	unsigned resourceIndex = 0;
	std::unique_ptr<ResourcePrefab> prefab = ImporterCommon::CreateResource<ResourcePrefab>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(prefab.get());
	if (!saved) {
		LOG("Failed to save prefab resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(prefab->GetResourceFilePath().c_str(), stringBuffer.GetString(), stringBuffer.GetSize());
	if (!saved) {
		LOG("Failed to save prefab resource file.");
		return false;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(prefab);

	unsigned timeMs = timer.Stop();
	LOG("Prefab imported in %ums", timeMs);
	return true;
}

bool PrefabImporter::SavePrefab(const char* filePath, GameObject* root) {
	// Timer to measure saving a prefab
	MSTimer timer;
	timer.Start();
	LOG("Saving prefab to path: \"%s\".", filePath);

	// Create document
	rapidjson::Document document;
	document.SetObject();
	JsonValue jScene(document, document);

	// Save GameObjects
	JsonValue jRoot = jScene[JSON_TAG_ROOT];
	root->SavePrefab(jRoot);

	// Write document to buffer
	rapidjson::StringBuffer stringBuffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator, rapidjson::kWriteNanAndInfFlag> writer(stringBuffer);
	document.Accept(writer);

	// Save to file
	bool saved = App->files->Save(filePath, stringBuffer.GetString(), stringBuffer.GetSize());
	if (!saved) {
		LOG("Failed to save prefab resource.");
		return false;
	}

	unsigned timeMs = timer.Stop();
	LOG("Prefab saved in %ums", timeMs);
	return true;
}
