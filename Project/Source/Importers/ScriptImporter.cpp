#include "ScriptImporter.h"

#include "Globals.h"
#include "Application.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleFiles.h"
#include "Resources/ResourceScript.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
#include "Utils/MSTimer.h"
#include "Utils/PathUtils.h"
#include "ImporterCommon.h"

#include "Utils/Leaks.h"

bool ScriptImporter::ImportScript(const char* filePath, JsonValue jMeta) {
	LOG("Importing Script from path: \"%s\".", filePath);

	// Timer to measure importing a material
	MSTimer timer;
	timer.Start();

	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error loading script %s", filePath);
		return false;
	}

	// Create script resource
	unsigned resourceIndex = 0;
	std::unique_ptr<ResourceScript> script = ImporterCommon::CreateResource<ResourceScript>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(script.get());
	if (!saved) {
		LOG("Failed to save script resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(script->GetResourceFilePath().c_str(), buffer);
	if (!saved) {
		LOG("Failed to save script resource file.");
		return false;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(script);

	unsigned timeMs = timer.Stop();
	LOG("Script imported in %ums", timeMs);
	return true;
}