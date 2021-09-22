#include "ComponentScript.h"

#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleProject.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleEditor.h"
#include "Resources/ResourceScript.h"
#include "Resources/ResourcePrefab.h"
#include "Resources/ResourceScene.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/Logging.h"

#include "Math/myassert.h"
#include "imgui_stdlib.h"

#include "Utils/Leaks.h"

#define JSON_TAG_SCRIPT "Script"
#define JSON_TAG_VALUES "Values"
#define JSON_TAG_NAME "Name"
#define JSON_TAG_TYPE "Type"
#define JSON_TAG_VALUE "Value"

ComponentScript::~ComponentScript() {
	App->resources->DecreaseReferenceCount(scriptId);

	for (auto& entry : changedValues) {
		if (entry.second.first == MemberType::PREFAB_RESOURCE_UID) {
			UID prefabId = std::get<UID>(entry.second.second);
			App->resources->DecreaseReferenceCount(prefabId);
		}
	}
}

void ComponentScript::Init() {
	App->resources->IncreaseReferenceCount(scriptId);

	for (auto& entry : changedValues) {
		if (entry.second.first == MemberType::PREFAB_RESOURCE_UID) {
			UID prefabId = std::get<UID>(entry.second.second);
			App->resources->IncreaseReferenceCount(prefabId);
		}
	}

	if (App->project->IsGameLoaded()) {
		CreateScriptInstance();
	}
}

void ComponentScript::Start() {
	if (scriptInstance) {
		scriptInstance->Start();
	}
}

void ComponentScript::OnEditorUpdate() {
	if (ImGui::Checkbox("Active", &active)) {
		if (GetOwner().IsActive()) {
			if (active) {
				Enable();
			} else {
				Disable();
			}
		}
	}
	ImGui::Separator();
	UID oldScriptId = scriptId;
	ImGui::ResourceSlot<ResourceScript>("Script", &scriptId);
	if (oldScriptId != scriptId) {
		changedValues.clear();
		ReleaseScriptInstance();
		CreateScriptInstance();
	}

	if (ImGui::Button("Create Script")) {
		ImGui::OpenPopup("Name the script");
	}

	ImGui::SetNextWindowSize(ImVec2(260, 100), ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Name the script", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
		static char name[FILENAME_MAX] = "New script";
		ImGui::InputText("Name##scriptName", name, IM_ARRAYSIZE(name));
		ImGui::NewLine();
		ImGui::SameLine(ImGui::GetWindowWidth() - 120);
		if (ImGui::Button("Save", ImVec2(50, 20))) {
			App->project->CreateScript(std::string(name));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine(ImGui::GetWindowWidth() - 60);
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (scriptInstance != nullptr) {
		const std::vector<Member>& members = scriptInstance->GetMembers();
		if (!members.empty()) {
			ImGui::Separator();
			ImGui::TextColored(App->editor->titleColor, "Members");
		}
		for (const Member& member : members) {
			switch (member.type) {
			case MemberType::BOOL: {
				bool* memberPtr = (bool*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				bool old = *memberPtr;
				ImGui::Checkbox(member.name.c_str(), memberPtr);
				if (old != *memberPtr) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::INT: {
				int* memberPtr = (int*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				int old = *memberPtr;
				ImGui::InputScalar(member.name.c_str(), ImGuiDataType_S32, memberPtr);
				if (old != *memberPtr) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::UINT: {
				unsigned* memberPtr = (unsigned*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				unsigned old = *memberPtr;
				ImGui::InputScalar(member.name.c_str(), ImGuiDataType_U32, memberPtr);
				if (old != *memberPtr) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::LONGLONG: {
				long long* memberPtr = (long long*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				long long old = *memberPtr;
				ImGui::InputScalar(member.name.c_str(), ImGuiDataType_S64, memberPtr);
				if (old != *memberPtr) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::ULONGLONG: {
				unsigned long long* memberPtr = (unsigned long long*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				unsigned long long old = *memberPtr;
				ImGui::InputScalar(member.name.c_str(), ImGuiDataType_U64, memberPtr);
				if (old != *memberPtr) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::FLOAT: {
				float* memberPtr = (float*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				float old = *memberPtr;
				ImGui::InputFloat(member.name.c_str(), memberPtr);
				if (old != *memberPtr) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::FLOAT2: {
				float2* memberPtr = (float2*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				float2 old = *memberPtr;
				ImGui::InputFloat2(member.name.c_str(), &memberPtr->x, "%.1f");
				if (!old.Equals(*memberPtr)) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::FLOAT3: {
				float3* memberPtr = (float3*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				float3 old = *memberPtr;
				ImGui::InputFloat3(member.name.c_str(), &memberPtr->x, "%.1f");
				if (!old.Equals(*memberPtr)) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::DOUBLE: {
				double* memberPtr = (double*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				double old = *memberPtr;
				ImGui::InputDouble(member.name.c_str(), memberPtr);
				if (old != *memberPtr) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::STRING: {
				std::string* memberPtr = (std::string*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				std::string old = *memberPtr;
				ImGui::InputText(member.name.c_str(), memberPtr);
				if (old != *memberPtr) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::GAME_OBJECT_UID: {
				UID* memberPtr = (UID*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				UID old = *memberPtr;
				ImGui::GameObjectSlot(member.name.c_str(), memberPtr);
				if (old != *memberPtr) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::PREFAB_RESOURCE_UID: {
				UID* memberPtr = (UID*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				UID old = *memberPtr;
				ImGui::ResourceSlot<ResourcePrefab>(member.name.c_str(), memberPtr);
				if (old != *memberPtr) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::SCENE_RESOURCE_UID: {
				UID* memberPtr = (UID*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				UID old = *memberPtr;
				ImGui::ResourceSlot<ResourceScene>(member.name.c_str(), memberPtr);
				if (old != *memberPtr) {
					changedValues[member.name] = std::pair<MemberType, MEMBER_VARIANT>(member.type, *memberPtr);
				}
				break;
			}
			case MemberType::SEPARATOR: {
				ImGui::Text("");
				ImGui::Separator();
				ImGui::TextColored(App->editor->memberSeparatorTextColor, member.name.c_str());
				ImGui::Text("");
				break;
			}
			default:
				LOG("Member of type %i hasn't been registered in ComponentScript's OnEditorUpdate function.", (unsigned) member.type);
				assert(false); // ERROR: Member type not registered
				break;
			}
		}
	}
}

void ComponentScript::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_SCRIPT] = scriptId;
	JsonValue jValues = jComponent[JSON_TAG_VALUES];
	unsigned i = 0;
	for (auto& entry : changedValues) {
		JsonValue jValue = jValues[i++];
		jValue[JSON_TAG_NAME] = entry.first.c_str();
		jValue[JSON_TAG_TYPE] = GetMemberTypeName(entry.second.first);
		switch (entry.second.first) {
		case MemberType::BOOL:
			jValue[JSON_TAG_VALUE] = std::get<bool>(entry.second.second);
			break;
		case MemberType::INT:
			jValue[JSON_TAG_VALUE] = std::get<int>(entry.second.second);
			break;
		case MemberType::UINT:
			jValue[JSON_TAG_VALUE] = std::get<unsigned>(entry.second.second);
			break;
		case MemberType::LONGLONG:
			jValue[JSON_TAG_VALUE] = std::get<long long>(entry.second.second);
			break;
		case MemberType::ULONGLONG:
			jValue[JSON_TAG_VALUE] = std::get<unsigned long long>(entry.second.second);
			break;
		case MemberType::FLOAT:
			jValue[JSON_TAG_VALUE] = std::get<float>(entry.second.second);
			break;
		case MemberType::FLOAT2: {
			float2 aFloat2 = std::get<float2>(entry.second.second);
			JsonValue float2JsonVal = jValue[JSON_TAG_VALUE];
			float2JsonVal[0] = aFloat2.x;
			float2JsonVal[1] = aFloat2.y;
			break;
		}
		case MemberType::FLOAT3: {
			float3 aFloat3 = std::get<float3>(entry.second.second);
			JsonValue float3JsonVal = jValue[JSON_TAG_VALUE];
			float3JsonVal[0] = aFloat3.x;
			float3JsonVal[1] = aFloat3.y;
			float3JsonVal[2] = aFloat3.z;
			break;
		}
		case MemberType::DOUBLE:
			jValue[JSON_TAG_VALUE] = std::get<double>(entry.second.second);
			break;
		case MemberType::STRING:
			jValue[JSON_TAG_VALUE] = std::get<std::string>(entry.second.second).c_str();
			break;
		case MemberType::GAME_OBJECT_UID:
			jValue[JSON_TAG_VALUE] = std::get<UID>(entry.second.second);
			break;
		case MemberType::PREFAB_RESOURCE_UID:
			jValue[JSON_TAG_VALUE] = std::get<UID>(entry.second.second);
			break;
		case MemberType::SCENE_RESOURCE_UID:
			jValue[JSON_TAG_VALUE] = std::get<UID>(entry.second.second);
			break;
		default:
			LOG("Member of type %i hasn't been registered in ComponentScript's Save function.", (unsigned) type);
			assert(false); // ERROR: Member type not registered
			break;
		}
	}
}

void ComponentScript::Load(JsonValue jComponent) {
	scriptId = jComponent[JSON_TAG_SCRIPT];
	changedValues.clear();
	JsonValue jValues = jComponent[JSON_TAG_VALUES];
	for (unsigned i = 0; i < jValues.Size(); ++i) {
		JsonValue jValue = jValues[i];
		std::string valueName = jValue[JSON_TAG_NAME];
		std::string typeName = jValue[JSON_TAG_TYPE];
		MemberType type = GetMemberTypeFromName(typeName.c_str());
		switch (type) {
		case MemberType::BOOL:
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, static_cast<bool>(jValue[JSON_TAG_VALUE]));
			break;
		case MemberType::INT:
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, static_cast<int>(jValue[JSON_TAG_VALUE]));
			break;
		case MemberType::UINT:
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, static_cast<unsigned>(jValue[JSON_TAG_VALUE]));
			break;
		case MemberType::LONGLONG:
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, static_cast<long long>(jValue[JSON_TAG_VALUE]));
			break;
		case MemberType::ULONGLONG:
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, static_cast<unsigned long long>(jValue[JSON_TAG_VALUE]));
			break;
		case MemberType::FLOAT:
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, static_cast<float>(jValue[JSON_TAG_VALUE]));
			break;
		case MemberType::FLOAT2: {
			JsonValue jsonVal = jValue[JSON_TAG_VALUE];
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, float2(jsonVal[0], jsonVal[1]));
			break;
		}
		case MemberType::FLOAT3: {
			JsonValue jsonVal = jValue[JSON_TAG_VALUE];
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, float3(jsonVal[0], jsonVal[1], jsonVal[2]));
			break;
		}
		case MemberType::DOUBLE:
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, static_cast<double>(jValue[JSON_TAG_VALUE]));
			break;
		case MemberType::STRING:
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, static_cast<std::string>(jValue[JSON_TAG_VALUE]));
			break;
		case MemberType::GAME_OBJECT_UID:
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, static_cast<UID>(jValue[JSON_TAG_VALUE]));
			break;
		case MemberType::PREFAB_RESOURCE_UID:
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, static_cast<UID>(jValue[JSON_TAG_VALUE]));
			break;
		case MemberType::SCENE_RESOURCE_UID:
			changedValues[valueName] = std::pair<MemberType, MEMBER_VARIANT>(type, static_cast<UID>(jValue[JSON_TAG_VALUE]));
			break;
		default:
			LOG("Member of type %i hasn't been registered in ComponentScript's Load function.", (unsigned) type);
			assert(false); // ERROR: Member type not registered
			break;
		}
	}
}

void ComponentScript::CreateScriptInstance() {
	if (scriptInstance != nullptr) return;

	const char* scriptName = App->resources->GetResourceName(scriptId);
	if (scriptName == nullptr) return;

	scriptInstance.reset(Factory::Create(scriptName, &GetOwner()));
	if (scriptInstance == nullptr) return;

	const std::vector<Member>& members = scriptInstance->GetMembers();
	for (const Member& member : members) {
		auto it = changedValues.find(member.name);
		if (it != changedValues.end()) {
			switch (member.type) {
			case MemberType::BOOL: {
				bool* memberPtr = (bool*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<bool>(it->second.second);
				break;
			}
			case MemberType::INT: {
				int* memberPtr = (int*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<int>(it->second.second);
				break;
			}
			case MemberType::UINT: {
				unsigned* memberPtr = (unsigned*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<unsigned>(it->second.second);
				break;
			}
			case MemberType::LONGLONG: {
				long long* memberPtr = (long long*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<long long>(it->second.second);
				break;
			}
			case MemberType::ULONGLONG: {
				unsigned long long* memberPtr = (unsigned long long*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<unsigned long long>(it->second.second);
				break;
			}
			case MemberType::FLOAT: {
				float* memberPtr = (float*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<float>(it->second.second);
				break;
			}
			case MemberType::FLOAT2: {
				float2* memberPtr = (float2*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<float2>(it->second.second);
				break;
			}
			case MemberType::FLOAT3: {
				float3* memberPtr = (float3*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<float3>(it->second.second);
				break;
			}
			case MemberType::DOUBLE: {
				double* memberPtr = (double*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<double>(it->second.second);
				break;
			}
			case MemberType::STRING: {
				std::string* memberPtr = (std::string*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<std::string>(it->second.second);
				break;
			}
			case MemberType::GAME_OBJECT_UID: {
				UID* memberPtr = (UID*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<UID>(it->second.second);
				break;
			}
			case MemberType::PREFAB_RESOURCE_UID: {
				UID* memberPtr = (UID*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<UID>(it->second.second);
				break;
			}
			case MemberType::SCENE_RESOURCE_UID: {
				UID* memberPtr = (UID*) GET_OFFSET_MEMBER(scriptInstance.get(), member.offset);
				*memberPtr = std::get<UID>(it->second.second);
				break;
			}
			default:
				LOG("Member of type %i hasn't been registered in ComponentScript's ReloadScriptInstance function.", (unsigned) member.type);
				assert(false); // ERROR: Member type not registered
				break;
			}
		}
	}
}

void ComponentScript::ReleaseScriptInstance() {
	scriptInstance.reset();
}

Script* ComponentScript::GetScriptInstance() const {
	return scriptInstance.get();
}

const char* ComponentScript::GetScriptName() const {
	return App->resources->GetResourceName(scriptId);
}
