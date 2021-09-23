#include "Barrel.h"
#include "GameplaySystems.h"
#include "Components/Physics/ComponentSphereCollider.h"
#include "CameraController.h"
#include "Components/ComponentParticleSystem.h"

EXPOSE_MEMBERS(Barrel) {
	// Add members here to expose them to the engine. Example:
	MEMBER(MemberType::GAME_OBJECT_UID, barrelUID),
		MEMBER(MemberType::GAME_OBJECT_UID, sphereColliderUID),
		MEMBER(MemberType::GAME_OBJECT_UID, cameraUID),
		MEMBER(MemberType::GAME_OBJECT_UID, particlesUID),
		MEMBER(MemberType::GAME_OBJECT_UID, particlesForTimerUID),
		MEMBER(MemberType::FLOAT, timeToDestroy),
		MEMBER(MemberType::FLOAT, timerToDestroy)

};

GENERATE_BODY_IMPL(Barrel);

void Barrel::Start() {
	barrel = GameplaySystems::GetGameObject(barrelUID);

	GameObject* barrelColliderAux = GameplaySystems::GetGameObject(sphereColliderUID);
	if (barrelColliderAux) {
		barrelCollider = barrelColliderAux;
		barrelCollider->Disable();
	}

	GameObject* cameraAux = GameplaySystems::GetGameObject(cameraUID);
	if (cameraAux) {
		cameraController = GET_SCRIPT(cameraAux, CameraController);
	}
	GameObject* particleAux = GameplaySystems::GetGameObject(particlesUID);
	if (particleAux) {
		particles = particleAux->GetComponent<ComponentParticleSystem>();
		audio = particleAux->GetComponent<ComponentAudioSource>();
	}

	GameObject* particleForTimerAux = GameplaySystems::GetGameObject(particlesForTimerUID);
	if (particleForTimerAux) {
		particlesForTimer = particleForTimerAux->GetComponent<ComponentParticleSystem>();
		audioForTimer = particleForTimerAux->GetComponent<ComponentAudioSource>();
	}
}

void Barrel::Update() {

	if (startTimerToDestroy && timerDestroyActivated) {
		if (particlesForTimer) particlesForTimer->PlayChildParticles();
		if (audioForTimer) audioForTimer->Play();

		currentTimerToDestroy += Time::GetDeltaTime();
		if (currentTimerToDestroy >= timerToDestroy) {
			if (audioForTimer) audioForTimer->Stop();
			isHit = true;
			startTimerToDestroy = false;
		}

	}

	if (isHit) {
		if(barrelCollider) barrelCollider->Enable();
		isHit = false;
		if(particles) particles->PlayChildParticles();
		if(audio) audio->Play();
		if(barrel) barrel->Disable();
		destroy = true;
	}

	if (destroy) {

		if (timeToDestroy > 0) {
			timeToDestroy -= Time::GetDeltaTime();
		}
		else {
			destroy = false;
			if(barrel) GameplaySystems::DestroyGameObject(barrel->GetParent());
		}
	}

}

void Barrel::OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle)
{
	if (particle) {
		ComponentParticleSystem::Particle* p = (ComponentParticleSystem::Particle*)particle;
		ComponentParticleSystem* pSystem = collidedWith.GetComponent<ComponentParticleSystem>();
		if (pSystem) pSystem->KillParticle(p);

		if ( collidedWith.name == "FangRightBullet" || collidedWith.name == "FangLeftBullet" || collidedWith.name == "OnimaruBullet"  ) {

			if (!timerDestroyActivated) {
				startTimerToDestroy = true;
				timerDestroyActivated = true;
			}

			ComponentSphereCollider* thisCollider = GetOwner().GetComponent<ComponentSphereCollider>();
			thisCollider->Disable();
		}

	}

}