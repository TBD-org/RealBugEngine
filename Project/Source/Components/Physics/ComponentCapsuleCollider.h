#pragma once

#include "Components/Component.h"
#include "Modules/ModulePhysics.h"
#include "Utils/MotionState.h"
#include "Utils/Collider.h"

#include "Math/float3.h"

class btRigidBody;

enum class CapsuleType {
	X,
	Y,
	Z
};

class ComponentCapsuleCollider : public Component {
public:
	REGISTER_COMPONENT(ComponentCapsuleCollider, ComponentType::CAPSULE_COLLIDER, false); // Refer to ComponentType for the Constructor
	~ComponentCapsuleCollider();

	// ------- Core Functions ------ //
	void Init() override;
	void Start() override;
	void DrawGizmos() override;
	void OnEditorUpdate() override;
	void Save(JsonValue jComponent) const override;
	void Load(JsonValue jComponent) override;
	void OnEnable() override;
	void OnDisable() override;

	// ----- Collider Functions ---- //
	void OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, ComponentParticleSystem::Particle* p = nullptr); // Callback function for scripting

public:
	btRigidBody* rigidBody = nullptr;										// Body that is represented in the physic world.
	MotionState motionState = MotionState(nullptr, float3(0, 0, 0), false); // Interface between btTransform (transform in physical world) and ComponentTransform.
	ColliderType colliderType = ColliderType::DYNAMIC;						// DYNAMIC, STATIC, KINEMATIC, TRIGGER. Refer to description in ModulePhysics.h
	WorldLayers layer = WorldLayers::EVERYTHING;							// Interaction layers. Refer to ModulePhysics.h to see how objects in a layer interact between the others.
	int layerIndex = 4;														// WorldLayers is a bitwise enum. This index is used to generate the dropdown selection box in the Inspector.
	float mass = 1.f;														// Represents the weight of the GameObject. If set to 0 the object will act as STATIC.
	float radius = 1.f;														// Radius of the Rigidbody two hemisphere shapes.
	float height = 1.f;														// Height of the Rigidbody inner cylinder shape.
	float3 centerOffset = float3::inf;										// Center of the Rigidbody shape relative to GameObject origin.
	bool freezeRotation = false;											// If set to true, the GameObject will not rotate when interacted on a collision.
	bool drawGizmo = true;
	Collider col{ this, typeid(Component) };

	CapsuleType capsuleType = CapsuleType::Y; // Defines the initial orientation of the capule. By default (Y) the cilinder is vertical and the hemispheres ara above and below it.
};
