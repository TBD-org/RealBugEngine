#pragma once
#include "Module.h"

#include "Math/float4x4.h"
#include "btBulletDynamicsCommon.h"

/*BULLET DEBUG: Uncomment to activate it*/
class DebugDrawer;

class MotionState;
class btBroadphaseInterface;
class ComponentSphereCollider;
class ComponentBoxCollider;
class ComponentCapsuleCollider;
class btBroadphaseInterface;
enum class CapsuleType;

/* --- Collider Type ---
*	DYNAMIC = The object will respond to collisions, but not to user input (Such as modifying the transform).
*	STATIC = The object will never move.
*	KINEMATIC = The object will not respond to collisions, but it will to user input. Other Dynamic colliders (only) will phisically react to collisions with this object.
*	TRIGGER = It is like static, but the collisions against it have no physical effect to the colliding object.
*	Usage in our game: Kinematics for player, enemies and bullets. Static for any level prop inside the navmesh. Trigger for any game event callback.
*/
enum class ColliderType {
	DYNAMIC,
	STATIC,
	KINEMATIC,
	TRIGGER
};

/* --- World Layers ----
*	NO_COLLISION = Doesn't collide with anything.
*	WOLRD_ELEMENTS = All Objects that are physically present in the scene, including map props and enemies. Interaction with other WOLRD_ELEMENTS and PLAYER.
*	EVENT_TRIGGERS = All trigger colliders that will be activated by the player. Interaction with PLAYER.
*	PLAYER = The Player of the game, should only exist one of this type. Interaction with WORLD_ELEMENTS and EVENT_TRIGGERS. Doesnt itneract with itself.
*	EVERYTHING = The default setting. Interaction with all other types except NO_COLLISION.
*/
enum WorldLayers {
	NO_COLLISION = 1 << 1,
	EVENT_TRIGGERS = 1 << 2,
	WORLD_ELEMENTS = 1 << 3,
	PLAYER = 1 << 4,
	EVERYTHING = -1
};

class ModulePhysics : public Module {
public:
	// ------- Core Functions ------ //
	bool Init() override;
	UpdateStatus PreUpdate();
	UpdateStatus Update();
	bool CleanUp();

	// -- Add/Remove Sphere Body --- //
	void CreateSphereRigidbody(ComponentSphereCollider* sphereCollider);
	btRigidBody* AddSphereBody(MotionState* myMotionState, float radius, float mass);
	void RemoveSphereRigidbody(ComponentSphereCollider* sphereCollider);
	void UpdateSphereRigidbody(ComponentSphereCollider* sphereCollider);
	// ---- Add/Remove Box Body ---- //
	void CreateBoxRigidbody(ComponentBoxCollider* boxCollider);
	btRigidBody* AddBoxBody(MotionState* myMotionState, float3 size, float mass);
	void RemoveBoxRigidbody(ComponentBoxCollider* boxCollider);
	void UpdateBoxRigidbody(ComponentBoxCollider* boxCollider);
	// -- Add/Remove Capsule Body -- //
	void CreateCapsuleRigidbody(ComponentCapsuleCollider* capsuleCollider);
	btRigidBody* AddCapsuleBody(MotionState* myMotionState, float radius, float height, CapsuleType type, float mass);
	void RemoveCapsuleRigidbody(ComponentCapsuleCollider* capsuleCollider);
	void UpdateCapsuleRigidbody(ComponentCapsuleCollider* capsuleCollider);

	void AddBodyToWorld(btRigidBody* rigidbody, ColliderType colliderType, WorldLayers layer);

	void InitializeRigidBodies(); // Called on Play(), this function adds all the collision objects to the physics world.
	void ClearPhysicBodies();	  // Called on Stop(), this function removes all the collision objects from the physics world.

	// ----------- Setters --------- //
	void SetGravity(float newGravity);

public:
	float gravity = -9.81f;

private:
	float3 CalculateMeanPoint(btPersistentManifold* contactManifold, bool isFirstContact);

private:
	// ----- Physics World Config ----- //
	btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
	btCollisionDispatcher* dispatcher = nullptr;
	btBroadphaseInterface* broadPhase = nullptr;
	btSequentialImpulseConstraintSolver* constraintSolver = nullptr;
	btDiscreteDynamicsWorld* world = nullptr;

	/* BULLET DEBUG: Uncomment to activate it */
	DebugDrawer* debugDrawer;
	
	bool debug = true;
};

 //BULLET DEBUG: Uncomment to activate it
class DebugDrawer : public btIDebugDraw {
public:
	DebugDrawer() {}
	void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);
	void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color);
	void reportErrorWarning(const char* warningString);
	void draw3dText(const btVector3& location, const char* textString);
	void setDebugMode(int debugMode);
	int getDebugMode() const;

	DebugDrawModes mode; // How to initialise this enum?
};
