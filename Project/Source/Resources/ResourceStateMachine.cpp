#include "ResourceStateMachine.h"
#include "Application.h"
#include "Transition.h"
#include "State.h"

#include "Resources/ResourceClip.h"
#include "Modules/ModuleFiles.h"
#include "FileSystem/JsonValue.h"
#include "FileSystem/StateMachinGenerator.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleEditor.h"

#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include "imgui.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
#include "Utils/Leaks.h"

#define JSON_TAG_CLIPS "Clips"
#define JSON_TAG_STATES "States"
#define JSON_TAG_INITIAL_STATE "InitialState"
#define JSON_TAG_TRANSITIONS "Transitions"

#define JSON_TAG_CLIP_ID "ClipId"

#define JSON_TAG_NAME "Name"
#define JSON_TAG_ID "Id"
#define JSON_TAG_BONES "Bones"

#define JSON_TAG_SOURCE "Source"
#define JSON_TAG_TARGET "Target"
#define JSON_TAG_TRIGGER_NAME "Trigger"
#define JSON_TAG_INTERPOLATION_DURATION "Interpolation"

void ResourceStateMachine::Load() {
	// Timer to measure loading a material
	MSTimer timer;
	timer.Start();
	std::string filePath = GetResourceFilePath();
	LOG("Loading material from path: \"%s\".", filePath.c_str());

	Buffer<char> buffer = App->files->Load(filePath.c_str());
	if (buffer.Size() == 0) return;

	rapidjson::Document document;
	document.ParseInsitu<rapidjson::kParseNanAndInfFlag>(buffer.Data());
	if (document.HasParseError()) {
		LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document.GetParseError()), document.GetErrorOffset());
		return;
	}
	JsonValue jStateMachine(document, document);
	assert(document.IsObject());
	assert(document.HasMember(JSON_TAG_INITIAL_STATE));

	JsonValue clipArray = jStateMachine[JSON_TAG_CLIPS];
	for (unsigned int i = 0; i < clipArray.Size(); ++i) {
		UID clipUID = clipArray[i];
		ResourceClip* clip = App->resources->GetResource<ResourceClip>(clipUID);
		App->resources->IncreaseReferenceCount(clipUID);
		clipsUids.push_back(clipUID);
	}

	JsonValue bonesArray = jStateMachine[JSON_TAG_BONES];
	for (unsigned int i = 0; i < bonesArray.Size(); ++i) {
		std::string name = bonesArray[i];
		bones.insert(name);
	}

	initialState = State();
	UID initialStateId = jStateMachine[JSON_TAG_INITIAL_STATE];
	JsonValue stateArray = jStateMachine[JSON_TAG_STATES];
	for (unsigned int i = 0; i < stateArray.Size(); ++i) {
		UID id = stateArray[i][JSON_TAG_ID];
		std::string name = stateArray[i][JSON_TAG_NAME];
		UID clipId = stateArray[i][JSON_TAG_CLIP_ID];
		State state(name, clipId, id);
		states.insert(std::make_pair(id, state));

		//Setting initial state
		if (initialStateId != 0 && initialStateId == id) {
			SetInitialState(state);
		}
	}
	states.insert(std::make_pair(0, State())); // create state "empty" for clean secondary State Machin

	JsonValue transitionArray = jStateMachine[JSON_TAG_TRANSITIONS];
	for (unsigned int i = 0; i < transitionArray.Size(); ++i) {
		std::string triggerName = transitionArray[i][JSON_TAG_TRIGGER_NAME];
		UID id = transitionArray[i][JSON_TAG_ID];
		UID source = transitionArray[i][JSON_TAG_SOURCE];
		UID target = transitionArray[i][JSON_TAG_TARGET];
		float interpolationDuration = transitionArray[i][JSON_TAG_INTERPOLATION_DURATION];
		Transition transition(states.find(source)->second, states.find(target)->second, interpolationDuration, id);
		transitions.insert(std::make_pair(triggerName, transition));
	}

	unsigned timeMs = timer.Stop();
	LOG("Material loaded in %ums", timeMs);
}

void ResourceStateMachine::Unload() {
	std::list<UID>::iterator itClip;
	for (itClip = clipsUids.begin(); itClip != clipsUids.end(); ++itClip) {
		App->resources->DecreaseReferenceCount((*itClip));
	}
}

void ResourceStateMachine::SaveToFile(const char* filePath) {
	LOG("Saving ResourceStateMachine to path: \"%s\".", filePath);

	MSTimer timer;
	timer.Start();

	// Create document
	rapidjson::Document document;
	JsonValue jStateMachine(document, document);

	// Save JSON values
	document.SetObject();

	//Saving initial state
	jStateMachine[JSON_TAG_INITIAL_STATE] = initialState.id;

	// Saving Clips UIDs
	JsonValue clipArray = jStateMachine[JSON_TAG_CLIPS];
	int i = 0;
	std::list<UID>::iterator itClip;
	for (itClip = clipsUids.begin(); itClip != clipsUids.end(); ++itClip) {
		clipArray[i] = (*itClip);
		++i;
	}

	// Saving Bones Strings
	JsonValue bonesArray = jStateMachine[JSON_TAG_BONES];
	i = 0;
	std::set<std::string>::iterator bone;
	for (bone = bones.begin(); bone != bones.end(); ++bone) {
		bonesArray[i] = (*bone->c_str());
		++i;
	}

	// Saving States
	JsonValue stateArray = jStateMachine[JSON_TAG_STATES];
	i = 0;
	for (const auto& element : states) {
		stateArray[i][JSON_TAG_ID] = element.second.id;
		stateArray[i][JSON_TAG_NAME] = element.second.name.c_str();
		stateArray[i][JSON_TAG_CLIP_ID] = element.second.clipUid;
		++i;
	}

	//Saving transitions
	JsonValue transitionArray = jStateMachine[JSON_TAG_TRANSITIONS];
	i = 0;
	for (const auto& element : transitions) {
		transitionArray[i][JSON_TAG_TRIGGER_NAME] = element.first.c_str();
		transitionArray[i][JSON_TAG_ID] = element.second.id;
		transitionArray[i][JSON_TAG_SOURCE] = element.second.source.id;
		transitionArray[i][JSON_TAG_TARGET] = element.second.target.id;
		transitionArray[i][JSON_TAG_INTERPOLATION_DURATION] = element.second.interpolationDuration;
		++i;
	}

	// Write document to buffer
	rapidjson::StringBuffer stringBuffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator, rapidjson::kWriteNanAndInfFlag> writer(stringBuffer);
	document.Accept(writer);

	// Save to file
	bool saved = App->files->Save(filePath, stringBuffer.GetString(), stringBuffer.GetSize());
	if (!saved) {
		LOG("Failed to save material resource.");
		return;
	}

	unsigned timeMs = timer.Stop();
	LOG("Material saved in %ums", timeMs);
}

void ResourceStateMachine::OnEditorUpdate() {
	ImGui::TextColored(App->editor->titleColor, "Resource State Machine");

	if (ImGui::Button("Generate JSON State Machin##StateMacghin")) {
		LOG("Generate JSON State Machin");
		std::string filePath = GetAssetFilePath();
		StateMachineGenerator::GenerateStateMachine(filePath.c_str());
	}
	/*
	char nameStateMachine[100];
	sprintf_s(nameClip, 100, "%s", name.c_str());
	if (ImGui::InputText("##clip_name", nameClip, 100)) {
		name = nameClip;
	}*/
}

State ResourceStateMachine::AddState(const std::string& name, UID clipUID) {
	State state(name, clipUID);
	states.insert(std::make_pair(state.id, state));

	AddClip(clipUID);

	return state;
}

void ResourceStateMachine::AddClip(UID clipUid) {
	if (std::find(clipsUids.begin(), clipsUids.end(), clipUid) == clipsUids.end()) {
		clipsUids.push_back(clipUid);
	}
}

void ResourceStateMachine::AddTransition(const State& from, const State& to, const float interpolation, const std::string& name) {
	//Checking for unique name
	Transition* transition = FindTransitionGivenName(name);
	if (transition) {
		return;
	}

	transitions.insert(std::make_pair(name, Transition(from, to, interpolation)));
}

Transition* ResourceStateMachine::FindTransitionGivenName(const std::string& name) {
	if (transitions.empty()) {
		return nullptr;
	}

	std::unordered_map<std::string, Transition>::iterator it = transitions.find(name);
	if (it != transitions.end()) {
		return &(*it).second;
	}

	return nullptr;
}
