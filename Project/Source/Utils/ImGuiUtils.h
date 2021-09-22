#pragma once

#include "Application.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleEditor.h"
#include "Resources/ResourceTexture.h"
#include "Resources/ResourceScene.h"

#include "imgui.h"
#include "GL/glew.h"
#include "IconsFontAwesome5.h"
#include <functional>

namespace ImGui {
	template<typename T> void ResourceSlot(
		const char* label,
		UID* target,
		std::function<void()> oldCallBack = []() {},
		std::function<void()> newCallBack = []() {});
	void GameObjectSlot(const char* label, UID* target);
} // namespace ImGui

template<typename T>
inline void ImGui::ResourceSlot(const char* label, UID* target, std::function<void()> oldCallBack, std::function<void()> newCallBack) {
	ImGui::Text(label);

	if (ImGui::BeginTable(label, 2)) {
		ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, 32);
		ImGui::TableNextColumn();

		ImGui::BeginChildFrame(ImGui::GetID(target), ImVec2(32, 32));
		ImGui::EndChildFrame();

		if (ImGui::BeginDragDropTarget()) {
			std::string payloadType = std::string("_RESOURCE_") + GetResourceTypeName(T::staticType);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str())) {
				UID newUID = *(UID*) payload->Data;
				UID oldUID = *target;
				if (oldUID != newUID) {
					if (oldUID != 0) {
						oldCallBack();
						App->resources->DecreaseReferenceCount(oldUID);
					}
					*target = newUID;
					App->resources->IncreaseReferenceCount(newUID);
					newCallBack();
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::TableNextColumn();

		std::string resourceName = "None";
		T* resource = App->resources->GetResource<T>(*target);
		if (resource != nullptr) {
			resourceName = resource->GetName();
		}

		std::string resourceNameLabel = std::string("Name: ") + resourceName;
		ImGui::Text(resourceNameLabel.c_str());

		std::string removeButton = std::string(ICON_FA_TIMES "##") + label;
		if (ImGui::Button(removeButton.c_str())) {
			if (*target != 0) {
				App->resources->DecreaseReferenceCount(*target);
				*target = 0;
			}
		}
		ImGui::SameLine();
		ImGui::TextUnformatted("Id:");
		ImGui::SameLine();
		ImGui::TextColored(App->editor->textColor, "%llu", *target);

		ImGui::EndTable();
	}
}

template<>
inline void ImGui::ResourceSlot<ResourceTexture>(const char* label, UID* target, std::function<void()> oldCallBack, std::function<void()> newCallBack) {
	ImGui::Text(label);

	if (ImGui::BeginTable(label, 2)) {
		ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, 32);
		ImGui::TableNextColumn();

		ImGui::BeginChildFrame(ImGui::GetID(target), ImVec2(32, 32));
		ResourceTexture* resource = App->resources->GetResource<ResourceTexture>(*target);
		if (resource) {
			ImTextureID texId = (ImTextureID) resource->glTexture;
			ImGui::Image(texId, ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
		}
		ImGui::EndChildFrame();

		if (ImGui::BeginDragDropTarget()) {
			std::string payloadType = std::string("_RESOURCE_") + GetResourceTypeName(ResourceTexture::staticType);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str())) {
				UID newUID = *(UID*) payload->Data;
				UID oldUID = *target;
				if (oldUID != newUID) {
					if (oldUID != 0) {
						oldCallBack();
						App->resources->DecreaseReferenceCount(oldUID);
					}
					*target = newUID;
					App->resources->IncreaseReferenceCount(newUID);
					newCallBack();
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::TableNextColumn();

		std::string resourceName = "None";
		if (resource != nullptr) {
			resourceName = resource->GetName();
		}

		std::string resourceNameLabel = std::string("Name: ") + resourceName;
		ImGui::Text(resourceNameLabel.c_str());

		std::string removeButton = std::string(ICON_FA_TIMES "##") + label;
		if (ImGui::Button(removeButton.c_str())) {
			if (*target != 0) {
				App->resources->DecreaseReferenceCount(*target);
				*target = 0;
			}
		}
		ImGui::SameLine();
		ImGui::TextUnformatted("Id:");
		ImGui::SameLine();
		ImGui::TextColored(App->editor->textColor, "%llu", *target);

		ImGui::EndTable();
	}
}

template<>
inline void ImGui::ResourceSlot<ResourceScene>(const char* label, UID* target, std::function<void()> oldCallBack, std::function<void()> newCallBack) {
	ImGui::Text(label);

	if (ImGui::BeginTable(label, 2)) {
		ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, 32);
		ImGui::TableNextColumn();

		ImGui::BeginChildFrame(ImGui::GetID(target), ImVec2(32, 32));
		ImGui::EndChildFrame();

		if (ImGui::BeginDragDropTarget()) {
			std::string payloadType = std::string("_RESOURCE_") + GetResourceTypeName(ResourceScene::staticType);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str())) {
				UID newUID = *(UID*) payload->Data;
				UID oldUID = *target;
				if (oldUID != newUID) {
					if (oldUID != 0) {
						oldCallBack();
					}
					*target = newUID;
					newCallBack();
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::TableNextColumn();

		std::string resourceName = "None";
		ResourceScene* resource = App->resources->GetResource<ResourceScene>(*target);
		if (resource != nullptr) {
			resourceName = resource->GetName();
		}

		std::string resourceNameLabel = std::string("Name: ") + resourceName;
		ImGui::Text(resourceNameLabel.c_str());

		std::string removeButton = std::string(ICON_FA_TIMES "##") + label;
		if (ImGui::Button(removeButton.c_str())) {
			if (*target != 0) {
				App->resources->DecreaseReferenceCount(*target);
				*target = 0;
			}
		}
		ImGui::SameLine();
		ImGui::TextUnformatted("Id:");
		ImGui::SameLine();
		ImGui::TextColored(App->editor->textColor, "%llu", *target);

		ImGui::EndTable();
	}
}