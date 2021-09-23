#include "Fang.h"
#include "GameplaySystems.h"
#include "GameController.h"
#include "HUDController.h"
#include "HUDManager.h"
#include "CameraController.h"
#include "UltimateFang.h"

void Fang::Init(UID fangUID, UID dashParticleUID, UID leftGunUID, UID rightGunUID, UID rightBulletUID, UID leftBulletUID, UID laserUID, UID cameraUID, UID HUDManagerObjectUID, UID dashUID, UID EMPUID, UID EMPEffectsUID, UID fangUltimateUID, UID ultimateVFXUID, UID rightFootVFX, UID leftFootVFX) {
	SetTotalLifePoints(lifePoints);
	characterGameObject = GameplaySystems::GetGameObject(fangUID);
	if (characterGameObject && characterGameObject->GetParent()) {
		playerMainTransform = characterGameObject->GetParent()->GetComponent<ComponentTransform>();
		agent = characterGameObject->GetParent()->GetComponent<ComponentAgent>();
		compAnimation = characterGameObject->GetComponent<ComponentAnimation>();
		GameObject* cameraAux = GameplaySystems::GetGameObject(cameraUID);
		if (cameraAux) {
			lookAtMouseCameraComp = cameraAux->GetComponent<ComponentCamera>();
			cameraController = GET_SCRIPT(cameraAux, CameraController);
		}
		//right gun
		GameObject* gunAux = GameplaySystems::GetGameObject(rightGunUID);
		if (gunAux) rightGunTransform = gunAux->GetComponent<ComponentTransform>();
		//left gun
		GameObject* gunAux1 = GameplaySystems::GetGameObject(leftGunUID);
		if (gunAux1) leftGunTransform = gunAux1->GetComponent<ComponentTransform>();

		GameObject* lookAtPoint = GameplaySystems::GetGameObject(lookAtPointUID);
		if (lookAtPoint) {
			lookAtMousePlanePosition = lookAtPoint->GetComponent<ComponentTransform>()->GetGlobalPosition();
		}

		//laser
		fangLaser = GameplaySystems::GetGameObject(laserUID);

		GameObject* dashAux = GameplaySystems::GetGameObject(dashParticleUID);
		if (dashAux) {
			dashParticle = dashAux->GetComponent<ComponentParticleSystem>();
			if (dashParticle) dashParticle->Stop();
		}

		rightBulletAux = GameplaySystems::GetGameObject(rightBulletUID);
		if (rightBulletAux) {
			rightBullet = rightBulletAux->GetComponent<ComponentParticleSystem>();
		}
		leftBulletAux = GameplaySystems::GetGameObject(leftBulletUID);
		if (leftBulletAux) {
			leftBullet = leftBulletAux->GetComponent<ComponentParticleSystem>();
		}
		if (compAnimation) {
			currentState = compAnimation->GetCurrentState();
		}
		movementSpeed = normalMovementSpeed;
		if (agent) {
			agent->SetMaxSpeed(movementSpeed);
			agent->SetMaxAcceleration(MAX_ACCELERATION);
		}
		GameObject* HUDManagerGO = GameplaySystems::GetGameObject(HUDManagerObjectUID);
		if (HUDManagerGO) {
			hudManagerScript = GET_SCRIPT(HUDManagerGO, HUDManager);
		}

		if (characterGameObject) {
			characterGameObject->GetComponent<ComponentCapsuleCollider>()->Enable();

			//Get audio sources
			int i = 0;

			for (ComponentAudioSource& src : characterGameObject->GetComponents<ComponentAudioSource>()) {
				if (i < static_cast<int>(FANG_AUDIOS::TOTAL)) fangAudios[i] = &src;
				i++;
			}
		}
		dash = GameplaySystems::GetGameObject(dashUID);

		EMP = GameplaySystems::GetGameObject(EMPUID);
		if (EMP) {
			ComponentSphereCollider* sCollider = EMP->GetComponent<ComponentSphereCollider>();
			if (sCollider) sCollider->radius = EMPRadius;
		}

		GameObject* fangUltimateGameObject = GameplaySystems::GetGameObject(fangUltimateUID);
		if (fangUltimateGameObject) {
			ultimateScript = GET_SCRIPT(fangUltimateGameObject, UltimateFang);
			ultimateCooldownRemaining = ultimateCooldown;
		}
	}

	if (characterGameObject) {
		characterGameObject->GetComponent<ComponentCapsuleCollider>()->Enable();

		//Get audio sources
		int i = 0;

		for (ComponentAudioSource& src : characterGameObject->GetComponents<ComponentAudioSource>()) {
			if (i < static_cast<int>(FANG_AUDIOS::TOTAL)) fangAudios[i] = &src;
			i++;
		}
	}
	dash = GameplaySystems::GetGameObject(dashUID);

	EMP = GameplaySystems::GetGameObject(EMPUID);
	if (EMP) {
		ComponentSphereCollider* sCollider = EMP->GetComponent<ComponentSphereCollider>();
		if (sCollider) {
			sCollider->radius = EMPRadius;
			sCollider->Disable();
			sCollider->Enable();
		}
		EMP->Enable();
		EMP->Disable();
	}
	GameObject* EMPEffectsGO = GameplaySystems::GetGameObject(EMPEffectsUID);
	if (EMPEffectsGO) EMPEffects = EMPEffectsGO->GetComponent<ComponentParticleSystem>();

	GameObject* fangUltimateGameObject = GameplaySystems::GetGameObject(fangUltimateUID);
	if (fangUltimateGameObject) {
		ultimateScript = GET_SCRIPT(fangUltimateGameObject, UltimateFang);
		ultimateCooldownRemaining = 0;
	}

	GameObject* ultimateVFXGO = GameplaySystems::GetGameObject(ultimateVFXUID);
	if (ultimateVFXGO) ultimateVFX = ultimateVFXGO->GetComponent<ComponentParticleSystem>();

	GameObject* leftFootVFXGO = GameplaySystems::GetGameObject(leftFootVFX);
	if (leftFootVFXGO) leftFootstepsVFX = leftFootVFXGO->GetComponent<ComponentParticleSystem>();

	GameObject* rightFootVFXGO = GameplaySystems::GetGameObject(rightFootVFX);
	if (rightFootVFXGO) rightFootstepsVFX = rightFootVFXGO->GetComponent<ComponentParticleSystem>();
}

bool Fang::IsVulnerable() const {
	return !ultimateOn;
}

bool Fang::CanSwitch() const {
	if (!EMP) return false;
	return isAlive && !EMP->IsActive() && !ultimateOn && GameController::IsSwitchTutorialReached() && (!GameController::IsGameplayBlocked() || GameController::IsSwitchTutorialActive());
}

void Fang::IncreaseUltimateCounter() {
	if (!ultimateOn) ultimateCooldownRemaining++;
}

bool Fang::IsInstantOrientation(bool useGamepad) const {
	return !useGamepad || !Input::IsGamepadConnected(0);
}

void Fang::GetHit(float damage_) {
	if (!dashing && isAlive) {
		if (cameraController) {
			cameraController->StartShake();
		}

		lifePoints -= damage_;
		if (fangAudios[static_cast<int>(FANG_AUDIOS::HIT)]) fangAudios[static_cast<int>(FANG_AUDIOS::HIT)]->Play();
		isAlive = lifePoints > 0.0f;

		if (!isAlive && fangAudios[static_cast<int>(FANG_AUDIOS::DEATH)]) {
			fangAudios[static_cast<int>(FANG_AUDIOS::DEATH)]->Play();
		}
		//OnDeath();
	}
}

void Fang::InitDash() {
	if (CanDash()) {
		hasDashed = true;
		if (dash && level1Upgrade) dash->Enable();
		if (movementInputDirection != MovementDirection::NONE) {
			dashDirection = GetDirection();
			dashMovementDirection = movementInputDirection;
		} else {
			dashDirection = facePointDir;
		}

		if (dashParticle) {
			Quat rotation = Quat::RotateFromTo(float3(0.0f, 1.0f, 0.0f), dashDirection);
			dashParticle->GetOwner().GetComponent<ComponentTransform>()->SetGlobalRotation(rotation);
			dashParticle->PlayChildParticles();
		}

		dashCooldownRemaining = dashCooldown;
		dashRemaining = dashDuration;
		dashInCooldown = true;
		dashing = true;
		if (agent) {
			agent->SetMaxSpeed(dashSpeed);
		}

		if (fangAudios[static_cast<int>(FANG_AUDIOS::DASH)]) {
			fangAudios[static_cast<int>(FANG_AUDIOS::DASH)]->Play();
		}

		if (hudManagerScript) {
			hudManagerScript->SetCooldownRetreival(HUDManager::Cooldowns::FANG_SKILL_1);
		}
	}
}

void Fang::Dash() {
	if (dashing) {
		float3 newPosition = playerMainTransform->GetGlobalPosition();
		newPosition += dashSpeed * dashDirection;
		agent->SetMoveTarget(newPosition, false);
	}
}

bool Fang::CanDash() {
	return isAlive && !dashing && !dashInCooldown && !EMP->IsActive() && !ultimateOn && !GameController::IsGameplayBlocked();
}

void Fang::ActivateEMP() {
	if (EMP && CanEMP()) {
		if (EMPEffects) EMPEffects->PlayChildParticles();
		EMP->Enable();
		EMPCooldownRemaining = EMPCooldown;
		EMPInCooldown = true;

		if (fangAudios[static_cast<int>(FANG_AUDIOS::EMP)]) {
			fangAudios[static_cast<int>(FANG_AUDIOS::EMP)]->Play();
		}

		if (hudManagerScript) {
			hudManagerScript->SetCooldownRetreival(HUDManager::Cooldowns::FANG_SKILL_2);
		}
	}
}

bool Fang::CanEMP() {
	return !EMP->IsActive() && !EMPInCooldown && !dashing && !ultimateOn && !GameController::IsGameplayBlocked();
}

void Fang::CheckCoolDowns(bool noCooldownMode) {
	//Combat
	if (aiming) {
		if (!isUltimate && !EMP->IsActive()) {
			if (fangLaser && !fangLaser->IsActive()) {
				fangLaser->Enable();
			}
		} else {
			fangLaser->Disable();
		}
		timeWithoutCombat += Time::GetDeltaTime();
		if (timeWithoutCombat >= aimTime || GameController::IsGameplayBlocked()) {
			aiming = false;
			transitioning = 0;
			timeWithoutCombat = aimTime;
		}
	} else {
		if (fangLaser && fangLaser->IsActive()) {
			fangLaser->Disable();
		}
	}

	//Dash Cooldown
	if (dashInCooldown) {
		if (noCooldownMode || dashCooldownRemaining <= 0.f) {
			dashCooldownRemaining = 0.f;
			dashInCooldown = false;
			dashMovementDirection = MovementDirection::NONE;
		} else {
			dashCooldownRemaining -= Time::GetDeltaTime();
		}
	}

	//Dash duration
	if (dashing) {
		if (dashRemaining <= 0.f) {
			dashRemaining = 0.f;
			dashing = false;
			if (dash) {
				if (dash->IsActive()) dash->Disable();
			}
			agent->SetMaxSpeed(movementSpeed);
		} else {
			dashRemaining -= Time::GetDeltaTime();
		}
	}

	//EMP Cooldown
	if (EMPInCooldown) {
		if (noCooldownMode || EMPCooldownRemaining <= 0.f) {
			EMPCooldownRemaining = 0.f;
			EMPInCooldown = false;
		} else {
			EMPCooldownRemaining -= Time::GetDeltaTime();
		}
	}

	//Ultimate Cooldown
	if (ultimateInCooldown) {
		if (noCooldownMode || ultimateCooldownRemaining >= ultimateCooldown) {
			ultimateCooldownRemaining = ultimateCooldown;
			ultimateInCooldown = false;
		}
	}
}

void Fang::OnAnimationFinished() {
	int idle = aiming ? static_cast<int>(FANG_STATES::IDLE_AIM) : static_cast<int>(FANG_STATES::IDLE);
	if (compAnimation) {
		if (compAnimation->GetCurrentState()) {
			if (compAnimation->GetCurrentState()->name == states[static_cast<int>(FANG_STATES::EMP)]) {
				compAnimation->SendTrigger(states[static_cast<int>(FANG_STATES::EMP)] + states[idle]);
				EMP->Disable();
			} else if (compAnimation->GetCurrentState()->name == states[static_cast<int>(FANG_STATES::ULTIMATE)]) {
				compAnimation->SendTrigger(states[static_cast<int>(FANG_STATES::ULTIMATE)] + states[idle]);
				ultimateOn = false;
				isUltimate = false;
				movementSpeed = normalMovementSpeed;
				ultimateScript->EndUltimate();

				if (hudManagerScript) {
					hudManagerScript->StopUsingSkill(HUDManager::Cooldowns::FANG_SKILL_3);
				}
			} else if (compAnimation->GetCurrentState()->name == states[static_cast<int>(FANG_STATES::DRIFT)]) {
				compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[static_cast<int>(FANG_STATES::IDLE)]);
			}
		}
	}
}

void Fang::OnAnimationEvent(StateMachineEnum stateMachineEnum, const char* eventName) {
	switch (stateMachineEnum) {
	case StateMachineEnum::PRINCIPAL:
		if (std::strcmp(eventName, "FootstepRight") == 0) {
			if (fangAudios[static_cast<int>(FANG_AUDIOS::FOOTSTEP_RIGHT)]) {
				fangAudios[static_cast<int>(FANG_AUDIOS::FOOTSTEP_RIGHT)]->Play();
			}
			if (rightFootstepsVFX) rightFootstepsVFX->PlayChildParticles();
		} else if (std::strcmp(eventName, "FootstepLeft") == 0) {
			if (fangAudios[static_cast<int>(FANG_AUDIOS::FOOTSTEP_LEFT)]) {
				fangAudios[static_cast<int>(FANG_AUDIOS::FOOTSTEP_LEFT)]->Play();
			}
			if (leftFootstepsVFX) leftFootstepsVFX->PlayChildParticles();
		}
		break;
	case StateMachineEnum::SECONDARY:
		if (std::strcmp(eventName, "LeftShot") == 0) {
			bullet = leftBullet;
		} else if (std::strcmp(eventName, "RightShot") == 0) {
			bullet = rightBullet;
		}
		break;
	}
	if (bullet) {
		transitioning++;
		if (transitioning > 1) {
			if (fangAudios[static_cast<int>(FANG_AUDIOS::SHOOT)]) {
				fangAudios[static_cast<int>(FANG_AUDIOS::SHOOT)]->Play();
			}
			bullet->PlayChildParticles();
		}
		bullet = nullptr;
	}
}

float Fang::GetRealDashCooldown() {
	return 1.0f - (dashCooldownRemaining / dashCooldown);
}

float Fang::GetRealEMPCooldown() {
	return 1.0f - (EMPCooldownRemaining / EMPCooldown);
}

float Fang::GetRealUltimateCooldown() {
	return (ultimateCooldownRemaining / (float)ultimateCooldown);
}

bool Fang::CanShoot() {
	return !shooting && !ultimateOn && !compAnimation->GetCurrentStateSecondary() && !GameController::IsGameplayBlocked();
}

bool Fang::IsAiming() {
	return aiming;
}

void Fang::Shoot() {
	if (CanShoot()) {
		//shootingOnCooldown = true;
		shooting = true;
		attackCooldownRemaining = 1.f / attackSpeed;
		//setear la velocidad de animacion
		if (compAnimation->GetCurrentState()) compAnimation->SendTriggerSecondary(compAnimation->GetCurrentState()->name + states[static_cast<int>(FANG_STATES::SHOOTING)]);
		if (compAnimation->GetCurrentStateSecondary()) {
			ResourceClip* clip = GameplaySystems::GetResource<ResourceClip>(compAnimation->GetCurrentStateSecondary()->clipUid);
			clip->speed = attackSpeed;
		}
	}
}

void Fang::PlayAnimation() {
	if (!compAnimation) return;
	if (!EMP) return;

	int dashAnimation = 0;
	if (dashing) {
		dashAnimation = 4;
		movementInputDirection = dashMovementDirection;
	}
	if (EMP->IsActive()) movementInputDirection = MovementDirection::NONE;
	int idle = aiming ? static_cast<int>(FANG_STATES::IDLE_AIM) : static_cast<int>(FANG_STATES::IDLE);
	if (compAnimation->GetCurrentState()) {
		if (ultimateOn || compAnimation->GetCurrentState()->name == states[static_cast<int>(FANG_STATES::ULTIMATE)]) {
			if (compAnimation->GetCurrentState()->name != states[static_cast<int>(FANG_STATES::ULTIMATE)]) {
				compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[static_cast<int>(FANG_STATES::ULTIMATE)]);
			}
		} else if (movementInputDirection == MovementDirection::NONE) {
			if (!isAlive) {
				if (compAnimation->GetCurrentState()->name != states[static_cast<int>(FANG_STATES::DEATH)]) {
					compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[static_cast<int>(FANG_STATES::DEATH)]);
					if (compAnimation->GetCurrentStateSecondary()) {
						compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + states[static_cast<int>(FANG_STATES::DEATH)]);
					}
				}
			} else {
				if (compAnimation->GetCurrentState()->name == states[static_cast<int>(FANG_STATES::SPRINT)]) {
					compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[static_cast<int>(FANG_STATES::DRIFT)]);
					decelerating = true;
				} else if (!decelerating) {
					if (compAnimation->GetCurrentState()->name != states[idle] && compAnimation->GetCurrentState()->name != states[static_cast<int>(FANG_STATES::EMP)]) {
						compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[idle]);
					}
				}
				if (compAnimation->GetCurrentState()->name != states[static_cast<int>(FANG_STATES::EMP)] && EMP->IsActive()) {
					compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[static_cast<int>(FANG_STATES::EMP)]);
				}
			}
		} else {
			if (compAnimation->GetCurrentState()->name != states[aiming ? (GetMouseDirectionState() + dashAnimation) : static_cast<int>(FANG_STATES::SPRINT)]) {
				compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[aiming ? (GetMouseDirectionState() + dashAnimation) : static_cast<int>(FANG_STATES::SPRINT)]);
				ResourceClip* clip = GameplaySystems::GetResource<ResourceClip>(compAnimation->GetCurrentState()->clipUid);
				SetClipSpeed(clip, agent->GetMaxSpeed());
			}
		}
	}
}

void Fang::ActiveUltimate() {
	if (CanUltimate()) {
		ultimateTimeRemaining = ultimateTotalTime;
		ultimateCooldownRemaining = 0;
		ultimateOn = true;
		ultimateInCooldown = true;
		if (ultimateVFX) ultimateVFX->PlayChildParticles();
		ultimateScript->StartUltimate();
		isUltimate = true;
		if (hudManagerScript) {
			hudManagerScript->StartUsingSkill(HUDManager::Cooldowns::FANG_SKILL_3);
		}

		movementSpeed = ultimateMovementSpeed;

		if (fangAudios[static_cast<int>(FANG_AUDIOS::ULTIMATE)]) {
			fangAudios[static_cast<int>(FANG_AUDIOS::ULTIMATE)]->Play();
		}

		if (hudManagerScript) {
			hudManagerScript->SetCooldownRetreival(HUDManager::Cooldowns::FANG_SKILL_3);
		}
	}
}

bool Fang::CanUltimate() {
	return !dashing && !EMP->IsActive() && ultimateCooldownRemaining >= ultimateCooldown && !ultimateOn && !GameController::IsGameplayBlocked();
}

void Fang::Update(bool useGamepad, bool lockMovement, bool lockRotation) {
	if (isAlive) {
		if (ultimateOn) {
			ultimateTimeRemaining -= Time::GetDeltaTime();
			if (ultimateTimeRemaining <= 0) {
				ultimateTimeRemaining = 0;
			}
		}

		if (EMP) {
			faceToFront = !aiming;
			Player::Update(useGamepad, dashing || EMP->IsActive(), dashing || EMP->IsActive() || ultimateOn);
			if (GetInputBool(InputActions::ABILITY_1, useGamepad) && !EMP->IsActive() && !ultimateOn) {
				InitDash();
			}

			if (!dashing && !EMP->IsActive()) {
				if (GetInputBool(InputActions::SHOOT, useGamepad) && !GameController::IsGameplayBlocked()) {
					ResetIsInCombatValues();
					Shoot();
				}
				
				if (ultimateOn) {
					ResetIsInCombatValues();
				}
			}

			Dash();
			if (!GetInputBool(InputActions::SHOOT, useGamepad) || dashing || EMP->IsActive() || ultimateOn || GameController::IsGameplayBlocked()) {
				if (shooting) {
					compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + compAnimation->GetCurrentState()->name);
					shooting = false;
				}
			}
			if (GetInputBool(InputActions::ABILITY_2, useGamepad) && !ultimateOn) {
				ActivateEMP();
			}

			if (GetInputBool(InputActions::ABILITY_3, useGamepad)) {
				ActiveUltimate();
			}
		}
	} else {
		if (agent) agent->RemoveAgentFromCrowd();
		movementInputDirection = MovementDirection::NONE;
	}
	PlayAnimation();
}

void Fang::ResetIsInCombatValues() {
	timeWithoutCombat = 0.f;
	aiming = true;
	decelerating = 0;
}