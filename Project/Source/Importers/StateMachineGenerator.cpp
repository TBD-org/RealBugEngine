#include "StateMachineGenerator.h"

#include "Application.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleTime.h"
#include "Utils/PathUtils.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
#include "Utils/JsonValue.h"

#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include "Utils/Leaks.h"

#define JSON_TAG_ID "Id"
#define JSON_TAG_RESOURCES "Resources"

#define JSON_TAG_CLIPS "Clips"
#define JSON_TAG_STATES "States"
#define JSON_TAG_INITIAL_STATE "InitialState"
#define JSON_TAG_TRANSITIONS "Transitions"

#define JSON_TAG_CLIP_ID "ClipId"

#define JSON_TAG_NAME "Name"
#define JSON_TAG_BONES "Bones"

#define JSON_TAG_SOURCE "Source"
#define JSON_TAG_TARGET "Target"
#define JSON_TAG_TRIGGER_NAME "Trigger"
#define JSON_TAG_INTERPOLATION_DURATION "Interpolation"

bool StateMachineGenerator::GenerateStateMachine(const char* filePath) {
	std::string filePath2 = PathUtils::GetFileFolder(filePath);

	std::vector<std::string> files = App->files->GetFilesInFolder(filePath2.c_str());
	if (files.empty()) {
		return false;
	}

	std::unordered_map<UID, std::string> listClips;
	for (std::string file : files) {
		std::string clipMeta = PathUtils::GetFileExtension(PathUtils::GetFileName(file.c_str()).c_str()) + PathUtils::GetFileExtension(file.c_str());
		if (clipMeta == ".clip.meta") {
			std::string pathCipMeta = filePath2 + "/" + file;
			Buffer<char> buffer = App->files->Load(pathCipMeta.c_str());
			if (buffer.Size() == 0) return false;

			rapidjson::Document document;
			document.ParseInsitu<rapidjson::kParseNanAndInfFlag>(buffer.Data());
			if (document.HasParseError()) {
				LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document.GetParseError()), document.GetErrorOffset());
				return false;
			}
			JsonValue jStateMachine(document, document);

			UID clipUid = jStateMachine[JSON_TAG_RESOURCES][0][JSON_TAG_ID];
			std::string name = PathUtils::GetFileName(PathUtils::GetFileName(pathCipMeta.c_str()).c_str());
			LOG("GenerateStateMachine; name: %s, UID: %lld", name.c_str(), clipUid);
			listClips.insert(std::make_pair(clipUid, name));
		}
	}

	SaveToFile(filePath, listClips);

	return true;
}

void StateMachineGenerator::SaveToFile(const char* filePath, std::unordered_map<UID, std::string>& listClips) {
	LOG("Saving ResourceStateMachine to path: \"%s\".", filePath);

	MSTimer timer;
	timer.Start();

	// Create document
	rapidjson::Document document;
	JsonValue jStateMachine(document, document);

	// Save JSON values
	document.SetObject();

	jStateMachine[JSON_TAG_INITIAL_STATE] = 0;

	// Saving Clips UIDs
	JsonValue clipArray = jStateMachine[JSON_TAG_CLIPS];
	int i = 0;
	std::unordered_map<UID, std::string>::iterator itClip;
	for (itClip = listClips.begin(); itClip != listClips.end(); ++itClip) {
		clipArray[i] = (*itClip).first;
		++i;
	}

	// Saving States
	JsonValue stateArray = jStateMachine[JSON_TAG_STATES];
	std::unordered_map<UID, std::string> states;
	i = 0;
	for (const auto& element : listClips) {
		UID idState = GenerateUID();
		stateArray[i][JSON_TAG_ID] = idState;
		stateArray[i][JSON_TAG_NAME] = element.second.c_str();
		stateArray[i][JSON_TAG_CLIP_ID] = element.first;

		states.insert(std::make_pair(idState, element.second));

		//Saving initial state
		if (i == 0) {
			jStateMachine[JSON_TAG_INITIAL_STATE] = idState;
		}

		++i;
	}

	//Saving transitions
	JsonValue transitionArray = jStateMachine[JSON_TAG_TRANSITIONS];
	i = 0;
	for (const auto& source : states) {
		for (const auto& target : states) {
			if (source == target) {
				continue;
			}
			transitionArray[i][JSON_TAG_TRIGGER_NAME] = (source.second + target.second).c_str();
			transitionArray[i][JSON_TAG_ID] = GenerateUID();
			transitionArray[i][JSON_TAG_SOURCE] = source.first;
			transitionArray[i][JSON_TAG_TARGET] = target.first;
			transitionArray[i][JSON_TAG_INTERPOLATION_DURATION] = 0.3;
			++i;
		}
	}

	// Saving Bones Strings
	JsonValue bonesArray = jStateMachine[JSON_TAG_BONES];
	bonesArray[0] = "FEED_ME_WITH_BONES_IAM_STARVING";

	// Write document to buffer
	rapidjson::StringBuffer stringBuffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator, rapidjson::kWriteNanAndInfFlag> writer(stringBuffer);
	document.Accept(writer);

	// Save to file
	bool saved = App->files->Save(filePath, stringBuffer.GetString(), stringBuffer.GetSize());

	// Save Second State Machin
	std::string nameSecondary = PathUtils::GetFileName(filePath) + "Secondary";
	std::string filePathSecondary = PathUtils::GetFileFolder(filePath) + "/" + nameSecondary + ".stma";
	saved &= App->files->Save(filePathSecondary.c_str(), stringBuffer.GetString(), stringBuffer.GetSize());

	if (!saved) {
		LOG("Failed to save state machine resource.");
		return;
	}

	unsigned timeMs = timer.Stop();
	LOG("State machine saved in %ums", timeMs);
}
