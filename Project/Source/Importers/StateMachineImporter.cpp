#include "StateMachineImporter.h"

#include "Utils/Logging.h"
#include "Utils/MSTimer.h"
#include "Utils/Buffer.h"
#include "Utils/PathUtils.h"
#include "Application.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleTime.h"
#include "Resources/ResourceStateMachine.h"
#include "ImporterCommon.h"

#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include "Utils/Leaks.h"

bool StateMachineImporter::ImportStateMachine(const char* filePath, JsonValue jMeta) {
	LOG("Importing sate machine from path: \"%s\".", filePath);

	// Timer to measure importing a State Machine
	MSTimer timer;
	timer.Start();

	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error loading state machine %s", filePath);
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

	// Create state machine resource
	unsigned resourceIndex = 0;
	std::unique_ptr<ResourceStateMachine> stateMachine = ImporterCommon::CreateResource<ResourceStateMachine>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(stateMachine.get());
	if (!saved) {
		LOG("Failed to save state machine resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(stateMachine->GetResourceFilePath().c_str(), stringBuffer.GetString(), stringBuffer.GetSize());
	if (!saved) {
		LOG("Failed to save state machine resource file.");
		return false;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(stateMachine);

	unsigned timeMs = timer.Stop();
	LOG("StateMachine imported in %ums", timeMs);
	return true;
}
