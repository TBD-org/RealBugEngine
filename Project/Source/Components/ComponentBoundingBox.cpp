#pragma once
#include "ComponentBoundingBox.h"

#include "Globals.h"
#include "Application.h"
#include "Utils/Logging.h"
#include "GameObject.h"
#include "Components/ComponentTransform.h"
#include "Modules/ModuleEditor.h"

#include "debugdraw.h"
#include "Math/float3x3.h"

#include "Utils/Leaks.h"

#define JSON_TAG_LOCAL_BOUNDING_BOX "LocalBoundingBox"

void ComponentBoundingBox::OnEditorUpdate() {
	ImGui::TextColored(App->editor->titleColor, "Bounding Box");

	if (ImGui::Checkbox("Active", &active)) {
		if (GetOwner().IsActive()) {
			if (active) {
				Enable();
			} else {
				Disable();
			}
		}
	}

	if (IsActive()) DrawBoundingBox();
}

void ComponentBoundingBox::Save(JsonValue jComponent) const {
	JsonValue jLocalBoundingBox = jComponent[JSON_TAG_LOCAL_BOUNDING_BOX];
	jLocalBoundingBox[0] = localAABB.minPoint.x;
	jLocalBoundingBox[1] = localAABB.minPoint.y;
	jLocalBoundingBox[2] = localAABB.minPoint.z;
	jLocalBoundingBox[3] = localAABB.maxPoint.x;
	jLocalBoundingBox[4] = localAABB.maxPoint.y;
	jLocalBoundingBox[5] = localAABB.maxPoint.z;
}

void ComponentBoundingBox::Load(JsonValue jComponent) {
	JsonValue jLocalBoundingBox = jComponent[JSON_TAG_LOCAL_BOUNDING_BOX];
	localAABB.minPoint.Set(jLocalBoundingBox[0], jLocalBoundingBox[1], jLocalBoundingBox[2]);
	localAABB.maxPoint.Set(jLocalBoundingBox[3], jLocalBoundingBox[4], jLocalBoundingBox[5]);

	dirty = true;
}

void ComponentBoundingBox::DuplicateComponent(GameObject& owner) {
	ComponentBoundingBox* component = owner.CreateComponent<ComponentBoundingBox>();
	component->SetLocalBoundingBox(this->localAABB);
}

void ComponentBoundingBox::SetLocalBoundingBox(const AABB& boundingBox) {
	localAABB = boundingBox;
	dirty = true;
}

void ComponentBoundingBox::CalculateWorldBoundingBox(bool force) {
	if (dirty || force) {
		GameObject& owner = GetOwner();
		ComponentTransform* transform = owner.GetComponent<ComponentTransform>();
		worldOBB = OBB(localAABB);
		worldOBB.Transform(transform->GetGlobalMatrix());
		worldAABB = worldOBB.MinimalEnclosingAABB();
		dirty = false;
	}
}

void ComponentBoundingBox::DrawBoundingBox() {
	float3 points[8];
	GetWorldOBB().GetCornerPoints(points);

	// Reorder points for drawing
	float3 aux;
	aux = points[2];
	points[2] = points[3];
	points[3] = aux;
	aux = points[6];
	points[6] = points[7];
	points[7] = aux;

	dd::box(points, dd::colors::White);
}

void ComponentBoundingBox::Invalidate() {
	dirty = true;
}

const OBB& ComponentBoundingBox::GetWorldOBB() {
	CalculateWorldBoundingBox();
	return worldOBB;
}

const AABB& ComponentBoundingBox::GetWorldAABB() {
	CalculateWorldBoundingBox();
	return worldAABB;
}

const float3 ComponentBoundingBox::GetLocalMinPointAABB() {
	CalculateWorldBoundingBox();
	return localAABB.minPoint;
}
const float3 ComponentBoundingBox::GetLocalMaxPointAABB() {
	CalculateWorldBoundingBox();
	return localAABB.maxPoint;
}