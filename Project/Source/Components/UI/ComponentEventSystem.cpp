#include "ComponentEventSystem.h"

#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleUserInterface.h"
#include "Modules/ModuleInput.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleTime.h"
#include "Components/UI/ComponentSelectable.h"
#include "Scene.h"

#include "Utils/ImGuiUtils.h"
#include "Utils/Logging.h"

#include "Utils/Leaks.h"

#define JSON_TAG_FIRST_SELECTED_ID "FirstSelectedId"

ComponentEventSystem ::~ComponentEventSystem() {
	ComponentEventSystem* eventSystem = App->userInterface->GetCurrentEventSystem();
	if (App->userInterface->GetCurrentEventSystem() == this) {
		App->userInterface->SetCurrentEventSystem(0);
	}
}

void ComponentEventSystem::Start() {
	App->userInterface->SetCurrentEventSystem(GetID());
	LOG("established %u as CurrentEventSystem", GetID());

	GameObject* objectToSelect = GetOwner().scene->GetGameObject(firstSelectedId);
	if (objectToSelect) {
		ComponentSelectable* selectable = objectToSelect->GetComponent<ComponentSelectable>();

		SetSelected(selectable->GetID());
	}
}

void ComponentEventSystem::Update() {
	if (App->time->HasGameStarted()) {
		navigationTimer = Max(0.0f, navigationTimer - App->time->GetRealTimeDeltaTime());
	}

	if (GetCurrentSelected() == nullptr || !GetCurrentSelected()->IsActive()) return;

	bool keyPressed = false;
	if (!App->userInterface->handlingSlider && navigationTimer == 0) {
		float2 selectionDir = float2(0.f, 0.f);

		if (App->input->GetKey(SDL_SCANCODE_UP) == KeyState::KS_DOWN) {
			selectionDir = float2(0.f, 1.f);
			keyPressed = true;
		}
		if (App->input->GetKey(SDL_SCANCODE_DOWN) == KeyState::KS_DOWN) {
			selectionDir = float2(0.f, -1.f);
			keyPressed = true;
		}
		if (App->input->GetKey(SDL_SCANCODE_LEFT) == KeyState::KS_DOWN) {
			selectionDir = float2(-1.f, 0.f);
			keyPressed = true;
		}
		if (App->input->GetKey(SDL_SCANCODE_RIGHT) == KeyState::KS_DOWN) {
			selectionDir = float2(1.f, 0.f);
			keyPressed = true;
		}

		PlayerController* controller = App->input->GetPlayerController(0);
		if (controller) {
			if (selectionDir.x == 0 && selectionDir.y == 0) {
				if (controller->GetButtonState(SDL_CONTROLLER_BUTTON_DPAD_UP) == KeyState::KS_REPEAT || controller->GetButtonState(SDL_CONTROLLER_BUTTON_DPAD_UP) == KeyState::KS_DOWN || controller->GetAxisNormalized(SDL_CONTROLLER_AXIS_LEFTY) < -0.5f) {
					selectionDir = float2(0.f, 1.f);
					keyPressed = true;
				}
				if (controller->GetButtonState(SDL_CONTROLLER_BUTTON_DPAD_DOWN) == KeyState::KS_REPEAT || controller->GetButtonState(SDL_CONTROLLER_BUTTON_DPAD_DOWN) == KeyState::KS_DOWN || controller->GetAxisNormalized(SDL_CONTROLLER_AXIS_LEFTY) > 0.5f) {
					selectionDir = float2(0.f, -1.f);
					keyPressed = true;
				}
				if (controller->GetButtonState(SDL_CONTROLLER_BUTTON_DPAD_LEFT) == KeyState::KS_REPEAT || controller->GetButtonState(SDL_CONTROLLER_BUTTON_DPAD_LEFT) == KeyState::KS_DOWN || controller->GetAxisNormalized(SDL_CONTROLLER_AXIS_LEFTX) < -0.5f) {
					selectionDir = float2(-1.f, 0.f);
					keyPressed = true;
				}
				if (controller->GetButtonState(SDL_CONTROLLER_BUTTON_DPAD_RIGHT) == KeyState::KS_REPEAT || controller->GetButtonState(SDL_CONTROLLER_BUTTON_DPAD_RIGHT) == KeyState::KS_DOWN || controller->GetAxisNormalized(SDL_CONTROLLER_AXIS_LEFTX) > 0.5f) {
					selectionDir = float2(1.f, 0.f);
					keyPressed = true;
				}
			}
		}

		if (keyPressed) {
			if (selectedId != 0) {
				ComponentSelectable* currentSel = GetCurrentSelected();
				if (currentSel != nullptr) {
					ComponentSelectable* newSel = currentSel->FindSelectableOnDir(selectionDir);
					if (newSel != nullptr) {
						navigationTimer = timeBetweenNavigations;
						SetSelected(newSel->GetID());
					}
				}
			}
		}
	}
}

void ComponentEventSystem::OnEditorUpdate() {
	ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Current Selected:");

	if (selectedId != 0) {
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 1.f), GetCurrentSelected()->GetOwner().name.c_str());
	}

	ImGui::GameObjectSlot("First selected object", &firstSelectedId);
}

void ComponentEventSystem::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_FIRST_SELECTED_ID] = firstSelectedId;
}

void ComponentEventSystem::Load(JsonValue jComponent) {
	firstSelectedId = jComponent[JSON_TAG_FIRST_SELECTED_ID];
}

void ComponentEventSystem::OnEnable() {
	if (App->userInterface->GetCurrentEventSystem() == nullptr) {
		App->userInterface->SetCurrentEventSystem(GetID());
	}
}

void ComponentEventSystem::OnDisable() {
	if (App->userInterface->GetCurrentEventSystem() == this) {
		App->userInterface->SetCurrentEventSystem(0);
	}
}

void ComponentEventSystem::SetSelected(ComponentSelectable* newSelectableComponent) {
	ComponentSelectable* currentSel = GetCurrentSelected();

	if (currentSel != nullptr) {
		currentSel->OnDeselect();
	}

	if (!newSelectableComponent) return;

	selectedId = newSelectableComponent->GetID();

	newSelectableComponent->OnSelect();
}

void ComponentEventSystem::SetSelected(UID newSelectableComponentId) {
	ComponentSelectable* currentSel = GetCurrentSelected();
	if (currentSel != nullptr) {
		currentSel->OnDeselect();
	}
	selectedId = newSelectableComponentId;

	if (newSelectableComponentId != 0) {
		ComponentSelectable* newSelectableComponent = GetOwner().scene->GetComponent<ComponentSelectable>(newSelectableComponentId);

		if (newSelectableComponent != nullptr) {
			newSelectableComponent->OnSelect();
		}
	}
}

void ComponentEventSystem::EnteredPointerOnSelectable(ComponentSelectable* newHoveredComponent) {
	if (hoveredSelectableID != 0) {
		ComponentSelectable* selectableToUnHover = GetOwner().scene->selectableComponents.Find(hoveredSelectableID);
		if (selectableToUnHover) selectableToUnHover->SetHovered(false);
	}

	hoveredSelectableID = newHoveredComponent->GetID();
}

void ComponentEventSystem::ExitedPointerOnSelectable(ComponentSelectable* newUnHoveredComponent) {
	if (hoveredSelectableID == newUnHoveredComponent->GetID()) {
		hoveredSelectableID = 0;
	}
}

ComponentSelectable* ComponentEventSystem::GetCurrentSelected() const {
	if (selectedId == 0) return nullptr;

	return GetOwner().scene->GetComponent<ComponentSelectable>(selectedId);
}

ComponentSelectable* ComponentEventSystem::GetCurrentlyHovered() const {
	if (hoveredSelectableID == 0) return nullptr;

	ComponentSelectable* sel = GetOwner().scene->GetComponent<ComponentSelectable>(hoveredSelectableID);
	if (sel == nullptr) return nullptr;
	return sel->IsActive() ? sel : nullptr;
}

void ComponentEventSystem::SetClickedGameObject(GameObject* clickedObj_) {
	clickedObj = clickedObj_;
}

GameObject* ComponentEventSystem::GetClickedGameObject() {
	return clickedObj;
}
