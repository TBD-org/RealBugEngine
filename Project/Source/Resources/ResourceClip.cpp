#include "ResourceClip.h"

#include "Application.h"

#include "FileSystem/JsonValue.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleEditor.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/UID.h"

#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include "imgui.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
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
	App->resources->IncreaseReferenceCount(animationUID);
	endIndex = jStateMachine[JSON_TAG_END_INDEX];
	beginIndex = jStateMachine[JSON_TAG_BEGIN_INDEX];
	loop = jStateMachine[JSON_TAG_LOOP];
	speed = jStateMachine[JSON_TAG_SPEED];

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

	unsigned timeMs = timer.Stop();
	LOG("Clip info received in %ums", timeMs);
}

void ResourceClip::Unload() {
	App->resources->DecreaseReferenceCount(animationUID);
}

void ResourceClip::OnEditorUpdate() {	
	ImGui::TextColored(App->editor->titleColor, "Clip");
	ImGui::SameLine();
	if (ImGui::Button("Get info from Json##clip")) {
		GetInfoJson();
	}

	char nameClip[100];
	sprintf_s(nameClip, 100, "%s", name.c_str());
	if (ImGui::InputText("##clip_name", nameClip, 100)) {
		name = nameClip;
	}	

	ImGui::ResourceSlot<ResourceAnimation>("Animaton", &animationUID);

	ImGui::Checkbox("Loop", &loop);

	ImGui::DragScalar("Begin Index", ImGuiDataType_U32, &beginIndex);
	SetBeginIndex(beginIndex);

	ImGui::DragScalar("End Index", ImGuiDataType_U32, &endIndex);
	SetEndIndex(endIndex);

	ImGui::DragFloat("Speed", &speed, 0.001f);

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

	ResourceAnimation* animationResource = GetResourceAnimation();
	App->resources->IncreaseReferenceCount(mAnimationUID);
	SetEndIndex(mEndIndex);
	SetBeginIndex(mBeginIndex);
}

void ResourceClip::SetBeginIndex(unsigned int index) {
	ResourceAnimation* animationResource = GetResourceAnimation();
	if (!animationResource) {
		return;
	}
	if (endIndex >= index) {
		beginIndex = index;
		keyFramesSize = endIndex - beginIndex;
		if (keyFramesSize == 0) {
			duration = 1;
		} else {
			duration = keyFramesSize * animationResource->duration / animationResource->keyFrames.size();
		}
	}
}

void ResourceClip::SetEndIndex(unsigned int index) {
	ResourceAnimation* animationResource = GetResourceAnimation();
	if (!animationResource) {
		return;
	}

	if (index >= beginIndex) {
		endIndex = index;
		keyFramesSize = endIndex - beginIndex;
		if (keyFramesSize == 0) {
			duration = 1;
		} else {
			duration = keyFramesSize * animationResource->duration / animationResource->keyFrames.size();
		}
	}
}

ResourceAnimation* ResourceClip::GetResourceAnimation() const {
	return App->resources->GetResource<ResourceAnimation>(animationUID);
}