#include "PanelControlEditor.h"

#include "Application.h"
#include "Utils/Logging.h"
#include "TesseractEvent.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleInput.h"
#include "Modules/ModuleEvents.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleProject.h"
#include "Modules/ModulePrograms.h"
#include "Modules/ModuleWindow.h"
#include "Modules/ModuleScene.h"
#include "Scene.h"

#include "imgui_internal.h"
#include "IconsFontAwesome5.h"
#include "Math/float3x3.h"
#include "Math/float2.h"
#include "Geometry/OBB.h"
#include "SDL_mouse.h"
#include "SDL_scancode.h"
#include <algorithm>

#include "Utils/Leaks.h"

PanelControlEditor::PanelControlEditor()
	: Panel("Editor Control", true) {}

void PanelControlEditor::Update() {
	ImGui::SetNextWindowDockID(App->editor->dockUpId, ImGuiCond_FirstUseEver);
	std::string windowName = std::string(ICON_FA_TOOLS " ") + name;
	ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove;

	if (ImGui::Begin(windowName.c_str(), &enabled, flags)) {
		// Control Bar
		if (App->input->GetMouseButton(SDL_BUTTON_RIGHT) == KeyState::KS_IDLE) {
			if (App->input->GetKey(SDL_SCANCODE_W) != KeyState::KS_IDLE) currentGuizmoOperation = ImGuizmo::TRANSLATE; // W key
			if (App->input->GetKey(SDL_SCANCODE_E) != KeyState::KS_IDLE) currentGuizmoOperation = ImGuizmo::ROTATE;
			if (App->input->GetKey(SDL_SCANCODE_R) != KeyState::KS_IDLE) currentGuizmoOperation = ImGuizmo::SCALE; // R key
		}

		std::string translate = std::string(ICON_FA_ARROWS_ALT);
		std::string rotate = std::string(ICON_FA_SYNC_ALT);
		std::string scale = std::string(ICON_FA_EXTERNAL_LINK_ALT);
		std::string local = std::string(ICON_FA_BOX);
		std::string global = std::string(ICON_FA_GLOBE);
		if (ImGui::BeginMenuBar()) {
			if (ImGui::RadioButton(translate.c_str(), currentGuizmoOperation == ImGuizmo::TRANSLATE)) currentGuizmoOperation = ImGuizmo::TRANSLATE;
			ImGui::SameLine();
			if (ImGui::RadioButton(rotate.c_str(), currentGuizmoOperation == ImGuizmo::ROTATE)) currentGuizmoOperation = ImGuizmo::ROTATE;
			ImGui::SameLine();
			if (ImGui::RadioButton(scale.c_str(), currentGuizmoOperation == ImGuizmo::SCALE)) currentGuizmoOperation = ImGuizmo::SCALE;

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			if (currentGuizmoOperation != ImGuizmo::SCALE) {
				if (ImGui::RadioButton(local.c_str(), currentGuizmoMode == ImGuizmo::LOCAL)) currentGuizmoMode = ImGuizmo::LOCAL;
				ImGui::SameLine();
				if (ImGui::RadioButton(global.c_str(), currentGuizmoMode == ImGuizmo::WORLD)) currentGuizmoMode = ImGuizmo::WORLD;
			} else {
				currentGuizmoMode = ImGuizmo::LOCAL;
				ImGui::Dummy(ImVec2(78, 0));
			}

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			ImGui::TextColored(App->editor->titleColor, "Snap");
			ImGui::SameLine();
			ImGui::Checkbox("##snap", &useSnap);
			ImGui::SameLine();

			ImGui::PushItemWidth(150);
			switch (currentGuizmoOperation) {
			case ImGuizmo::TRANSLATE:
				ImGui::InputFloat3("##Snap", &snap[0]);
				break;
			case ImGuizmo::ROTATE:
				ImGui::InputFloat("##Snap Angle", &snap[0]);
				break;
			case ImGuizmo::SCALE:
				ImGui::InputFloat("##Snap Scale", &snap[0]);
				break;
			}

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(75, 0));
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			std::string play = std::string(ICON_FA_PLAY);
			std::string pause = std::string(ICON_FA_PAUSE);
			std::string stop = std::string(ICON_FA_STOP);
			std::string step = std::string(ICON_FA_STEP_FORWARD);
			std::string compile = std::string("Compile Scripts");
			// Play / Pause / Step buttons
			if (App->time->HasGameStarted()) {
				if (ImGui::Button(stop.c_str())) {
					App->window->ActivateCursor(false);
					App->events->AddEvent(TesseractEvent(TesseractEventType::PRESSED_STOP));
				}
				ImGui::SameLine();
				if (App->time->IsGameRunning()) {
					if (ImGui::Button(pause.c_str())) {
						App->window->ActivateCursor(false);
						App->events->AddEvent(TesseractEvent(TesseractEventType::PRESSED_PAUSE));
					}
				} else {
					if (ImGui::Button(play.c_str())) {
						App->window->ActivateCursor(true);
						App->events->AddEvent(TesseractEvent(TesseractEventType::PRESSED_RESUME));
					}
				}
			} else {
				if (ImGui::Button(play.c_str())) {
					if (!App->project->IsGameLoaded()) {
#if _DEBUG
						App->project->CompileProject(Configuration::DEBUG_EDITOR);
#else
						App->project->CompileProject(Configuration::RELEASE_EDITOR);
#endif // _DEBUG
					}
					App->events->AddEvent(TesseractEvent(TesseractEventType::PRESSED_PLAY));
				}
			}
			ImGui::SameLine();
			if (ImGui::Button(step.c_str())) {
				App->window->ActivateCursor(false);
				App->events->AddEvent(TesseractEvent(TesseractEventType::PRESSED_STEP));
			}
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();
			if (App->time->IsGameRunning()) {
				ImGui::Dummy(ImVec2(48, 0));
			} else {
				ImGui::Dummy(ImVec2(75, 0));
			}

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();
			if (ImGui::Button(compile.c_str())) {
#if _DEBUG
				App->project->CompileProject(Configuration::DEBUG_EDITOR);
#else
				App->project->CompileProject(Configuration::RELEASE_EDITOR);
#endif
			}

			ImGui::SameLine();
			if (ImGui::Button("Compile Shaders")) {
				App->programs->UnloadShaders();
				App->programs->LoadShaders();
			}

			ImGui::PopItemWidth();
			ImGui::EndMenuBar();
		}
	}
	ImGui::End();
}

ImGuizmo::OPERATION PanelControlEditor::GetImGuizmoOperation() const {
	return currentGuizmoOperation;
}

ImGuizmo::MODE PanelControlEditor::GetImGuizmoMode() const {
	return currentGuizmoMode;
}

bool PanelControlEditor::GetImGuizmoUseSnap() const {
	return useSnap;
}

void PanelControlEditor::GetImguizmoSnap(float* newSnap) const {
	newSnap[0] = snap[0];
	newSnap[1] = snap[1];
	newSnap[2] = snap[2];
}