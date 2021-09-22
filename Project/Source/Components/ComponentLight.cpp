#include "ComponentLight.h"

#include "Globals.h"
#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleScene.h"
#include "Components/ComponentTransform.h"

#include "debugdraw.h"
#include "Math/float3x3.h"

#include "Utils/Leaks.h"

#define JSON_TAG_LIGHT_TYPE "LightType"
#define JSON_TAG_COLOR "Color"
#define JSON_TAG_INTENSITY "Intensity"
#define JSON_TAG_RADIUS "Radius"
#define JSON_TAG_USE_CUSTOM_FALLOFF "UseCustomFalloff"
#define JSON_TAG_FALLOFF_EXPONENT "FalloffExponent"
#define JSON_TAG_INNER_ANGLE "InnerAngle"
#define JSON_TAG_OUTER_ANGLE "OuterAngle"

void ComponentLight::Init() {
	if (GetOwner().scene->directionalLight == nullptr) {
		GetOwner().scene->directionalLight = &this->GetOwner();
	}
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
			if (lightType == LightType::POINT) {
				dd::sphere(pos, dd::colors::White, radius);
			} else if (lightType == LightType::SPOT) {
				dd::cone(pos, direction * radius, dd::colors::White, radius * tan(outerAngle), 0.0f);
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
				if (lightType == LightType::DIRECTIONAL) {
					if (GetOwner().scene->directionalLight == nullptr) {
						GetOwner().scene->directionalLight = &this->GetOwner();
					}
				} else {
					if (GetOwner().scene->directionalLight == &this->GetOwner()) {
						GetOwner().scene->directionalLight = nullptr;
					}
				}
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (lightType == LightType::DIRECTIONAL) {
		ImGui::InputFloat3("Direction", direction.ptr(), "%.3f", ImGuiInputTextFlags_ReadOnly);
	}

	ImGui::ColorEdit3("Color", color.ptr());
	ImGui::DragFloat("Intensity", &intensity, App->editor->dragSpeed1f, 0.0f, inf);

	if (lightType == LightType::POINT || lightType == LightType::SPOT) {
		ImGui::DragFloat("Radius", &radius, App->editor->dragSpeed1f, 0.0f, inf);
		ImGui::Checkbox("Use Custom Falloff", &useCustomFalloff);
		if (useCustomFalloff) {
			ImGui::DragFloat("Falloff Exponent", &falloffExponent, App->editor->dragSpeed4f, 0.0f, inf);
		}
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
	jComponent[JSON_TAG_LIGHT_TYPE] = (int) lightType;

	JsonValue jColor = jComponent[JSON_TAG_COLOR];
	jColor[0] = color.x;
	jColor[1] = color.y;
	jColor[2] = color.z;

	jComponent[JSON_TAG_INTENSITY] = intensity;
	jComponent[JSON_TAG_RADIUS] = radius;
	jComponent[JSON_TAG_USE_CUSTOM_FALLOFF] = useCustomFalloff;
	jComponent[JSON_TAG_FALLOFF_EXPONENT] = falloffExponent;
	jComponent[JSON_TAG_INNER_ANGLE] = innerAngle;
	jComponent[JSON_TAG_OUTER_ANGLE] = outerAngle;
}

void ComponentLight::Load(JsonValue jComponent) {
	lightType = (LightType)(int) jComponent[JSON_TAG_LIGHT_TYPE];

	JsonValue jColor = jComponent[JSON_TAG_COLOR];
	color.Set(jColor[0], jColor[1], jColor[2]);

	intensity = jComponent[JSON_TAG_INTENSITY];
	radius = jComponent[JSON_TAG_RADIUS];
	useCustomFalloff = jComponent[JSON_TAG_USE_CUSTOM_FALLOFF];
	falloffExponent = jComponent[JSON_TAG_FALLOFF_EXPONENT];
	innerAngle = jComponent[JSON_TAG_INNER_ANGLE];
	outerAngle = jComponent[JSON_TAG_OUTER_ANGLE];
}

void ComponentLight::UpdateLight() {
	ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
	pos = transform->GetGlobalPosition();
	direction = transform->GetGlobalRotation() * float3::unitZ;
}

void ComponentLight::SetIntensity(float newIntensity) {
	intensity = std::max(0.0f, newIntensity);
}

float ComponentLight::GetIntensity() const {
	return intensity;
}
