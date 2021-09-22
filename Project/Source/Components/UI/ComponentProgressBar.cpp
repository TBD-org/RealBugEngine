#include "ComponentProgressBar.h"

#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleScene.h"
#include "Components/UI/ComponentTransform2D.h"
#include "Utils/JsonValue.h"

#include "Utils/Leaks.h"

#define JSON_TAG_FILL_POSITION "FillPosition"
#define JSON_TAG_MIN_VALUE "Min"
#define JSON_TAG_MAX_VALUE "Max"
#define JSON_TAG_FILL_DIRECTION "FillDirection"
#define JSON_TAG_PROGRESS_VALUE "ProgressValue"
#define JSON_TAG_FILL_IMAGE "FillImage"
#define JSON_TAG_BACKGROUND_IMAGE "BackgroundImage"
#define JSON_TAG_BACKGROUND_POSITION "BackgroundPosition"
#define JSON_TAG_BACKGROUND_SIZE "BackgroundSize"

void ComponentProgressBar::Init() {
	fill = GetOwner().scene->GetGameObject(fillID);
	background = GetOwner().scene->GetGameObject(backgroundID);
}

void ComponentProgressBar::Update() {
	//The progress bar GameObject must have two image gameobjects as childs (for background and fill)
	if (background == nullptr || fill == nullptr) {

		//IMPORTANT: Background goes first then fill
		for (std::vector<GameObject*>::const_iterator it = GetOwner().GetChildren().begin(); it != GetOwner().GetChildren().end(); ++it) {
			if (it == GetOwner().GetChildren().begin()) {
				background = *it;
			} else if (fill == nullptr) {
				fill = *it;
			}
		}
	}

	if (background != nullptr && fill != nullptr) {
		rectBack = background->GetComponent<ComponentTransform2D>();
		rectFill = fill->GetComponent<ComponentTransform2D>();
	} else {
		return;
	}

	backPos = rectBack->GetPosition();
	backSize = rectBack->GetSize();

	float percent = (value - min) / (max - min);
	//we set the fill image width as the percent of the background according to the value (if 0.5 then 50% of background's width)
	//height stays the same as the back
	rectFill->SetSize(float2((backSize.x * percent), backSize.y));

	//with the size of the fill setted we must position it since if we keep the background pos we will have it centered
	//The image is aligned to the left here, we will give the option to slide from left to right in the future
	fillXPos = ((backSize.x - (backSize.x * percent)) / 2);

	switch(dir) {
	case FillDirection::LEFT_TO_RIGHT:
		fillXPos = backPos.x - fillXPos;
		rectFill->SetPosition(float3(fillXPos, backPos.y, backPos.z));
		break;
	case FillDirection::RIGHT_TO_LEFT:
		fillXPos = backPos.x + fillXPos;
		rectFill->SetPosition(float3(fillXPos, backPos.y, backPos.z));
		break;
	case FillDirection::BOTTOM_TO_TOP:
		rectFill->SetSize(float2(backSize.x, backSize.y * percent));
		fillXPos = ((backSize.y - (backSize.y * percent)) / 2);
		fillXPos = backPos.y - fillXPos;
		rectFill->SetPosition(float3(backPos.x, fillXPos, backPos.z));
		break;
	case FillDirection::TOP_TO_BOTTOM:
		rectFill->SetSize(float2(backSize.x, backSize.y * percent));
		fillXPos = ((backSize.y - (backSize.y * percent)) / 2);
		fillXPos = backPos.y + fillXPos;
		rectFill->SetPosition(float3(backPos.x, fillXPos, backPos.z));
		break;
	}

}

void ComponentProgressBar::OnEditorUpdate() {
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

	ImGui::DragFloat("Value", &value, App->editor->dragSpeed2f, min, max);
	ImGui::DragFloat("Min", &min, App->editor->dragSpeed2f, -inf, max - 1);
	ImGui::DragFloat("Max", &max, App->editor->dragSpeed2f, min + 1, inf);

	ImGui::Combo("Fill Direction", &dirIndex, fillDirections, IM_ARRAYSIZE(fillDirections));
	dir = static_cast<FillDirection>(dirIndex);

	ImGui::Separator();
}

void ComponentProgressBar::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_FILL_POSITION] = fillXPos;
	jComponent[JSON_TAG_MIN_VALUE] = min;
	jComponent[JSON_TAG_MAX_VALUE] = max;
	jComponent[JSON_TAG_PROGRESS_VALUE] = value;
	jComponent[JSON_TAG_FILL_DIRECTION] = dirIndex;
	JsonValue jFillDir = jComponent[JSON_TAG_FILL_DIRECTION];
	jFillDir = static_cast<int>(dir);
	jComponent[JSON_TAG_FILL_IMAGE] = fill ? fill->GetID() : 0;
	jComponent[JSON_TAG_BACKGROUND_IMAGE] = background ? background->GetID() : 0;
	JsonValue jPosition = jComponent[JSON_TAG_BACKGROUND_POSITION];
	jPosition[0] = backPos.x;
	jPosition[1] = backPos.y;
	jPosition[2] = backPos.z;
	JsonValue jSize = jComponent[JSON_TAG_BACKGROUND_SIZE];
	jSize[0] = backSize.x;
	jSize[1] = backSize.y;
}

void ComponentProgressBar::Load(JsonValue jComponent) {
	fillXPos = jComponent[JSON_TAG_FILL_POSITION];
	min = jComponent[JSON_TAG_MIN_VALUE];
	max = jComponent[JSON_TAG_MAX_VALUE];
	value = jComponent[JSON_TAG_PROGRESS_VALUE];
	JsonValue jFillDir = jComponent[JSON_TAG_FILL_DIRECTION];
	dir = (FillDirection)(int) jFillDir;
	dirIndex = (int) dir;
	fillID = jComponent[JSON_TAG_FILL_IMAGE];
	backgroundID = jComponent[JSON_TAG_BACKGROUND_IMAGE];
	JsonValue jPosition = jComponent[JSON_TAG_BACKGROUND_POSITION];
	backPos.Set(jPosition[0], jPosition[1], jPosition[2]);
	JsonValue jSize = jComponent[JSON_TAG_BACKGROUND_SIZE];
	backSize.Set(jSize[0], jSize[1]);
}

void ComponentProgressBar::SetValue(float v) {
	value = v;
}

void ComponentProgressBar::SetFillPos(float fillPos) {
	fillXPos = fillPos;
}

void ComponentProgressBar::SetMin(float m) {
	min = m;
}

void ComponentProgressBar::SetMax(float n) {
	max = n;
}

float ComponentProgressBar::GetValue() {
	return value;
}

float ComponentProgressBar::GetFillPos() {
	return fillXPos;
}

float ComponentProgressBar::GetMin() {
	return min;
}

float ComponentProgressBar::GetMax() {
	return max;
}
