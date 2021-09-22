#include "SkyboxImporter.h"

#include "Application.h"
#include "Resources/ResourceSkybox.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleFiles.h"
#include "Globals.h"
#include "Utils/Logging.h"
#include "Utils/MSTimer.h"
#include "Utils/Buffer.h"
#include "Utils/PathUtils.h"
#include "ImporterCommon.h"

#include "IL/il.h"
#include "IL/ilu.h"
#include "GL/glew.h"

#include "Utils/Leaks.h"

bool SkyboxImporter::ImportSkybox(const char* filePath, JsonValue jMeta) {
	LOG("Importing skybox from path: \"%s\".", filePath);

	MSTimer timer;
	timer.Start();

	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error loading skybox %s", filePath);
		return false;
	}

	// Create skybox resource
	unsigned resourceIndex = 0;
	std::unique_ptr<ResourceSkybox> skybox = ImporterCommon::CreateResource<ResourceSkybox>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(skybox.get());
	if (!saved) {
		LOG("Failed to save skybox resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(skybox->GetResourceFilePath().c_str(), buffer);
	if (!saved) {
		LOG("Failed to save skybox resource file.");
		return false;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(skybox);

	unsigned timeMs = timer.Stop();
	LOG("skybox imported in %ums", timeMs);
	return true;
}