#include "ComponentButton.h"

#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleInput.h"
#include "Modules/ModuleUserInterface.h"
#include "Modules/ModuleEditor.h"
#include "Components/ComponentScript.h"
#include "Components/UI/ComponentEventSystem.h"

#include "Utils/Leaks.h"

#define JSON_TAG_COLOR_HOVER "ColorHover"
#define JSON_TAG_COLOR_CLICK "ColorClick"
#define JSON_TAG_COLOR_DISABLE "ColorDisable"
#define JSON_TAG_CLICKED "Clicked"

void ComponentButton::Init() {
}

void ComponentButton::OnEditorUpdate() {
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

	ImGui::ColorEdit4("Click Color##", colorClicked.ptr());
}

void ComponentButton::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_CLICKED] = clicked;

	JsonValue jColorClick = jComponent[JSON_TAG_COLOR_CLICK];
	jColorClick[0] = colorClicked.x;
	jColorClick[1] = colorClicked.y;
	jColorClick[2] = colorClicked.z;
	jColorClick[3] = colorClicked.w;
}

void ComponentButton::Load(JsonValue jComponent) {
	clicked = jComponent[JSON_TAG_CLICKED];

	JsonValue jColorClick = jComponent[JSON_TAG_COLOR_CLICK];
	colorClicked.Set(jColorClick[0], jColorClick[1], jColorClick[2], jColorClick[3]);
}

void ComponentButton::OnDisable() {
	clicked = false;
}

void ComponentButton::OnClicked() {
	for (ComponentScript& scriptComponent : GetOwner().GetComponents<ComponentScript>()) {
		Script* script = scriptComponent.GetScriptInstance();
		if (script != nullptr) {
			script->OnButtonClick();
		}
	}
}

void ComponentButton::OnClickedInternal() {
	ComponentEventSystem* currEvSys = App->userInterface->GetCurrentEventSystem();
	if (!currEvSys) return;
	clicked = true;

	currEvSys->SetSelected(GetOwner().GetComponent<ComponentSelectable>()->GetID());
}

bool ComponentButton::IsClicked() const {
	return clicked;
}

void ComponentButton::SetClicked(bool clicked_) {
	clicked = clicked_;
}

float4 ComponentButton::GetClickColor() const {
	return colorClicked;
}

float4 ComponentButton::GetTintColor() const {
	if (!IsActive()) return App->userInterface->GetErrorColor();

	ComponentSelectable* sel = GetOwner().GetComponent<ComponentSelectable>();

	if (!sel) return App->userInterface->GetErrorColor();

	if (sel->GetTransitionType() == ComponentSelectable::TransitionType::COLOR_CHANGE) {
		if (!sel->IsInteractable()) {
			return sel->GetDisabledColor();
		} else if (IsClicked()) {
			return colorClicked;
		} else if (sel->IsSelected()) {
			return sel->GetSelectedColor();
		} else if (sel->IsHovered()) {
			return sel->GetHoverColor();
		}
	}

	return App->userInterface->GetErrorColor();
}

void ComponentButton::Update() {
	bool gameControllerConnected = App->input->GetPlayerController(0);

	if (clicked) {
		if (App->input->GetMouseButton(1) == KeyState::KS_IDLE && App->input->GetKey(SDL_SCANCODE_RETURN) == KeyState::KS_IDLE && (!gameControllerConnected || gameControllerConnected && App->input->GetPlayerController(0)->GetButtonState(SDL_CONTROLLER_BUTTON_A) == KeyState::KS_IDLE)) {
			clicked = false;
		}
	}
}
