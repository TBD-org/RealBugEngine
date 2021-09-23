#include "PlayerDeath.h"

#include "GameObject.h"
#include "GameplaySystems.h"

#include "PlayerController.h"
#include "SceneTransition.h"
#include "GameOverUIController.h"

#define LEFT_SHOT "LeftShot"
#define RIGHT_SHOT "RightShot"

EXPOSE_MEMBERS(PlayerDeath) {
	MEMBER(MemberType::SCENE_RESOURCE_UID, sceneUID),
	MEMBER(MemberType::GAME_OBJECT_UID, playerUID),
	MEMBER(MemberType::FLOAT, rangedDamageTaken),
	MEMBER(MemberType::FLOAT, meleeDamageTaken),
	MEMBER(MemberType::FLOAT, barrelDamageTaken),
	MEMBER(MemberType::FLOAT, laserBeamTaken),
	MEMBER(MemberType::FLOAT, laserHitCooldown),
	MEMBER(MemberType::FLOAT, laserHitCooldownTimer),
	MEMBER(MemberType::FLOAT, fireDamageTaken),
	MEMBER(MemberType::FLOAT, cooldownFireDamage),
	MEMBER(MemberType::GAME_OBJECT_UID, transitionUID),
	MEMBER(MemberType::GAME_OBJECT_UID, gameOverUID)
};

GENERATE_BODY_IMPL(PlayerDeath);

void PlayerDeath::Start() {
	player = GameplaySystems::GetGameObject(playerUID);
	if (player) playerController = GET_SCRIPT(player, PlayerController);
	if (transitionUID != 0) {
		transitionGO = GameplaySystems::GetGameObject(transitionUID);
		if (transitionGO) sceneTransition = GET_SCRIPT(transitionGO, SceneTransition);
	}
	GameObject* gameOverGO = GameplaySystems::GetGameObject(gameOverUID);

	if (gameOverGO)gameOverController = GET_SCRIPT(gameOverGO, GameOverUIController);

	laserHitCooldownTimer = laserHitCooldown;
}

void PlayerDeath::Update() {
	if (player) {
		if (playerController) {
			dead = playerController->IsActiveCharacterDead();
		}

		if (getLaserHit) {
			if (!lastFrameLaserHit && getLaserHit) laserHitCooldownTimer = laserHitCooldown;
			laserHitCooldownTimer += Time::GetDeltaTime();
			if (laserHitCooldownTimer > laserHitCooldown) {
				laserHitCooldownTimer = 0.0f;
				if (playerController) playerController->TakeDamage(laserBeamTaken);
				getLaserHit = false;
			}
			lastFrameLaserHit = true;
		} else {
			lastFrameLaserHit = false;
		}

		if (timerFireDamage <= cooldownFireDamage) {
			timerFireDamage += Time::GetDeltaTime();
			if (timerFireDamage > cooldownFireDamage) {
				fireDamageActive = true;
				timerFireDamage = 0.f;
			}
		}
	}
}

void PlayerDeath::OnAnimationFinished() {
	if (dead) {
		if (playerController) {
			if (playerController->IsPlayerDead()) {
				OnLoseConditionMet();
			} else {
				playerController->OnCharacterDeath();
			}
		}
	} else {
		if (!playerController)return;
		if (playerController->playerFang.characterGameObject->IsActive()) {
			playerController->playerFang.OnAnimationFinished();
		} else {
			playerController->playerOnimaru.OnAnimationFinished();
		}
	}
}

void PlayerDeath::OnAnimationSecondaryFinished() {
	if (playerController) {
		if (playerController->playerFang.IsActive()) {
			playerController->playerFang.OnAnimationSecondaryFinished();
		}
		else if(playerController->playerOnimaru.IsActive()) {
			playerController->playerOnimaru.OnAnimationSecondaryFinished();
		}
	}
}

void PlayerDeath::OnAnimationEvent(StateMachineEnum stateMachineEnum, const char* eventName) {
	if (playerController) {
		if (playerController->playerFang.IsActive()) {
			playerController->playerFang.OnAnimationEvent(stateMachineEnum, eventName);
		} else {
			playerController->playerOnimaru.OnAnimationEvent(stateMachineEnum, eventName);
		}
	}
}

void PlayerDeath::OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle) {
	if (collidedWith.name == "WeaponParticles") {
		if (!particle) return;
		ComponentParticleSystem::Particle* p = (ComponentParticleSystem::Particle*)particle;
		ComponentParticleSystem* pSystem = collidedWith.GetComponent<ComponentParticleSystem>();
		if (pSystem) pSystem->KillParticle(p);
		if (playerController) playerController->TakeDamage(rangedDamageTaken);
	} else if (collidedWith.name == "RightBlade" || collidedWith.name == "LeftBlade") { //meleegrunt
		if (playerController) {
			float3 onimaruFront = -playerController->playerOnimaru.playerMainTransform->GetFront();
			if (!(playerController->playerOnimaru.IsShielding() && collisionNormal.Dot(onimaruFront) > 0.f)) {
				playerController->TakeDamage(meleeDamageTaken);
			}
		}
		collidedWith.Disable();
	} else if (collidedWith.name == "Barrel") {
		if (playerController) playerController->TakeDamage(barrelDamageTaken);
		collidedWith.Disable();
	} else if (collidedWith.name == "LaserBeam") {
		getLaserHit = true;
	} else if (collidedWith.name == "FireTile") {
		if (fireDamageActive) {
			if (playerController) playerController->TakeDamage(fireDamageTaken);
			fireDamageActive = false;
		}
	}
}

void PlayerDeath::OnLoseConditionMet() {
	if (gameOverController) {
			gameOverController->GameOver();
	} else{
		if (sceneTransition) {
			sceneTransition->StartTransition();
		} else {
			if (sceneUID != 0) SceneManager::ChangeScene(sceneUID);
		}
	}
}
