#include "Shield.h"

#include "GameObject.h"
#include "GameplaySystems.h"
#include "PlayerController.h"
#include "RangerProjectileScript.h"
#include "Components/Physics/ComponentSphereCollider.h"
#include "Components/ComponentAudioSource.h"
#include "Components/ComponentAgent.h"
#include "Math/float3.h"

EXPOSE_MEMBERS(Shield) {
	MEMBER(MemberType::INT, maxCharges),
	MEMBER(MemberType::FLOAT, chargeCooldown),
	MEMBER(MemberType::GAME_OBJECT_UID, playerUID),
	MEMBER_SEPARATOR("Debug only"),
	MEMBER(MemberType::INT, currentAvailableCharges)
};

GENERATE_BODY_IMPL(Shield);

void Shield::Start() {
	currentAvailableCharges = maxCharges;
	GameObject* playerGO = GameplaySystems::GetGameObject(playerUID);
	if (playerGO) playerController = GET_SCRIPT(playerGO, PlayerController);
	audio = GetOwner().GetComponent<ComponentAudioSource>();
}

void Shield::Update() {}

void Shield::InitShield() {
	isActive = true;
}

void Shield::FadeShield() {
	isActive = false;
}

void Shield::OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle) {
	if ((collidedWith.name == "WeaponParticles" || collidedWith.name == "RightBlade" || collidedWith.name == "LeftBlade") && isActive && playerController) {

		if (!particle) {
			collidedWith.Disable();
		} else {
			ComponentParticleSystem::Particle* p = (ComponentParticleSystem::Particle*)particle;
			ComponentParticleSystem* pSystem = collidedWith.GetComponent<ComponentParticleSystem>();
			if (pSystem) pSystem->KillParticle(p);

			if (playerController->playerOnimaru.level1Upgrade && collidedWith.name == "WeaponParticles") {		// Reflect projectile
				/*ComponentSphereCollider* sCollider = collidedWith.GetComponent<ComponentSphereCollider>();
				if (!sCollider) return;
				RangerProjectileScript* rps = GET_SCRIPT(&collidedWith, RangerProjectileScript);
				if (rps) {
					// Separate Bullet from shield
					float3 actualPenDistance = penetrationDistance.ProjectTo(collisionNormal);
					collidedWith.GetComponent<ComponentTransform>()->SetGlobalPosition(collidedWith.GetComponent<ComponentTransform>()->GetGlobalPosition() + actualPenDistance);
					// Reflect
					float3 front = rps->GetRangerDirection() * float3(0, 0, 1);
					float3 normal = -float3(collisionNormal.x, 0, collisionNormal.z);
					float3 newFront = front - 2 * front.ProjectTo(normal);
					rps->SetRangerDirection(Quat::LookAt(float3(0, 0, 1), newFront, float3(0, 1, 0), float3(0, 1, 0)));
					// Convert to player Bullet
					sCollider->layer = WorldLayers::BULLET;
					sCollider->layerIndex = 5;
					Physics::UpdateRigidbody(sCollider);
				}*/
			}
		}

		currentAvailableCharges--;

		if (audio) {		// Play hit effect
			audio->Play();
		}
	} else if (collidedWith.name == "MeleeGrunt" || collidedWith.name == "RangedGrunt") {
		ComponentAgent* agent = collidedWith.GetComponent<ComponentAgent>();
		if (agent) {
			agent->RemoveAgentFromCrowd();
			float3 actualPenDistance = -penetrationDistance.ProjectTo(collisionNormal);
			collidedWith.GetComponent<ComponentTransform>()->SetGlobalPosition(collidedWith.GetComponent<ComponentTransform>()->GetGlobalPosition() + actualPenDistance);
			agent->AddAgentToCrowd();
		}
	}
}