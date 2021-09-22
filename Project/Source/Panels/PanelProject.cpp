#include "PanelProject.h"

#include "Globals.h"
#include "Application.h"
#include "Utils/Logging.h"
#include "Utils/PathUtils.h"
#include "GameObject.h"
#include "Components/ComponentTransform.h"
#include "Components/ComponentBoundingBox.h"
#include "Modules/ModuleInput.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleCamera.h"
#include "Modules/ModuleRender.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleResources.h"
#include "Utils/AssetCache.h"
#include "Resources/Resource.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"
#include "Math/float3x3.h"
#include "Math/float2.h"
#include "Geometry/OBB.h"
#include "SDL_mouse.h"
#include "SDL_scancode.h"
#include <algorithm>

#include "Utils/Leaks.h"

PanelProject::PanelProject()
	: Panel("Project", true) {}

void PanelProject::Update() {
	ImGui::SetNextWindowDockID(App->editor->dockDownId, ImGuiCond_FirstUseEver);
	std::string windowName = std::string(ICON_FA_FOLDER " ") + name;
	if (ImGui::Begin(windowName.c_str(), &enabled)) {
		AssetCache* assetCache = App->resources->GetAssetCache();
		if (assetCache != nullptr) {
			ImGui::Columns(3);
			ImGui::BeginChild("Folders");
			UpdateFoldersRecursive(assetCache->root);
			ImGui::EndChild();
			ImGui::NextColumn();
			ImGui::BeginChild("Assets");
			UpdateAssets();
			ImGui::EndChild();
			ImGui::NextColumn();
			ImGui::BeginChild("Resources");
			UpdateResources();
			ImGui::EndChild();
		}
	}
	ImGui::End();
}

void PanelProject::UpdateFoldersRecursive(const AssetFolder& folder) {
	std::string name = std::string(ICON_FA_FOLDER " ") + PathUtils::GetFileName(folder.path.c_str());

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
	if (folder.folders.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
	bool isSelected = App->editor->selectedFolder == folder.path;
	if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

	bool open = ImGui::TreeNodeEx(folder.path.c_str(), flags, name.c_str());

	if (ImGui::IsItemClicked()) {
		App->editor->selectedFolder = folder.path;
		App->editor->selectedAsset = "";
	}

	if (open) {
		for (const AssetFolder& childFolder : folder.folders) {
			UpdateFoldersRecursive(childFolder);
		}
		ImGui::TreePop();
	}
}

void PanelProject::UpdateAssets() {
	AssetCache* assetCache = App->resources->GetAssetCache();
	if (assetCache == nullptr) return;
	auto it = assetCache->foldersMap.find(App->editor->selectedFolder);
	if (it == assetCache->foldersMap.end()) return;
	AssetFolder* folder = it->second;

	for (const AssetFile& assetFile : folder->files) {
		std::string name = std::string(ICON_FA_BOX " ") + PathUtils::GetFileNameAndExtension(assetFile.path.c_str());

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
		bool isSelected = App->editor->selectedAsset == assetFile.path;
		if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

		bool open = ImGui::TreeNodeEx(assetFile.path.c_str(), flags, name.c_str());

		if (ImGui::IsItemClicked()) {
			App->editor->selectedAsset = assetFile.path;
		}

		if (open) {
			ImGui::TreePop();
		}
	}
}

void PanelProject::UpdateResources() {
	AssetCache* assetCache = App->resources->GetAssetCache();
	if (assetCache == nullptr) return;
	auto it = assetCache->filesMap.find(App->editor->selectedAsset);
	if (it == assetCache->filesMap.end()) return;
	AssetFile* assetFile = it->second;

	for (UID resourceId : assetFile->resourceIds) {
		ResourceType resourceType = App->resources->GetResourceType(resourceId);
		if (resourceType == ResourceType::UNKNOWN) continue;

		std::string resourceName = std::to_string(resourceId);
		std::string resourceTypeName = GetResourceTypeName(resourceType);
		std::string name = std::string(ICON_FA_FILE " ") + "[" + resourceTypeName + "] " + resourceName.c_str();

		ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
		ImGui::PushID(resourceName.c_str());
		if (ImGui::Selectable(name.c_str(), flags)) {
			App->editor->selectedResource = resourceId;
		}
		ImGui::PopID();

		if (ImGui::BeginDragDropSource()) {
			std::string payloadType = std::string("_RESOURCE_") + resourceTypeName;
			ImGui::SetDragDropPayload(payloadType.c_str(), &resourceId, sizeof(UID));
			ImGui::EndDragDropSource();
		}
	}
}
