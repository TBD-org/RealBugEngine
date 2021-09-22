#include "ResourceClip.h"

#include "Application.h"

#include "Modules/ModuleFiles.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleEditor.h"
#include "Utils/JsonValue.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/UID.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"

#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"
#include "imgui.h"

#include "Utils/Leaks.h"

#define JSON_TAG_CLIP_ID "ClipId"

#define JSON_TAG_NAME "Name"
#define JSON_TAG_ANIMATION_UID "AnimationUID"
#define JSON_TAG_BEGIN_INDEX "BeginIndex"
#define JSON_TAG_END_INDEX "EndIndex"
#define JSON_TAG_CLIP_UID "ClipUID"
#define JSON_TAG_LOOP "Loop"
#define JSON_TAG_SPEED "Speed"
#define JSON_TAG_ID "Id"
#define JSON_TAG_FRAMERATE "FrameRate"
#define JSON_TAG_KEY_FRAME "KeyFrame"
#define JSON_TAG_KEY_EVENT_CLIP "KeyEventClip"

void ResourceClip::Load() {
	// Timer to measure loading a clip
	MSTimer timer;
	timer.Start();
	std::string filePath = GetResourceFilePath();
	LOG("Loading ResourceClip from path: \"%s\".", filePath.c_str());

	Buffer<char> buffer = App->files->Load(filePath.c_str());
	if (buffer.Size() == 0) return;

	rapidjson::Document document;
	document.ParseInsitu<rapidjson::kParseNanAndInfFlag>(buffer.Data());
	if (document.HasParseError()) {
		LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document.GetParseError()), document.GetErrorOffset());
		return;
	}
	JsonValue jStateMachine(document, document);

	name = jStateMachine[JSON_TAG_NAME];
	animationUID = jStateMachine[JSON_TAG_ANIMATION_UID];
	endIndex = jStateMachine[JSON_TAG_END_INDEX];
	beginIndex = jStateMachine[JSON_TAG_BEGIN_INDEX];
	loop = jStateMachine[JSON_TAG_LOOP];
	speed = jStateMachine[JSON_TAG_SPEED];
	frameRate = jStateMachine[JSON_TAG_FRAMERATE];
	if (frameRate <= FLT_MIN) {
		frameRate = 1;
		LOG("WARNING frameRate missing");
	}

	JsonValue keyEventClipsJson = jStateMachine[JSON_TAG_KEY_EVENT_CLIP];
	for (unsigned int i = 0; i < keyEventClipsJson.Size(); ++i) {
		unsigned int keyframe = keyEventClipsJson[i][JSON_TAG_KEY_FRAME];
		std::string name = keyEventClipsJson[i][JSON_TAG_NAME];
		keyEventClips.insert(std::make_pair(keyframe, EventClip {false, name}));
	}

	Init(name, animationUID, beginIndex, endIndex, loop, speed, 0);

	unsigned timeMs = timer.Stop();
	LOG("Clip loaded in %ums", timeMs);
}

void ResourceClip::GetInfoJson() {
	// Timer to measure getting info from a clip
	MSTimer timer;
	timer.Start();
	std::string filePath = GetResourceFilePath();
	LOG("Getting info of ResourceClip from path: \"%s\".", filePath.c_str());

	Buffer<char> buffer = App->files->Load(filePath.c_str());
	if (buffer.Size() == 0) return;

	rapidjson::Document document;
	document.ParseInsitu<rapidjson::kParseNanAndInfFlag>(buffer.Data());
	if (document.HasParseError()) {
		LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document.GetParseError()), document.GetErrorOffset());
		return;
	}
	JsonValue jStateMachine(document, document);

	name = jStateMachine[JSON_TAG_NAME];
	animationUID = jStateMachine[JSON_TAG_ANIMATION_UID];
	endIndex = jStateMachine[JSON_TAG_END_INDEX];
	beginIndex = jStateMachine[JSON_TAG_BEGIN_INDEX];
	loop = jStateMachine[JSON_TAG_LOOP];
	speed = jStateMachine[JSON_TAG_SPEED];
	frameRate = jStateMachine[JSON_TAG_FRAMERATE];

	JsonValue keyEventClipsJson = jStateMachine[JSON_TAG_KEY_EVENT_CLIP];
	for (unsigned int i = 0; i < keyEventClipsJson.Size(); ++i) {
		unsigned int keyframe = keyEventClipsJson[i][JSON_TAG_KEY_FRAME];
		std::string name = keyEventClipsJson[i][JSON_TAG_NAME];
		keyEventClips.insert(std::make_pair(keyframe, EventClip {false, name}));
	}

	unsigned timeMs = timer.Stop();
	LOG("Clip info received in %ums", timeMs);
}

void ResourceClip::FinishLoading() {
	App->resources->IncreaseReferenceCount(animationUID);
}

void ResourceClip::Unload() {
	App->resources->DecreaseReferenceCount(animationUID);

	animationUID = 0;
}

void ResourceClip::OnEditorUpdate() {
	ImGui::TextColored(App->editor->titleColor, "Clip");
	ImGui::SameLine();
	if (ImGui::Button("Get info from Json##clip")) {
		GetInfoJson();
	}

	char nameClip[100];
	sprintf_s(nameClip, 100, "%s", name.c_str());
	if (ImGui::InputText("Clip Name##clip_name", nameClip, 100)) {
		name = nameClip;
	}

	ImGui::ResourceSlot<ResourceAnimation>("Animaton", &animationUID);

	int maxFrames = 0;
	ResourceAnimation* resourceAnimation = App->resources->GetResource<ResourceAnimation>(animationUID);
	if (resourceAnimation != nullptr && resourceAnimation->keyFrames.size() != 0) {
		maxFrames = resourceAnimation->keyFrames.size();
	}

	ImGui::Checkbox("Loop", &loop);

	ImGui::DragScalar("Begin Index", ImGuiDataType_U32, &beginIndex, 1, 0, &endIndex);
	SetBeginIndex(beginIndex);

	ImGui::DragScalar("End Index", ImGuiDataType_U32, &endIndex, 1, &beginIndex, &maxFrames);
	SetEndIndex(endIndex);

	ImGui::DragFloat("Speed", &speed, 0.001f);
	ImGui::NewLine();

	ImGui::TextColored(App->editor->textColor, "New key frame event");
	ImGui::DragScalar("KeyFrame", ImGuiDataType_U32, &newKeyFrameEditor);

	char newNameEvent[100];
	sprintf_s(newNameEvent, 100, "%s", newNameEditor.c_str());
	if (ImGui::InputText("##newEventClip_name", newNameEvent, 100)) {
		newNameEditor = newNameEvent;
	}

	if (ImGui::Button("Add")) {
		keyEventClips.insert(std::make_pair(newKeyFrameEditor, EventClip {false, newNameEditor}));
		newKeyFrameEditor = 0;
		newNameEditor = "NewName";
	}

	for (auto& element : keyEventClips) {
		ImGui::TextColored(App->editor->textColor, "KeyFrame %d", element.first);

		char nameEvent[100];
		sprintf_s(nameEvent, 100, "%s", element.second.name.c_str());

		char label[100];
		std::string labelString = "##eventClip_name" + std::to_string(element.first);
		sprintf_s(label, 100, "%s", labelString.c_str());

		if (ImGui::InputText(label, nameEvent, 100)) {
			element.second.name = nameEvent;
		}

		std::string buttonDelete = "Delete key frame: " + std::to_string(element.first);
		if (ImGui::Button(buttonDelete.c_str())) {
			keyToDelete = element.first;
		}
	}
	if (keyToDelete != -1) {
		keyEventClips.erase(keyToDelete);
		keyToDelete = -1;
	}

	ImGui::NewLine();
	if (ImGui::Button("Save##clip")) {
		SaveToFile(GetAssetFilePath().c_str());
	}
}

bool ResourceClip::SaveToFile(const char* filePath) {
	LOG("Saving ResourceClip to path: \"%s\".", filePath);

	MSTimer timer;
	timer.Start();

	// Create document
	rapidjson::Document document;
	JsonValue jStateMachine(document, document);

	jStateMachine[JSON_TAG_NAME] = name.c_str();
	jStateMachine[JSON_TAG_ANIMATION_UID] = animationUID;
	jStateMachine[JSON_TAG_BEGIN_INDEX] = beginIndex;
	jStateMachine[JSON_TAG_END_INDEX] = endIndex;
	jStateMachine[JSON_TAG_LOOP] = loop;
	jStateMachine[JSON_TAG_SPEED] = speed;
	jStateMachine[JSON_TAG_FRAMERATE] = frameRate;

	JsonValue keyEventClipArray = jStateMachine[JSON_TAG_KEY_EVENT_CLIP];
	int i = 0;
	for (const auto& element : keyEventClips) {
		keyEventClipArray[i][JSON_TAG_KEY_FRAME] = element.first;
		keyEventClipArray[i][JSON_TAG_NAME] = element.second.name.c_str();
		++i;
	}

	// Write document to buffer
	rapidjson::StringBuffer stringBuffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator, rapidjson::kWriteNanAndInfFlag> writer(stringBuffer);
	document.Accept(writer);

	// Save to file
	bool saved = App->files->Save(filePath, stringBuffer.GetString(), stringBuffer.GetSize());
	if (!saved) {
		LOG("Failed to save clip resource.");
		return false;
	}

	unsigned timeMs = timer.Stop();
	LOG("Clip saved in %ums", timeMs);
	return true;
}

void ResourceClip::Init(std::string& mName, UID mAnimationUID, unsigned int mBeginIndex, unsigned int mEndIndex, bool mLoop, float mSpeed, UID mid) {
	name = mName;
	animationUID = mAnimationUID;
	loop = mLoop;
	speed = mSpeed;
	if (animationUID == 0) {
		return;
	}

	SetEndIndex(mEndIndex);
	SetBeginIndex(mBeginIndex);
}

void ResourceClip::SetBeginIndex(unsigned int index) {
	if (endIndex >= index) {
		beginIndex = index;
		keyFramesSize = endIndex - beginIndex;
		if (keyFramesSize == 0) {
			duration = 1;
		} else {
			duration = keyFramesSize * frameRate;
		}
	}
}

void ResourceClip::SetEndIndex(unsigned int index) {
	if (index >= beginIndex) {
		endIndex = index;
		keyFramesSize = endIndex - beginIndex;
		if (keyFramesSize == 0) {
			duration = 1;
		} else {
			duration = keyFramesSize * frameRate;
		}
	}
}