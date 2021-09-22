#include "ComponentSphereCollider.h"

#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleTime.h"
#include "Components/ComponentBoundingBox.h"

#include "debugdraw.h"

#include "Utils/Leaks.h"

#define JSON_TAG_MASS "mass"
#define JSON_TAG_RADIUS "radius"
#define JSON_TAG_CENTER_OFFSET "centerOffset"
#define JSON_TAG_FREEZE_ROTATION "freezeRotation"
#define JSON_TAG_COLLIDER_TYPE "colliderType"
#define JSON_TAG_LAYER_TYPE "layerType"

ComponentSphereCollider::~ComponentSphereCollider() {
	if (rigidBody) App->physics->RemoveSphereRigidbody(this);
}

void ComponentSphereCollider::Init() {
	if (!centerOffset.IsFinite()) {
		ComponentBoundingBox* boundingBox = GetOwner().GetComponent<ComponentBoundingBox>();
		if (boundingBox) {
			radius = boundingBox->GetWorldOBB().HalfSize().MaxElement();
			centerOffset = boundingBox->GetWorldOBB().CenterPoint() - GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition();
		} else {
			centerOffset = float3::zero;
		}
	}
}

void ComponentSphereCollider::Start() {
	if (!rigidBody && IsActive()) App->physics->CreateSphereRigidbody(this);
}

void ComponentSphereCollider::DrawGizmos() {
	if (IsActive()) {
		if (drawGizmo) {
			ComponentTransform* ownerTransform = GetOwner().GetComponent<ComponentTransform>();
			dd::sphere(ownerTransform->GetGlobalPosition() + ownerTransform->GetGlobalRotation() * centerOffset, dd::colors::LawnGreen, radius);
		}
	}
}

void ComponentSphereCollider::OnEditorUpdate() {
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

	ImGui::Checkbox("Draw Shape", &drawGizmo);

	// World Layers combo box
	const char* layerTypeItems[] = {"No Collision", "Event Triggers", "World Elements", "Player", "Enemy", "Bullet", "Bullet Enemy", "Skills", "Everything"};	// Increase if (n == X) when adding a new layer, a few lines down
	const char* layerCurrent = layerTypeItems[layerIndex];
	if (ImGui::BeginCombo("Layer", layerCurrent)) {
		for (int n = 0; n < IM_ARRAYSIZE(layerTypeItems); ++n) {
			if (ImGui::Selectable(layerTypeItems[n])) {
				layerIndex = n;
				if (n == 8) {
					layer = WorldLayers::EVERYTHING;
				} else {
					layer = WorldLayers(1 << layerIndex);
				}
				if (App->time->HasGameStarted()) {
					App->physics->UpdateSphereRigidbody(this);
				}
			}
		}
		ImGui::EndCombo();
	}

	// Collider Type combo box
	const char* colliderTypeItems[] = {"Dynamic", "Static", "Kinematic", "Trigger"};
	const char* colliderCurrent = colliderTypeItems[(int) colliderType];
	if (ImGui::BeginCombo("Collider Mode", colliderCurrent)) {
		for (int n = 0; n < IM_ARRAYSIZE(colliderTypeItems); ++n) {
			if (ImGui::Selectable(colliderTypeItems[n])) {
				colliderType = ColliderType(n);
				if (App->time->HasGameStarted()) {
					App->physics->UpdateSphereRigidbody(this);
				}
			}
		}
		ImGui::EndCombo();
	}

	if (colliderType == ColliderType::DYNAMIC) { // Mass is only available when the collider is dynamic
		if (ImGui::DragFloat("Mass", &mass, App->editor->dragSpeed3f, 0.0f, 100.f) && App->time->HasGameStarted()) {
			rigidBody->setMassProps(mass, rigidBody->getLocalInertia());
		}
	}
	if (ImGui::DragFloat("Radius", &radius, App->editor->dragSpeed3f, 0.0f, inf) && App->time->HasGameStarted()) {
		App->physics->UpdateSphereRigidbody(this);
	}
	if (ImGui::DragFloat3("Center Offset", centerOffset.ptr(), App->editor->dragSpeed2f, -inf, inf) && App->time->HasGameStarted()) {
		float3 position = GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition();
		Quat rotation = GetOwner().GetComponent<ComponentTransform>()->GetGlobalRotation();
		rigidBody->setCenterOfMassTransform(btTransform(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w), btVector3(position.x, position.y, position.z)) * btTransform(btQuaternion::getIdentity(), btVector3(centerOffset.x, centerOffset.y, centerOffset.z)));
	}
	if (ImGui::Checkbox("Freeze rotation", &freezeRotation) && App->time->HasGameStarted()) {
		motionState.freezeRotation = freezeRotation;
	}
}

void ComponentSphereCollider::Save(JsonValue jComponent) const {
	JsonValue jColliderType = jComponent[JSON_TAG_COLLIDER_TYPE];
	jColliderType = (int) colliderType;

	JsonValue jLayerType = jComponent[JSON_TAG_LAYER_TYPE];
	jLayerType = (int) layerIndex;

	JsonValue jMass = jComponent[JSON_TAG_MASS];
	jMass = mass;

	JsonValue jRadius = jComponent[JSON_TAG_RADIUS];
	jRadius = radius;

	JsonValue jCenterOffset = jComponent[JSON_TAG_CENTER_OFFSET];
	jCenterOffset[0] = centerOffset.x;
	jCenterOffset[1] = centerOffset.y;
	jCenterOffset[2] = centerOffset.z;

	JsonValue jFreeze = jComponent[JSON_TAG_FREEZE_ROTATION];
	jFreeze = freezeRotation;
}

void ComponentSphereCollider::Load(JsonValue jComponent) {
	JsonValue jColliderType = jComponent[JSON_TAG_COLLIDER_TYPE];
	colliderType = (ColliderType)(int) jColliderType;

	JsonValue jLayerType = jComponent[JSON_TAG_LAYER_TYPE];
	layerIndex = (int) jLayerType;
	layer = WorldLayers(1 << layerIndex);

	JsonValue jMass = jComponent[JSON_TAG_MASS];
	mass = jMass;

	JsonValue jRadius = jComponent[JSON_TAG_RADIUS];
	radius = jRadius;

	JsonValue jCenterOffset = jComponent[JSON_TAG_CENTER_OFFSET];
	centerOffset = float3(jCenterOffset[0], jCenterOffset[1], jCenterOffset[2]);

	JsonValue jFreeze = jComponent[JSON_TAG_FREEZE_ROTATION];
	freezeRotation = jFreeze;
}

void ComponentSphereCollider::OnEnable() {
	if (!rigidBody && App->time->HasGameStarted() && App->scene->GetCurrentScene() == GetOwner().scene) App->physics->CreateSphereRigidbody(this);
}

void ComponentSphereCollider::OnDisable() {
	if (rigidBody && App->time->HasGameStarted() && App->scene->GetCurrentScene() == GetOwner().scene) App->physics->RemoveSphereRigidbody(this);
}

void ComponentSphereCollider::OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, ComponentParticleSystem::Particle* p) {
	if (p != nullptr) {
		bool alreadyCollided = false;
		for (GameObject* collided : p->collidedWith) {
			if (collided == &GetOwner()) {
				alreadyCollided = true;
			}
		}
		if (!alreadyCollided) {
			p->collidedWith.push_back(&GetOwner());
			p->hasCollided = true;
		} else {
			p->hasCollided = false;
		}
	}

	for (ComponentScript& scriptComponent : GetOwner().GetComponents<ComponentScript>()) {
		Script* script = scriptComponent.GetScriptInstance();
		if (script != nullptr) {
			script->OnCollision(collidedWith, collisionNormal, penetrationDistance, p);
		}
	}
}
