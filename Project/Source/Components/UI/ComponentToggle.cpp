#include "ComponentToggle.h"

#include "Utils/Logging.h"
#include "GameObject.h"
#include "Components/UI/ComponentEventSystem.h"
#include "Components/UI/ComponentImage.h"
#include "Components/ComponentBoundingBox2D.h"
#include "Application.h"
#include "Modules/ModuleUserInterface.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleInput.h"
#include "Resources/ResourceScript.h"
#include "Scripting/Script.h"
#include "imgui.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/Leaks.h"

#define JSON_TAG_IS_ON "IsOn"
#define JSON_TAG_ENABLED_IMAGE_ID "EnabledImageID"
#define JSON_TAG_CLICKED_COLOR "ClickedColor"

ComponentToggle ::~ComponentToggle() {}

void ComponentToggle::Init() {
	OnValueChanged();
}

void ComponentToggle::OnClicked() {
	SetChecked(!IsChecked());
	App->userInterface->GetCurrentEventSystem()->SetSelected(GetOwner().GetComponent<ComponentSelectable>()->GetID());
}

void ComponentToggle ::OnValueChanged() {
	ComponentImage* childImage = GetEnabledImage();
	if (childImage) {
		if (IsChecked()) {
			childImage->Enable();
		} else {
			childImage->Disable();
		}
	}

	for (ComponentScript& scriptComponent : GetOwner().GetComponents<ComponentScript>()) {
		Script* script = scriptComponent.GetScriptInstance();
		if (script != nullptr) {
			script->OnToggled(IsChecked());
		}
	}
}

bool ComponentToggle ::IsChecked() const {
	return isChecked;
}
void ComponentToggle ::SetChecked(bool b) {
	isChecked = b;
	OnValueChanged();
}

ComponentImage* ComponentToggle::GetEnabledImage() const {
	GameObject* imageObj = App->scene->scene->GetGameObject(enabledImageObjectID);
	if (imageObj) {
		return (ComponentImage*) App->scene->scene->GetGameObject(enabledImageObjectID)->GetComponent<ComponentImage>();
	}
	return nullptr;
}

void ComponentToggle::SetEnabledImageObj(UID enabledImageObjID_) {
	enabledImageObjectID = enabledImageObjID_;
}

bool ComponentToggle::IsClicked() const {
	return clicked;
}

void ComponentToggle::SetClicked(bool clicked_) {
	clicked = clicked_;
}

float4 ComponentToggle::GetTintColor() const {
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

float4 ComponentToggle::GetClickColor() const {
	return colorClicked;
}

void ComponentToggle::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_IS_ON] = isChecked;
	jComponent[JSON_TAG_ENABLED_IMAGE_ID] = enabledImageObjectID;

	JsonValue jColorClick = jComponent[JSON_TAG_CLICKED_COLOR];
	jColorClick[0] = colorClicked.x;
	jColorClick[1] = colorClicked.y;
	jColorClick[2] = colorClicked.z;
	jColorClick[3] = colorClicked.w;
}

void ComponentToggle::Load(JsonValue jComponent) {
	isChecked = jComponent[JSON_TAG_IS_ON];
	enabledImageObjectID = jComponent[JSON_TAG_ENABLED_IMAGE_ID];

	JsonValue jColorClick = jComponent[JSON_TAG_CLICKED_COLOR];
	colorClicked.Set(jColorClick[0], jColorClick[1], jColorClick[2], jColorClick[3]);
}

void ComponentToggle::OnEditorUpdate() {
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
	ImGui::GameObjectSlot("Enabled Image GameObject", &enabledImageObjectID);

	bool isChecked_ = IsChecked();
	if (ImGui::Checkbox("Is Checked", &isChecked_)) {
		SetChecked(!IsChecked());
	}
}

void ComponentToggle::Update() {
	if (clicked) {
		if (!App->input->GetMouseButton(1)) {
			clicked = false;
		}
	}
}