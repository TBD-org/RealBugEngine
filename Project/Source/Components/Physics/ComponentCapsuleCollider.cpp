#include "ComponentCapsuleCollider.h"

#include "Application.h"
#include "GameObject.h"
#include "Modules/ModulePhysics.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleTime.h"
#include "Components/ComponentBoundingBox.h"

#define JSON_TAG_MASS "mass"
#define JSON_TAG_RADIUS "radius"
#define JSON_TAG_HEIGHT "height"
#define JSON_TAG_TYPE "type"
#define JSON_TAG_CENTER_OFFSET "centerOffset"
#define JSON_TAG_FREEZE_ROTATION "freezeRotation"
#define JSON_TAG_COLLIDER_TYPE "colliderType"

void ComponentCapsuleCollider::Init() {
	if (!centerOffset.IsFinite()) {
		ComponentBoundingBox* boundingBox = GetOwner().GetComponent<ComponentBoundingBox>();
		if (boundingBox) {
			radius = (boundingBox->GetWorldOBB().HalfSize().x > boundingBox->GetWorldOBB().HalfSize().z) ? boundingBox->GetWorldOBB().HalfSize().x : boundingBox->GetWorldOBB().HalfSize().z;
			height = boundingBox->GetWorldOBB().Size().MaxElement() - 2 * radius;
			centerOffset = boundingBox->GetWorldOBB().CenterPoint() - GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition();
		} else {
			centerOffset = float3::zero;
		}
	}
	if (App->time->IsGameRunning() && !rigidBody) App->physics->CreateCapsuleRigidbody(this);
}

void ComponentCapsuleCollider::DrawGizmos() {
	if (IsActiveInHierarchy()) {
		ComponentTransform* ownerTransform = GetOwner().GetComponent<ComponentTransform>();
		switch (type) {
		case CapsuleType::X:
			dd::cone(ownerTransform->GetGlobalPosition() + ownerTransform->GetGlobalRotation() * centerOffset - float3(height / 2, 0, 0), ownerTransform->GetGlobalRotation() * float3(height, 0, 0).Mul(centerOffset), dd::colors::Green, radius, radius);
			dd::sphere(ownerTransform->GetGlobalPosition() + ownerTransform->GetGlobalRotation() * centerOffset + float3(height / 2, 0, 0), dd::colors::Green, radius, 3);
			dd::sphere(ownerTransform->GetGlobalPosition() + ownerTransform->GetGlobalRotation() * centerOffset - float3(height / 2, 0, 0), dd::colors::Green, radius, 4);
			break;
		case CapsuleType::Y:
			dd::cone(ownerTransform->GetGlobalPosition() + ownerTransform->GetGlobalRotation() * centerOffset - float3(0, height / 2, 0), ownerTransform->GetGlobalRotation() * float3(0, height, 0), dd::colors::Green, radius, radius);
			dd::sphere(ownerTransform->GetGlobalPosition() + ownerTransform->GetGlobalRotation() * centerOffset + float3(0, height / 2, 0), dd::colors::Green, radius, 1);
			dd::sphere(ownerTransform->GetGlobalPosition() + ownerTransform->GetGlobalRotation() * centerOffset - float3(0, height / 2, 0), dd::colors::Green, radius, 2);
			break;
		case CapsuleType::Z:
			dd::cone(ownerTransform->GetGlobalPosition() + ownerTransform->GetGlobalRotation() * centerOffset - float3(0, 0, height / 2), ownerTransform->GetGlobalRotation() * float3(0, 0, height), dd::colors::Green, radius, radius);
			dd::sphere(ownerTransform->GetGlobalPosition() + ownerTransform->GetGlobalRotation() * centerOffset + float3(0, 0, height / 2), dd::colors::Green, radius, 5);
			dd::sphere(ownerTransform->GetGlobalPosition() + ownerTransform->GetGlobalRotation() * centerOffset - float3(0, 0, height / 2), dd::colors::Green, radius, 6);
			break;
		}
	}
}

void ComponentCapsuleCollider::OnEditorUpdate() {
	const char* colliderTypeItems[] = {"Dynamic", "Static", "Kinematic", "Trigger"};
	const char* colliderCurrent = colliderTypeItems[(int) colliderType];
	if (ImGui::BeginCombo("Collider Mode", colliderCurrent)) {
		for (int n = 0; n < IM_ARRAYSIZE(colliderTypeItems); ++n) {
			if (ImGui::Selectable(colliderTypeItems[n])) {
				colliderType = ColliderType(n);
				if (App->time->IsGameRunning()) {
					App->physics->UpdateCapsuleRigidbody(this);
				}
			}
		}
		ImGui::EndCombo();
	}

	if (colliderType == ColliderType::DYNAMIC) {
		if (ImGui::DragFloat("Mass", &mass, App->editor->dragSpeed3f, 0.0f, 100.f) && App->time->IsGameRunning()) {
			rigidBody->setMassProps(mass, btVector3(0, 0, 0));
		}
	}

	if (ImGui::DragFloat("Radius", &radius, App->editor->dragSpeed3f, 0.0f, inf) && App->time->IsGameRunning()) {
		App->physics->UpdateCapsuleRigidbody(this);
	}

	if (ImGui::DragFloat("Height", &height, App->editor->dragSpeed3f, 0.0f, inf) && App->time->IsGameRunning()) {
		App->physics->UpdateCapsuleRigidbody(this);
	}

	const char* types[] = {"X", "Y", "Z"};
	const char* currentType = types[(int) type];
	if (ImGui::BeginCombo("Capsule Type Orientation", currentType)) {
		for (int n = 0; n < IM_ARRAYSIZE(types); ++n) {
			bool isSelected = (currentType == types[n]);
			if (ImGui::Selectable(types[n], isSelected)) {
				type = CapsuleType(n);
				ComponentBoundingBox* boundingBox = GetOwner().GetComponent<ComponentBoundingBox>();
				switch (type) {
				case CapsuleType::X:
					if (boundingBox) {
						radius = (boundingBox->GetWorldOBB().HalfSize().y > boundingBox->GetWorldOBB().HalfSize().z) ? boundingBox->GetWorldOBB().HalfSize().y : boundingBox->GetWorldOBB().HalfSize().z;
						height = boundingBox->GetWorldOBB().Size().MaxElement() - 2 * radius;
					}
					break;
				case CapsuleType::Y:
					if (boundingBox) {
						radius = (boundingBox->GetWorldOBB().HalfSize().x > boundingBox->GetWorldOBB().HalfSize().z) ? boundingBox->GetWorldOBB().HalfSize().x : boundingBox->GetWorldOBB().HalfSize().z;
						height = boundingBox->GetWorldOBB().Size().MaxElement() - 2 * radius;
					}
					break;
				case CapsuleType::Z:
					if (boundingBox) {
						radius = (boundingBox->GetWorldOBB().HalfSize().x > boundingBox->GetWorldOBB().HalfSize().y) ? boundingBox->GetWorldOBB().HalfSize().x : boundingBox->GetWorldOBB().HalfSize().y;
						height = boundingBox->GetWorldOBB().Size().MaxElement() - 2 * radius;
					}
					break;
				}

				if (App->time->IsGameRunning()) {
					App->physics->UpdateCapsuleRigidbody(this);
				}
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::DragFloat3("Center Offset", centerOffset.ptr(), App->editor->dragSpeed2f, -inf, inf) && App->time->IsGameRunning()) {
		float3 position = GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition();
		Quat rotation = GetOwner().GetComponent<ComponentTransform>()->GetGlobalRotation();
		rigidBody->setCenterOfMassTransform(btTransform(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w), btVector3(position.x, position.y, position.z)) * btTransform(btQuaternion::getIdentity(), btVector3(centerOffset.x, centerOffset.y, centerOffset.z)));
	}

	if (ImGui::Checkbox("Freeze rotation", &freezeRotation) && App->time->IsGameRunning()) {
		motionState.freezeRotation = freezeRotation;
	}
}

void ComponentCapsuleCollider::Save(JsonValue jComponent) const {
	JsonValue jColliderType = jComponent[JSON_TAG_COLLIDER_TYPE];
	jColliderType = (int) colliderType;

	JsonValue jMass = jComponent[JSON_TAG_MASS];
	jMass = mass;

	JsonValue jRadius = jComponent[JSON_TAG_RADIUS];
	jRadius = radius;

	JsonValue jHeight = jComponent[JSON_TAG_HEIGHT];
	jHeight = height;

	JsonValue jType = jComponent[JSON_TAG_TYPE];
	jType = (int) type;

	JsonValue jCenterOffset = jComponent[JSON_TAG_CENTER_OFFSET];
	jCenterOffset[0] = centerOffset.x;
	jCenterOffset[1] = centerOffset.y;
	jCenterOffset[2] = centerOffset.z;

	JsonValue jFreeze = jComponent[JSON_TAG_FREEZE_ROTATION];
	jFreeze = freezeRotation;
}

void ComponentCapsuleCollider::Load(JsonValue jComponent) {
	JsonValue jColliderType = jComponent[JSON_TAG_COLLIDER_TYPE];
	colliderType = (ColliderType)(int) jColliderType;

	JsonValue jMass = jComponent[JSON_TAG_MASS];
	mass = jMass;

	JsonValue jRadius = jComponent[JSON_TAG_RADIUS];
	radius = jRadius;

	JsonValue jHeight = jComponent[JSON_TAG_HEIGHT];
	height = jHeight;

	JsonValue jType = jComponent[JSON_TAG_TYPE];
	type = (CapsuleType)(int) jType;

	JsonValue jCenterOffset = jComponent[JSON_TAG_CENTER_OFFSET];
	centerOffset = float3(jCenterOffset[0], jCenterOffset[1], jCenterOffset[2]);

	JsonValue jFreeze = jComponent[JSON_TAG_FREEZE_ROTATION];
	freezeRotation = jFreeze;
}

void ComponentCapsuleCollider::OnCollision() {
	// TODO: Send event...
}
