#include "ModulePhysics.h"

#include "Application.h"
#include "ModuleTime.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "Components/Physics/ComponentSphereCollider.h"
#include "Components/Physics/ComponentCapsuleCollider.h"
#include "Components/Physics/ComponentBoxCollider.h"
#include "Components/ComponentTransform.h"
#include "Scene.h"
#include "Utils/MotionState.h"
#include "Utils/Logging.h"
#include "debugdraw.h"

bool ModulePhysics::Init() {
	LOG("Creating Physics environment using Bullet Physics.");

	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	broadPhase = new btDbvtBroadphase();
	constraintSolver = new btSequentialImpulseConstraintSolver();
	world = new btDiscreteDynamicsWorld(dispatcher, broadPhase, constraintSolver, collisionConfiguration);
	world->setGravity(btVector3(0.f, gravity, 0.f));

	/* BULLET DEBUG: Uncomment to activate it*/
	debugDrawer = new DebugDrawer();
	world->setDebugDrawer(debugDrawer);
	
	return true;
}

UpdateStatus ModulePhysics::PreUpdate() {
	if (App->time->IsGameRunning()) {
		world->stepSimulation(App->time->GetDeltaTime(), 15);

		int numManifolds = world->getDispatcher()->getNumManifolds();
		for (int i = 0; i < numManifolds; i++) {
			btPersistentManifold* contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
			btCollisionObject* obA = (btCollisionObject*) (contactManifold->getBody0());
			btCollisionObject* obB = (btCollisionObject*) (contactManifold->getBody1());

			int numContacts = contactManifold->getNumContacts();
			if (numContacts > 0) {
				Component* pbodyA = (Component*) obA->getUserPointer();
				Component* pbodyB = (Component*) obB->getUserPointer();

				if (pbodyA && pbodyB) {
					float3 collisionNormal = float3(contactManifold->getContactPoint(0).m_normalWorldOnB).Normalized();
					// Contact point is in global coordinates
			/*		float3 contactPoint = pbodyB->GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition() 
						                + pbodyB->GetOwner().GetComponent<ComponentTransform>()->GetGlobalRotation().Transform(float3(contactManifold->getContactPoint(0).m_localPointB));*/

					
					//float3 contactPoint = pbodyB->GetOwner().GetComponent<ComponentTransform>()->GetGlobalMatrix().MulPos(float3(contactManifold->getContactPoint(0).m_localPointB));
					float3 contactPoint = pbodyB->GetOwner().GetComponent<ComponentTransform>()->GetGlobalMatrix().MulPos(CalculateMeanPoint(contactManifold, true));
					switch (pbodyA->GetType()) {
					case ComponentType::SPHERE_COLLIDER: {
						ComponentSphereCollider* sphereCol = (ComponentSphereCollider*) pbodyA;
						sphereCol->OnCollision(pbodyB->GetOwner(), contactPoint, collisionNormal);
						break;
					}
					case ComponentType::BOX_COLLIDER: {
						ComponentBoxCollider* boxCol = (ComponentBoxCollider*) pbodyA;
						boxCol->OnCollision(pbodyB->GetOwner(), contactPoint, collisionNormal);
						break;
					}
					case ComponentType::CAPSULE_COLLIDER: {
						ComponentCapsuleCollider* capsuleCol = (ComponentCapsuleCollider*) pbodyA;
						capsuleCol->OnCollision(pbodyB->GetOwner(), contactPoint, collisionNormal);
						break;
					}
					default:
						break;
					}

				/*	contactPoint = pbodyA->GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition()
								 + pbodyA->GetOwner().GetComponent<ComponentTransform>()->GetGlobalRotation().Transform(float3(contactManifold->getContactPoint(0).m_localPointA));*/

					//contactPoint = pbodyB->GetOwner().GetComponent<ComponentTransform>()->GetGlobalMatrix().MulPos(float3(contactManifold->getContactPoint(0).m_localPointA));
					contactPoint = pbodyB->GetOwner().GetComponent<ComponentTransform>()->GetGlobalMatrix().MulPos(CalculateMeanPoint(contactManifold, false));

					switch (pbodyB->GetType()) {
					case ComponentType::SPHERE_COLLIDER: {
						ComponentSphereCollider* sphereCol = (ComponentSphereCollider*) pbodyB;
						sphereCol->OnCollision(pbodyA->GetOwner(), contactPoint, -collisionNormal);
						break;
					}
					case ComponentType::BOX_COLLIDER: {
						ComponentBoxCollider* boxCol = (ComponentBoxCollider*) pbodyB;
						boxCol->OnCollision(pbodyA->GetOwner(), contactPoint, -collisionNormal);
						break;
					}
					case ComponentType::CAPSULE_COLLIDER: {
						ComponentCapsuleCollider* capsuleCol = (ComponentCapsuleCollider*) pbodyB;
						capsuleCol->OnCollision(pbodyA->GetOwner(), contactPoint, -collisionNormal);
						break;
					}
					default:
						break;
					}
				}
			}
		}
	}
	return UpdateStatus::CONTINUE;
}

UpdateStatus ModulePhysics::Update() {
	/* BULLET DEBUG: Uncomment to activate it */
	if (debug == true) {
		world->debugDrawWorld();
	}
	
	return UpdateStatus::CONTINUE;
}

bool ModulePhysics::CleanUp() {
	ClearPhysicBodies();

	RELEASE(world);

	/* BULLET DEBUG: Uncomment to activate it */
	RELEASE(debugDrawer);
	RELEASE(constraintSolver);
	RELEASE(broadPhase);
	RELEASE(dispatcher);
	RELEASE(collisionConfiguration);

	return true;
}

void ModulePhysics::CreateSphereRigidbody(ComponentSphereCollider* sphereCollider) {
	sphereCollider->motionState = MotionState(sphereCollider, sphereCollider->centerOffset, sphereCollider->freezeRotation);
	sphereCollider->rigidBody = AddSphereBody(&sphereCollider->motionState, sphereCollider->radius, sphereCollider->colliderType == ColliderType::DYNAMIC ? sphereCollider->mass : 0);
	sphereCollider->rigidBody->setUserPointer(sphereCollider);
	AddBodyToWorld(sphereCollider->rigidBody, sphereCollider->colliderType, sphereCollider->layer);
}

btRigidBody* ModulePhysics::AddSphereBody(MotionState* myMotionState, float radius, float mass) {
	btCollisionShape* colShape = new btSphereShape(radius);

	btVector3 localInertia(0, 0, 0);
	if (mass != 0.f) {
		colShape->calculateLocalInertia(mass, localInertia);
	}

	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);

	return body;
}

void ModulePhysics::RemoveSphereRigidbody(ComponentSphereCollider* sphereCollider) {
	world->removeCollisionObject(sphereCollider->rigidBody);
	RELEASE(sphereCollider->rigidBody);
}

void ModulePhysics::UpdateSphereRigidbody(ComponentSphereCollider* sphereCollider) {
	RemoveSphereRigidbody(sphereCollider);
	CreateSphereRigidbody(sphereCollider);
}

void ModulePhysics::CreateBoxRigidbody(ComponentBoxCollider* boxCollider) {
	boxCollider->motionState = MotionState(boxCollider, boxCollider->centerOffset, boxCollider->freezeRotation);
	boxCollider->rigidBody = AddBoxBody(&boxCollider->motionState, boxCollider->size / 2, boxCollider->colliderType == ColliderType::DYNAMIC ? boxCollider->mass : 0);
	boxCollider->rigidBody->setUserPointer(boxCollider);
	AddBodyToWorld(boxCollider->rigidBody, boxCollider->colliderType, boxCollider->layer);
}

btRigidBody* ModulePhysics::AddBoxBody(MotionState* myMotionState, float3 size, float mass) {
	btCollisionShape* colShape = new btBoxShape(btVector3(size.x, size.y, size.z));

	btVector3 localInertia(0, 0, 0);
	if (mass != 0.f) {
		colShape->calculateLocalInertia(mass, localInertia);
	}

	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);

	return body;
}

void ModulePhysics::RemoveBoxRigidbody(ComponentBoxCollider* boxCollider) {
	world->removeCollisionObject(boxCollider->rigidBody);
	RELEASE(boxCollider->rigidBody);
}

void ModulePhysics::UpdateBoxRigidbody(ComponentBoxCollider* boxCollider) {
	RemoveBoxRigidbody(boxCollider);
	CreateBoxRigidbody(boxCollider);
}

void ModulePhysics::CreateCapsuleRigidbody(ComponentCapsuleCollider* capsuleCollider) {
	capsuleCollider->motionState = MotionState(capsuleCollider, capsuleCollider->centerOffset, capsuleCollider->freezeRotation);
	capsuleCollider->rigidBody = AddCapsuleBody(&capsuleCollider->motionState, capsuleCollider->radius, capsuleCollider->height, capsuleCollider->capsuleType, capsuleCollider->colliderType == ColliderType::DYNAMIC ? capsuleCollider->mass : 0);
	capsuleCollider->rigidBody->setUserPointer(capsuleCollider);
	AddBodyToWorld(capsuleCollider->rigidBody, capsuleCollider->colliderType, capsuleCollider->layer);
}

btRigidBody* ModulePhysics::AddCapsuleBody(MotionState* myMotionState, float radius, float height, CapsuleType type, float mass) {
	btCollisionShape* colShape = nullptr;

	switch (type) {
	case CapsuleType::X:
		colShape = new btCapsuleShapeX(radius, height);
		break;
	case CapsuleType::Y:
		colShape = new btCapsuleShape(radius, height);
		break;
	case CapsuleType::Z:
		colShape = new btCapsuleShapeZ(radius, height);
		break;
	}

	btVector3 localInertia(0, 0, 0);
	if (mass != 0.f) {
		colShape->calculateLocalInertia(mass, localInertia);
	}

	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);

	return body;
}

void ModulePhysics::RemoveCapsuleRigidbody(ComponentCapsuleCollider* capsuleCollider) {
	world->removeCollisionObject(capsuleCollider->rigidBody);
	RELEASE(capsuleCollider->rigidBody);
}

void ModulePhysics::UpdateCapsuleRigidbody(ComponentCapsuleCollider* capsuleCollider) {
	RemoveCapsuleRigidbody(capsuleCollider);
	CreateCapsuleRigidbody(capsuleCollider);
}

void ModulePhysics::AddBodyToWorld(btRigidBody* rigidbody, ColliderType colliderType, WorldLayers layer) {
	switch (colliderType) {
	case ColliderType::STATIC:
		rigidbody->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
		break;
	case ColliderType::KINEMATIC:
		rigidbody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
		rigidbody->setActivationState(DISABLE_DEACTIVATION);
		break;
	case ColliderType::TRIGGER:
		rigidbody->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | btCollisionObject::CF_NO_CONTACT_RESPONSE);
		break;
	default:
		break;
	}

	short collisionMask = 0;
	switch (layer) {
	case EVENT_TRIGGERS:
		collisionMask = WorldLayers::PLAYER | WorldLayers::EVERYTHING;
		break;
	case WORLD_ELEMENTS:
		collisionMask = WorldLayers::WORLD_ELEMENTS | WorldLayers::PLAYER | WorldLayers::EVERYTHING;
		break;
	case PLAYER:
		collisionMask = WorldLayers::EVENT_TRIGGERS | WorldLayers::WORLD_ELEMENTS | WorldLayers::EVERYTHING;
		break;
	case EVERYTHING:
		collisionMask = WorldLayers::EVENT_TRIGGERS | WorldLayers::WORLD_ELEMENTS | WorldLayers::PLAYER | WorldLayers::EVERYTHING;
		break;
	default: //NO_COLLISION
		collisionMask = 0;
		break;
	}

	world->addRigidBody(rigidbody, layer, collisionMask);
}

void ModulePhysics::InitializeRigidBodies() {
	for (ComponentSphereCollider& sphereCollider : App->scene->scene->sphereColliderComponents) {
		if (!sphereCollider.rigidBody) CreateSphereRigidbody(&sphereCollider);
	}

	for (ComponentBoxCollider& boxCollider : App->scene->scene->boxColliderComponents) {
		if (!boxCollider.rigidBody) CreateBoxRigidbody(&boxCollider);
	}

	for (ComponentCapsuleCollider& capsuleCollider : App->scene->scene->capsuleColliderComponents) {
		if (!capsuleCollider.rigidBody) CreateCapsuleRigidbody(&capsuleCollider);
	}
}

void ModulePhysics::ClearPhysicBodies() {
	for (int i = world->getNumCollisionObjects() - 1; i >= 0; i--) {
		btCollisionObject* obj = world->getCollisionObjectArray()[i];
		world->removeCollisionObject(obj);
		RELEASE(obj);
	}
}

void ModulePhysics::SetGravity(float newGravity) {
	world->setGravity(btVector3(0.f, newGravity, 0.f));
}

float3 ModulePhysics::CalculateMeanPoint(btPersistentManifold* contactManifold, bool isFirstContact) {
	float3 result = float3(0, 0, 0);
	for (int i = 0; i < contactManifold->getNumContacts(); ++i) {
		result += float3(isFirstContact ? contactManifold->getContactPoint(i).m_localPointB : contactManifold->getContactPoint(i).m_localPointA);
	}
	result /= contactManifold->getNumContacts();
	return result;
}

/* BULLET DEBUG: Uncomment to activate it. #include "debugdraw.h" in this file if using it.
// =================== BULLET DEBUG CALLBACKS ==========================*/
void DebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
	dd::line((ddVec3) from, (ddVec3) to, (ddVec3) color); // TODO: Test if this actually works
}

void DebugDrawer::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {
	dd::point((ddVec3) PointOnB, (ddVec3) color);
}

void DebugDrawer::reportErrorWarning(const char* warningString) {
	LOG("Bullet warning: %s", warningString);
}

void DebugDrawer::draw3dText(const btVector3& location, const char* textString) {
	LOG("Bullet draw text: %s", textString);
}

void DebugDrawer::setDebugMode(int debugMode) {
	mode = (DebugDrawModes) debugMode;
}

int DebugDrawer::getDebugMode() const {
	return mode;
}

