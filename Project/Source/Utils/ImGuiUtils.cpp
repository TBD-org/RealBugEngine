#include "ImGuiUtils.h"

#include "GameObject.h"
#include "Application.h"
#include "Modules/ModuleScene.h"
#include "Scene.h"

void ImGui::GameObjectSlot(const char* label, UID* target) {
	ImGui::Text(label);

	if (ImGui::BeginTable(label, 2)) {
		ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, 32);
		ImGui::TableNextColumn();

		ImGui::BeginChildFrame(ImGui::GetID(target), ImVec2(32, 32));
		ImGui::EndChildFrame();

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_HIERARCHY")) {
				*target = *(UID*) payload->Data;
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::TableNextColumn();

		GameObject* targetGameObject = App->scene->GetCurrentScene()->GetGameObject(*target);
		std::string nameText = std::string("GameObject: ") + (targetGameObject ? targetGameObject->name : "None");
		ImGui::Text(nameText.c_str());

		std::string removeButton = std::string(ICON_FA_TIMES "##") + label;
		if (ImGui::Button(removeButton.c_str())) {
			if (*target != 0) {
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