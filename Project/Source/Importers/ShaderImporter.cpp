#include "ShaderImporter.h"

#include "Resources/ResourceShader.h"
#include "Globals.h"
#include "Application.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
#include "Utils/PathUtils.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleTime.h"
#include "ImporterCommon.h"

#include "Utils/Leaks.h"

bool ShaderImporter::ImportShader(const char* filePath, JsonValue jMeta) {
	LOG("Importing Shader from path: \"%s\".", filePath);

	// Timer to measure importing a shader
	MSTimer timer;
	timer.Start();

	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error loading shader %s", filePath);
		return false;
	}

	// Create shader resource
	unsigned resourceIndex = 0;
	std::unique_ptr<ResourceShader> shader = ImporterCommon::CreateResource<ResourceShader>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(shader.get());
	if (!saved) {
		LOG("Failed to save shader resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(shader->GetResourceFilePath().c_str(), buffer);
	if (!saved) {
		LOG("Failed to save shader resource file.");
		return false;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(shader);

	unsigned timeMs = timer.Stop();
	LOG("Shader imported in %ums", timeMs);
	return true;
}
