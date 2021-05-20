#include "ComponentBoxCollider.h"

#include "Application.h"
#include "GameObject.h"
#include "Modules/ModulePhysics.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleTime.h"
#include "Components/ComponentBoundingBox.h"

#include "debugdraw.h"

#define JSON_TAG_MASS "mass"
#define JSON_TAG_SIZE "size"
#define JSON_TAG_CENTER_OFFSET "centerOffset"
#define JSON_TAG_FREEZE_ROTATION "freezeRotation"
#define JSON_TAG_COLLIDER_TYPE "colliderType"

void ComponentBoxCollider::Init() {
	if (!centerOffset.IsFinite()) {
		ComponentBoundingBox* boundingBox = GetOwner().GetComponent<ComponentBoundingBox>();
		if (boundingBox) {
			size = boundingBox->GetWorldOBB().Size();
			centerOffset = boundingBox->GetWorldOBB().CenterPoint() - GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition();
		} else {
			centerOffset = float3::zero;
		}
	}

	localAABB.SetFromCenterAndSize(centerOffset, size);

	if (App->time->IsGameRunning() && !rigidBody) App->physics->CreateBoxRigidbody(this);
}

void ComponentBoxCollider::DrawGizmos() {
	if (IsActive()) {
		float3 points[8];
		// TODO: dirty{
		CalculateWorldBoundingBox();
		//}
		worldOBB.GetCornerPoints(points);

		float3 aux;
		aux = points[2];
		points[2] = points[3];
		points[3] = aux;
		aux = points[6];
		points[6] = points[7];
		points[7] = aux;

		dd::box(points, dd::colors::White);
	}
}

void ComponentBoxCollider::OnEditorUpdate() {
	// Collider Type combo box
	const char* colliderTypeItems[] = {"Dynamic", "Static", "Kinematic", "Trigger"};
	const char* colliderCurrent = colliderTypeItems[(int) colliderType];
	if (ImGui::BeginCombo("Collider Mode", colliderCurrent)) {
		for (int n = 0; n < IM_ARRAYSIZE(colliderTypeItems); ++n) {
			if (ImGui::Selectable(colliderTypeItems[n])) {
				colliderType = ColliderType(n);
				if (App->time->IsGameRunning()) {
					App->physics->UpdateBoxRigidbody(this);
				}
			}
		}
		ImGui::EndCombo();
	}

	if (colliderType == ColliderType::DYNAMIC) { // Mass is only available when the collider is dynamic
		if (ImGui::DragFloat("Mass", &mass, App->editor->dragSpeed3f, 0.0f, 100.f) && App->time->IsGameRunning()) {
			rigidBody->setMassProps(mass, btVector3(0, 0, 0));
		}
	}

	if (ImGui::DragFloat3("Size", size.ptr(), App->editor->dragSpeed3f, 0.0f, inf)) {
		if (App->time->IsGameRunning()) {
			((btBoxShape*) rigidBody->getCollisionShape())->setLocalScaling(btVector3(size.x, size.y, size.z));
		}
		localAABB.SetFromCenterAndSize(centerOffset, size);
	}

	if (ImGui::DragFloat3("Center Offset", centerOffset.ptr(), App->editor->dragSpeed2f, -inf, inf)) {
		if (App->time->IsGameRunning()) {
			float3 position = GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition();
			Quat rotation = GetOwner().GetComponent<ComponentTransform>()->GetGlobalRotation();
			rigidBody->setCenterOfMassTransform(btTransform(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w), btVector3(position.x, position.y, position.z)) * btTransform(btQuaternion::getIdentity(), btVector3(centerOffset.x, centerOffset.y, centerOffset.z)));
		}
		localAABB.SetFromCenterAndSize(centerOffset, size);
	}

	if (ImGui::Checkbox("Freeze rotation", &freezeRotation) && App->time->IsGameRunning()) {
		motionState.freezeRotation = freezeRotation;
	}
}

void ComponentBoxCollider::Save(JsonValue jComponent) const {
	JsonValue jColliderType = jComponent[JSON_TAG_COLLIDER_TYPE];
	jColliderType = (int) colliderType;

	JsonValue jMass = jComponent[JSON_TAG_MASS];
	jMass = mass;

	JsonValue jSize = jComponent[JSON_TAG_SIZE];
	jSize[0] = size.x;
	jSize[1] = size.y;
	jSize[2] = size.z;

	JsonValue jCenterOffset = jComponent[JSON_TAG_CENTER_OFFSET];
	jCenterOffset[0] = centerOffset.x;
	jCenterOffset[1] = centerOffset.y;
	jCenterOffset[2] = centerOffset.z;

	JsonValue jFreeze = jComponent[JSON_TAG_FREEZE_ROTATION];
	jFreeze = freezeRotation;

}

void ComponentBoxCollider::Load(JsonValue jComponent) {
	JsonValue jColliderType = jComponent[JSON_TAG_COLLIDER_TYPE];
	colliderType = (ColliderType)(int) jColliderType;

	JsonValue jMass = jComponent[JSON_TAG_MASS];
	mass = jMass;

	JsonValue jSize = jComponent[JSON_TAG_SIZE];
	size.Set(jSize[0], jSize[1], jSize[2]);

	JsonValue jCenterOffset = jComponent[JSON_TAG_CENTER_OFFSET];
	centerOffset = float3(jCenterOffset[0], jCenterOffset[1], jCenterOffset[2]);

	JsonValue jFreeze = jComponent[JSON_TAG_FREEZE_ROTATION];
	freezeRotation = jFreeze;
}

void ComponentBoxCollider::OnCollision() {
	// TODO: Send event...
}

void ComponentBoxCollider::CalculateWorldBoundingBox() {
	worldOBB = OBB(localAABB);
	worldOBB.Transform(GetOwner().GetComponent<ComponentTransform>()->GetGlobalMatrix());
}
