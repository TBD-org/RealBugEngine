#include "ComponentLight.h"

#include "Globals.h"
#include "Application.h"
#include "Utils/Logging.h"
#include "GameObject.h"
#include "Components/ComponentTransform.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleEditor.h"

#include "debugdraw.h"
#include "Math/float3x3.h"
#include "imgui.h"

#include "Utils/Leaks.h"

#define JSON_TAG_LIGHT_TYPE "LightType"
#define JSON_TAG_COLOR "Color"
#define JSON_TAG_INTENSITY "Intensity"
#define JSON_TAG_KL "Kl"
#define JSON_TAG_KQ "Kq"
#define JSON_TAG_INNER_ANGLE "InnerAngle"
#define JSON_TAG_OUTER_ANGLE "OuterAngle"

void ComponentLight::Init() {
	UpdateLight();
}

void ComponentLight::Update() {
	UpdateLight();
}

void ComponentLight::DrawGizmos() {
	if (IsActive()) {
		if (lightType == LightType::DIRECTIONAL) {
			ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
			dd::cone(pos, direction * 200, dd::colors::White, 1.0f, 1.0f);
		} else {
			float delta = kl * kl - 4 * (kc - 10) * kq;
			float distance = Max(abs((-kl + sqrt(delta))) / (2 * kq), abs((-kl - sqrt(delta)) / (2 * kq)));
			if (lightType == LightType::POINT) {
				dd::sphere(pos, dd::colors::White, distance);
			} else if (lightType == LightType::SPOT) {
				dd::cone(pos, direction * distance, dd::colors::White, distance * tan(outerAngle), 0.0f);
			}
		}
	}
}

void ComponentLight::OnEditorUpdate() {
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


	ImGui::TextColored(App->editor->titleColor, "Parameters");

	// Light Type Combo
	const char* lightTypeCombo[] = {"Directional Light", "Point Light", "Spot Light"};
	const char* lightTypeComboCurrent = lightTypeCombo[(int) lightType];
	ImGui::TextColored(App->editor->textColor, "Light Type:");
	if (ImGui::BeginCombo("##lightType", lightTypeComboCurrent)) {
		for (int n = 0; n < IM_ARRAYSIZE(lightTypeCombo); ++n) {
			bool isSelected = (lightTypeComboCurrent == lightTypeCombo[n]);
			if (ImGui::Selectable(lightTypeCombo[n], isSelected)) {
				lightType = (LightType) n;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (lightType == LightType::DIRECTIONAL)
		ImGui::InputFloat3("Direction", direction.ptr(), "%.3f", ImGuiInputTextFlags_ReadOnly);

	ImGui::ColorEdit3("Color", color.ptr());
	ImGui::DragFloat("Intensity", &intensity, App->editor->dragSpeed3f, 0.0f, inf);

	if (lightType == LightType::POINT || lightType == LightType::SPOT) {
		ImGui::DragFloat("Linear Constant", &kl, App->editor->dragSpeed5f, 0.0f, 2.0f);
		ImGui::DragFloat("Quadratic Constant", &kq, App->editor->dragSpeed5f, 0.0f, 2.0f);
	}

	if (lightType == LightType::SPOT) {
		float degOuterAngle = outerAngle * RADTODEG;
		float degInnerAngle = innerAngle * RADTODEG;
		if (ImGui::DragFloat("Outter Angle", &degOuterAngle, App->editor->dragSpeed3f, 0.0f, 90.0f)) {
			outerAngle = degOuterAngle * DEGTORAD;
		}
		if (ImGui::DragFloat("Inner Angle", &degInnerAngle, App->editor->dragSpeed3f, 0.0f, degOuterAngle)) {
			innerAngle = degInnerAngle * DEGTORAD;
		}
	}
}

void ComponentLight::Save(JsonValue jComponent) const {
	JsonValue jLightType = jComponent[JSON_TAG_LIGHT_TYPE];
	jLightType = (int) lightType;

	JsonValue jColor = jComponent[JSON_TAG_COLOR];
	jColor[0] = color.x;
	jColor[1] = color.y;
	jColor[2] = color.z;

	JsonValue jIntensity = jComponent[JSON_TAG_INTENSITY];
	jIntensity = intensity;

	JsonValue jKl = jComponent[JSON_TAG_KL];
	jKl = kl;

	JsonValue jKq = jComponent[JSON_TAG_KQ];
	jKq = kq;

	JsonValue jInnerAngle = jComponent[JSON_TAG_INNER_ANGLE];
	jInnerAngle = innerAngle;

	JsonValue jOuterAngle = jComponent[JSON_TAG_OUTER_ANGLE];
	jOuterAngle = outerAngle;
}

void ComponentLight::Load(JsonValue jComponent) {
	JsonValue jLightType = jComponent[JSON_TAG_LIGHT_TYPE];
	lightType = (LightType)(int) jLightType;

	JsonValue jColor = jComponent[JSON_TAG_COLOR];
	color.Set(jColor[0], jColor[1], jColor[2]);

	JsonValue jIntensity = jComponent[JSON_TAG_INTENSITY];
	intensity = jIntensity;

	JsonValue jKl = jComponent[JSON_TAG_KL];
	kl = jKl;

	JsonValue jKq = jComponent[JSON_TAG_KQ];
	kq = jKq;

	JsonValue jInnerAngle = jComponent[JSON_TAG_INNER_ANGLE];
	innerAngle = jInnerAngle;

	JsonValue jOuterAngle = jComponent[JSON_TAG_OUTER_ANGLE];
	outerAngle = jOuterAngle;
}

void ComponentLight::UpdateLight() {
	ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
	pos = transform->GetGlobalPosition();
	direction = transform->GetGlobalRotation() * float3::unitZ;
}