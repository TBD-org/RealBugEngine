#pragma once
#include "Module.h"

#include "Math/float4x4.h"
#include "btBulletDynamicsCommon.h"

class DebugDrawer;
class MotionState;
class btBroadphaseInterface;
class ComponentSphereCollider;
class ComponentBoxCollider;
class ComponentCapsuleCollider;
class btBroadphaseInterface;
enum class CapsuleType;

/* --- Collider Type ---
	DYNAMIC = The object will respond to collisions, but not to user input (Such as modifying the transform).
	STATIC = The object will never move.
	KINEMATIC = The object will not respond to collisions, but it will to user input. Other Dynamic colliders (only) will phisically react to collisions with this object.
	TRIGGER = It is like static, but the collisions against it have no physical effect to the colliding object.
	*/
enum class ColliderType {
	DYNAMIC,
	STATIC,
	KINEMATIC,
	TRIGGER
};

class ModulePhysics : public Module {

public:
	// ------- Core Functions ------ //
	bool Init() override;
	//bool Start();
	UpdateStatus PreUpdate();
	UpdateStatus Update();
	//UpdateStatus PostUpdate();
	bool CleanUp();
	//void ReceiveEvent(TesseractEvent& e);

	// ------ Add/Remove Sphere Body ------ //
	void CreateSphereRigidbody(ComponentSphereCollider* sphereCollider);
	btRigidBody* AddSphereBody(MotionState* myMotionState, float radius, float mass);
	void RemoveSphereRigidbody(ComponentSphereCollider* sphereCollider);
	void UpdateSphereRigidbody(ComponentSphereCollider* sphereCollider);
	// ------ Add/Remove Box Body ------ //
	void CreateBoxRigidbody(ComponentBoxCollider* boxCollider);
	btRigidBody* AddBoxBody(MotionState* myMotionState, float3 size, float mass);
	void RemoveBoxRigidbody(ComponentBoxCollider* boxCollider);
	void UpdateBoxRigidbody(ComponentBoxCollider* boxCollider);

	void CreateCapsuleRigidbody(ComponentCapsuleCollider* capsuleCollider);
	btRigidBody* AddCapsuleBody(MotionState* myMotionState, float radius, float height, CapsuleType type, float mass);
	void RemoveCapsuleRigidbody(ComponentCapsuleCollider* capsuleCollider);
	void UpdateCapsuleRigidbody(ComponentCapsuleCollider* capsuleCollider);

	void InitializeRigidBodies();
	void ClearPhysicBodies();

	// ----------- Setters --------- //
	void SetGravity(float newGravity);

public:
	float gravity = -9.81f;

private:
	btDiscreteDynamicsWorld* world = nullptr;

	btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
	btCollisionDispatcher* dispatcher = nullptr;
	btBroadphaseInterface* broadPhase = nullptr;
	btSequentialImpulseConstraintSolver* constraintSolver = nullptr;

	DebugDrawer* debugDrawer;

	bool debug = true;

	/*p2List<btCollisionShape*> shapes;
	p2List<PhysBody3D*> bodies;
	p2List<btDefaultMotionState*> motions;
	p2List<btTypedConstraint*> constraints;*/
};

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