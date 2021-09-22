#include "PanelNavigation.h"

#include "Globals.h"
#include "Application.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleNavigation.h"
#include "Importers/NavMeshImporter.h"
#include "Navigation/NavMesh.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"
#include "IconsForkAwesome.h"
#include <string>

#include "Utils/Leaks.h"

PanelNavigation::PanelNavigation()
	: Panel("Navigation", true) {
}

void PanelNavigation::Update() {
	ImGui::SetNextWindowDockID(App->editor->dockRightId, ImGuiCond_FirstUseEver);
	std::string windowName = std::string(ICON_FA_MAP " ") + name;
	std::string optionsSymbol = std::string(ICON_FK_COG);
	if (ImGui::Begin(windowName.c_str(), &enabled)) {
		NavMesh* navMesh = App->navigation->GetCurrentNavMesh();

		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (navMesh != nullptr && ImGui::BeginTabBar("NavigationTabBar", tab_bar_flags)) {
			ImGui::Text("Rasterization");
			ImGui::DragFloat("Cell Size", &navMesh->cellSize, App->editor->dragSpeed2f, 0.10f, 1.0f);
			ImGui::DragFloat("Cell Height", &navMesh->cellHeight, App->editor->dragSpeed2f, 0.10f, 1.0f);
			ImGui::Text("");

			ImGui::Text("Agent");
			ImGui::DragFloat("Agent Height", &navMesh->agentHeight, App->editor->dragSpeed2f, 0.01f, 5.0f);
			ImGui::DragFloat("Agent Radius", &navMesh->agentRadius, App->editor->dragSpeed2f, 0.02f, 5.0f);
			ImGui::DragFloat("Max Slope", &navMesh->agentMaxSlope, 1.0f, 0, 60);
			ImGui::DragFloat("Step Height", &navMesh->agentMaxClimb, App->editor->dragSpeed2f, 0, 5.0f);
			ImGui::Text("");

			ImGui::Text("Region");
			ImGui::DragInt("Min Region Size", &navMesh->regionMinSize, 1, 0, 150);
			ImGui::DragInt("Merged Region Size", &navMesh->regionMergeSize, 1, 0, 150);
			ImGui::Text("");

			ImGui::Text("Partitioning");
			ImGui::RadioButton("Watershed", &navMesh->partitionType, 0);
			ImGui::RadioButton("Monotone", &navMesh->partitionType, 1);
			ImGui::RadioButton("Layers", &navMesh->partitionType, 2);
			ImGui::Text("");

			ImGui::Text("Polygonization");
			ImGui::DragInt("Max Edge Length", &navMesh->edgeMaxLen, 1, 0, 50);
			ImGui::DragFloat("Max Edge Error", &navMesh->edgeMaxError, App->editor->dragSpeed2f, 0.10f, 3.0f);
			ImGui::DragInt("Verts Per Poly", &navMesh->vertsPerPoly, 1, 3, 12);
			ImGui::Text("");

			ImGui::Text("Detail Mesh");
			ImGui::DragInt("Sample Distance", &navMesh->detailSampleDist, 1, 0, 16);
			ImGui::DragInt("Max Sample Error", &navMesh->detailSampleMaxError, 1, 0, 16);
			ImGui::Text("");

			ImGui::Checkbox("Keep Intermediate Results", &navMesh->keepInterResults);
			ImGui::Text("");
				
			ImGui::DragInt("Tile Size", &navMesh->tileSize, 8, 0, 128);
			ImGui::Text("");

			if (ImGui::Button("Bake")) {
				App->navigation->BakeNavMesh();
			}

			ImGui::SameLine();

			if (ImGui::Button("Save")) {					
				App->editor->modalToOpen = Modal::CREATE_NAVMESH;
			}

			ImGui::Text("");
			ImGui::TextWrapped("IMPORTANT! NavMesh will be saved, but NavMesh Configuration data won't be saved. Please, take note of the parameters before exiting.");

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}
