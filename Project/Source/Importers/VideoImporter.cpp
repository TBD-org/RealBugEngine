#include "VideoImporter.h"

#include "Application.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleFiles.h"
#include "Resources/ResourceVideo.h"
#include "Utils/Logging.h"
#include "Utils/PathUtils.h"
#include "Utils/Buffer.h"
#include "ImporterCommon.h"

bool VideoImporter::ImportVideo(const char* filePath, JsonValue jMeta) {
	// Timer to measure importing video
	MSTimer timer;
	timer.Start();

	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error loading video %s", filePath);
		return false;
	}

	// Create video resource
	 unsigned resourceIndex = 0;
	std::unique_ptr<ResourceVideo> video = ImporterCommon::CreateResource<ResourceVideo>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(video.get());
	if (!saved) {
		LOG("Failed to save video resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(video->GetResourceFilePath().c_str(), buffer);
	if (!saved) {
		LOG("Failed to save video resource file.");
		return false;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(video);
	
	unsigned timeMs = timer.Stop();
	LOG("Video imported in %ums", timeMs);
	return true;
}