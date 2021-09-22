#include "FontImporter.h"

#include "Application.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleResources.h"
#include "Resources/ResourceFont.h"
#include "Utils/MSTimer.h"
#include "Utils/Buffer.h"
#include "Utils/Logging.h"
#include "Utils/PathUtils.h"
#include "ImporterCommon.h"

#include "Utils/Leaks.h"

bool FontImporter::ImportFont(const char* filePath, JsonValue jMeta) {
	LOG("Importing font from path: \"%s\".", filePath);

	// Timer to measure importing a texture
	MSTimer timer;
	timer.Start();

	// Read from file
	Buffer<char> buffer;
	buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error loading font %s", filePath);
		return false;
	}

	// Create font resource
	unsigned resourceIndex = 0;
	std::unique_ptr<ResourceFont> font = ImporterCommon::CreateResource<ResourceFont>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(font.get());
	if (!saved) {
		LOG("Failed to save font resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(font->GetResourceFilePath().c_str(), buffer);
	if (!saved) {
		LOG("Failed to save font resource file.");
		return false;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(font);

	unsigned timeMs = timer.Stop();
	LOG("Font imported in %ums", timeMs);
	return true;
}