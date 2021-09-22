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
#include "Utils/ParticleMotionState.h"
#include "Utils/Logging.h"
#include "Utils/Collider.h"

#include "debugdraw.h"

bool ModulePhysics::Init() {
	LOG("Creating Physics environment using Bullet Physics.");

	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	broadPhase = new btDbvtBroadphase();
	constraintSolver = new btSequentialImpulseConstraintSolver();
	world = new btDiscreteDynamicsWorld(dispatcher, broadPhase, constraintSolver, collisionConfiguration);
	world->setGravity(btVector3(0.f, gravity, 0.f));

	// BULLET DEBUG: Uncomment to activate it
	/*debugDrawer = new DebugDrawer();
	world->setDebugDrawer(debugDrawer);*/

	return true;
}

UpdateStatus ModulePhysics::PreUpdate() {
	for (btRigidBody* rigidBody : rigidBodiesToRemove) {
		world->removeCollisionObject(rigidBody);
		btCollisionShape* shape = rigidBody->getCollisionShape();
		RELEASE(shape);
		delete rigidBody;
	}
	rigidBodiesToRemove.clear();

	if (App->time->HasGameStarted()) {
		world->stepSimulation(App->time->GetDeltaTime(), 15);

		int numManifolds = world->getDispatcher()->getNumManifolds();
		for (int i = 0; i < numManifolds; i++) {
			btPersistentManifold* contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
			btCollisionObject* obA = (btCollisionObject*) (contactManifold->getBody0());
			btCollisionObject* obB = (btCollisionObject*) (contactManifold->getBody1());

			int numContacts = contactManifold->getNumContacts();
			if (numContacts > 0) {
				Collider* bodyA = (Collider*) obA->getUserPointer();
				Collider* bodyB = (Collider*) obB->getUserPointer();

				if (bodyA && bodyB) {
					float3 contactOnA = float3(contactManifold->getContactPoint(0).getPositionWorldOnA());
					float3 contactOnB = float3(contactManifold->getContactPoint(0).getPositionWorldOnB());
					float3 diff = contactOnB - contactOnA;

					float3 collisionNormal = float3(contactManifold->getContactPoint(0).m_normalWorldOnB);

					if (bodyA->tid == typeid(Component)) {
						Component* pbodyA = (Component*) bodyA->collider;
						switch (pbodyA->GetType()) {
						case ComponentType::SPHERE_COLLIDER: {
							ComponentSphereCollider* sphereCol = (ComponentSphereCollider*) pbodyA;

							// Different casts whether it is a Component collider or a particle
							if (bodyB->tid == typeid(Component)) {
								Component* pbodyB = (Component*) bodyB->collider;
								sphereCol->OnCollision(pbodyB->GetOwner(), collisionNormal, diff);
							} else {
								ComponentParticleSystem::Particle* pbodyB = (ComponentParticleSystem::Particle*) bodyB->collider;
								sphereCol->OnCollision(pbodyB->emitter->GetOwner(), collisionNormal, diff, pbodyB);
							}
							break;
						}
						case ComponentType::BOX_COLLIDER: {
							ComponentBoxCollider* boxCol = (ComponentBoxCollider*) pbodyA;

							if (bodyB->tid == typeid(Component)) {
								Component* pbodyB = (Component*) bodyB->collider;
								boxCol->OnCollision(pbodyB->GetOwner(), collisionNormal, diff);
							} else {
								ComponentParticleSystem::Particle* pbodyB = (ComponentParticleSystem::Particle*) bodyB->collider;
								boxCol->OnCollision(pbodyB->emitter->GetOwner(), collisionNormal, diff, pbodyB);
							}

							break;
						}
						case ComponentType::CAPSULE_COLLIDER: {
							ComponentCapsuleCollider* capsuleCol = (ComponentCapsuleCollider*) pbodyA;

							if (bodyB->tid == typeid(Component)) {
								Component* pbodyB = (Component*) bodyB->collider;
								capsuleCol->OnCollision(pbodyB->GetOwner(), collisionNormal, diff);
							} else {
								ComponentParticleSystem::Particle* pbodyB = (ComponentParticleSystem::Particle*) bodyB->collider;
								capsuleCol->OnCollision(pbodyB->emitter->GetOwner(), collisionNormal, diff, pbodyB);
							}
							break;
						}
						default:
							break;
						}
					}

					if (bodyB->tid == typeid(Component)) {
						Component* pbodyB = (Component*) bodyB->collider;
						switch (pbodyB->GetType()) {
						case ComponentType::SPHERE_COLLIDER: {
							ComponentSphereCollider* sphereCol = (ComponentSphereCollider*) pbodyB;

							if (bodyA->tid == typeid(Component)) {
								Component* pbodyA = (Component*) bodyA->collider;
								sphereCol->OnCollision(pbodyA->GetOwner(), -collisionNormal, -diff);
							} else {
								ComponentParticleSystem::Particle* pbodyA = (ComponentParticleSystem::Particle*) bodyA->collider;
								sphereCol->OnCollision(pbodyA->emitter->GetOwner(), -collisionNormal, -diff, pbodyA);
							}
							break;
						}
						case ComponentType::BOX_COLLIDER: {
							ComponentBoxCollider* boxCol = (ComponentBoxCollider*) pbodyB;

							if (bodyA->tid == typeid(Component)) {
								Component* pbodyA = (Component*) bodyA->collider;
								boxCol->OnCollision(pbodyA->GetOwner(), -collisionNormal, -diff);
							} else {
								ComponentParticleSystem::Particle* pbodyA = (ComponentParticleSystem::Particle*) bodyA->collider;
								boxCol->OnCollision(pbodyA->emitter->GetOwner(), -collisionNormal, -diff, pbodyA);
							}
							break;
						}
						case ComponentType::CAPSULE_COLLIDER: {
							ComponentCapsuleCollider* capsuleCol = (ComponentCapsuleCollider*) pbodyB;

							if (bodyA->tid == typeid(Component)) {
								Component* pbodyA = (Component*) bodyA->collider;
								capsuleCol->OnCollision(pbodyA->GetOwner(), -collisionNormal, -diff);
							} else {
								ComponentParticleSystem::Particle* pbodyA = (ComponentParticleSystem::Particle*) bodyA->collider;
								capsuleCol->OnCollision(pbodyA->emitter->GetOwner(), -collisionNormal, -diff, pbodyA);
							}
							break;
						}
						default:
							break;
						}
					}
				}
			}
		}
	}
	return UpdateStatus::CONTINUE;
}

UpdateStatus ModulePhysics::Update() {
	// BULLET DEBUG: Uncomment to activate it
	/*if (debug == true) {
		world->debugDrawWorld();
	}*/

	return UpdateStatus::CONTINUE;
}

bool ModulePhysics::CleanUp() {
	RELEASE(world);

	/* BULLET DEBUG: Uncomment to activate it
	RELEASE(debugDrawer);
	*/
	RELEASE(constraintSolver);
	RELEASE(broadPhase);
	RELEASE(dispatcher);
	RELEASE(collisionConfiguration);

	return true;
}

void ModulePhysics::CreateSphereRigidbody(ComponentSphereCollider* sphereCollider) {
	sphereCollider->motionState = MotionState(sphereCollider, sphereCollider->centerOffset, sphereCollider->freezeRotation);
	sphereCollider->rigidBody = AddSphereBody(&sphereCollider->motionState, sphereCollider->radius, sphereCollider->colliderType == ColliderType::DYNAMIC ? sphereCollider->mass : 0);
	sphereCollider->rigidBody->setUserPointer(&sphereCollider->col);
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
	if (sphereCollider->rigidBody) {
		rigidBodiesToRemove.push_back(sphereCollider->rigidBody);
		sphereCollider->rigidBody = nullptr;
	}
}

void ModulePhysics::UpdateSphereRigidbody(ComponentSphereCollider* sphereCollider) {
	RemoveSphereRigidbody(sphereCollider);
	CreateSphereRigidbody(sphereCollider);
}

void ModulePhysics::CreateBoxRigidbody(ComponentBoxCollider* boxCollider) {
	boxCollider->motionState = MotionState(boxCollider, boxCollider->centerOffset, boxCollider->freezeRotation);
	boxCollider->rigidBody = AddBoxBody(&boxCollider->motionState, boxCollider->size / 2, boxCollider->colliderType == ColliderType::DYNAMIC ? boxCollider->mass : 0);
	boxCollider->rigidBody->setUserPointer(&boxCollider->col);
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
	if (boxCollider->rigidBody) {
		rigidBodiesToRemove.push_back(boxCollider->rigidBody);
		boxCollider->rigidBody = nullptr;
	}
}

void ModulePhysics::UpdateBoxRigidbody(ComponentBoxCollider* boxCollider) {
	RemoveBoxRigidbody(boxCollider);
	CreateBoxRigidbody(boxCollider);
}

void ModulePhysics::CreateCapsuleRigidbody(ComponentCapsuleCollider* capsuleCollider) {
	capsuleCollider->motionState = MotionState(capsuleCollider, capsuleCollider->centerOffset, capsuleCollider->freezeRotation);
	capsuleCollider->rigidBody = AddCapsuleBody(&capsuleCollider->motionState, capsuleCollider->radius, capsuleCollider->height, capsuleCollider->capsuleType, capsuleCollider->colliderType == ColliderType::DYNAMIC ? capsuleCollider->mass : 0);
	capsuleCollider->rigidBody->setUserPointer(&capsuleCollider->col);
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
	if (mass != 0.f && colShape) {
		colShape->calculateLocalInertia(mass, localInertia);
	}

	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);

	return body;
}

void ModulePhysics::RemoveCapsuleRigidbody(ComponentCapsuleCollider* capsuleCollider) {
	if (capsuleCollider->rigidBody) {
		rigidBodiesToRemove.push_back(capsuleCollider->rigidBody);
		capsuleCollider->rigidBody = nullptr;
	}
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

	int collisionMask = 0;
	switch (layer) {
	case WorldLayers::EVENT_TRIGGERS:
		collisionMask = (int) WorldLayers::PLAYER | (int) WorldLayers::EVERYTHING;
		break;
	case WorldLayers::ENEMY:
		collisionMask = (int) WorldLayers::WORLD_ELEMENTS | (int) WorldLayers::BULLET | (int) WorldLayers::SKILLS | (int) WorldLayers::EVERYTHING;
		break;
	case WorldLayers::BULLET:
		collisionMask = (int) WorldLayers::WORLD_ELEMENTS | (int) WorldLayers::ENEMY | (int) WorldLayers::EVERYTHING;
		break;
	case WorldLayers::BULLET_ENEMY:
		collisionMask = (int) WorldLayers::WORLD_ELEMENTS | (int) WorldLayers::PLAYER | (int) WorldLayers::SKILLS | (int) WorldLayers::EVERYTHING;
		break;
	case WorldLayers::WORLD_ELEMENTS:
		collisionMask = (int) WorldLayers::WORLD_ELEMENTS | (int) WorldLayers::PLAYER | (int) WorldLayers::ENEMY | (int) WorldLayers::BULLET | (int) WorldLayers::BULLET_ENEMY | (int) WorldLayers::EVERYTHING;
		break;
	case WorldLayers::PLAYER:
		collisionMask = (int) WorldLayers::EVENT_TRIGGERS | (int) WorldLayers::WORLD_ELEMENTS | (int) WorldLayers::BULLET_ENEMY | (int) WorldLayers::EVERYTHING;
		break;
	case WorldLayers::SKILLS:
		collisionMask = (int) WorldLayers::BULLET_ENEMY | (int) WorldLayers::ENEMY | (int) WorldLayers::EVERYTHING;
		break;
	case WorldLayers::EVERYTHING:
		collisionMask = (int) WorldLayers::EVENT_TRIGGERS | (int) WorldLayers::WORLD_ELEMENTS | (int) WorldLayers::PLAYER | (int) WorldLayers::ENEMY | (int) WorldLayers::BULLET | (int) WorldLayers::BULLET_ENEMY | (int) WorldLayers::SKILLS | (int) WorldLayers::EVERYTHING;
		break;
	default: //NO_COLLISION
		collisionMask = 0;
		break;
	}

	world->addRigidBody(rigidbody, (short) layer, collisionMask);
}

void ModulePhysics::CreateParticleRigidbody(ComponentParticleSystem::Particle* currentParticle) {
	currentParticle->motionState = new ParticleMotionState(currentParticle);

	// Create rigidbody
	btCollisionShape* colShape = new btSphereShape(currentParticle->radius);
	btVector3 localInertia(0, 0, 0);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(0.f, currentParticle->motionState, colShape, localInertia);
	currentParticle->rigidBody = new btRigidBody(rbInfo);
	currentParticle->rigidBody->setUserPointer(&currentParticle->col);
	AddBodyToWorld(currentParticle->rigidBody, ColliderType::KINEMATIC, currentParticle->emitter->layer);
}

void ModulePhysics::RemoveParticleRigidbody(ComponentParticleSystem::Particle* particle) {
	if (particle->rigidBody) {
		rigidBodiesToRemove.push_back(particle->rigidBody);
		particle->rigidBody = nullptr;
		RELEASE(particle->motionState);
	}
}

void ModulePhysics::UpdateParticleRigidbody(ComponentParticleSystem::Particle* particle) {
	RemoveParticleRigidbody(particle);
	CreateParticleRigidbody(particle);
}

void ModulePhysics::SetGravity(float newGravity) {
	world->setGravity(btVector3(0.f, newGravity, 0.f));
}

/*BULLET DEBUG: Uncomment to activate it. #include "debugdraw.h" in this file if using it.
// =================== BULLET DEBUG CALLBACKS ==========================
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
}*/