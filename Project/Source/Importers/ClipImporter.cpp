#include "ClipImporter.h"

#include "Application.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleFiles.h"
#include "Resources/Resourceclip.h"
#include "Utils/Logging.h"
#include "Utils/MSTimer.h"
#include "Utils/Buffer.h"
#include "Utils/PathUtils.h"
#include "ImporterCommon.h"

#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include "Utils/Leaks.h"

bool ClipImporter::ImportClip(const char* filePath, JsonValue jMeta) {
	LOG("Importing clip from path: \"%s\".", filePath);

	// Timer to measure importing a Clip
	MSTimer timer;
	timer.Start();

	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error loading clip %s", filePath);
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

	// Create clip resource
	unsigned resourceIndex = 0;
	std::unique_ptr<ResourceClip> clip = ImporterCommon::CreateResource<ResourceClip>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(clip.get());
	if (!saved) {
		LOG("Failed to save clip resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(clip->GetResourceFilePath().c_str(), stringBuffer.GetString(), stringBuffer.GetSize());
	if (!saved) {
		LOG("Failed to save clip resource file.");
		return false;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(clip);

	unsigned timeMs = timer.Stop();
	LOG("Clip imported in %ums", timeMs);
	return true;
}
