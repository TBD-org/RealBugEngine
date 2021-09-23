#include "Onimaru.h"

#include "GameplaySystems.h"
#include "GameController.h"
#include "HUDController.h"
#include "HUDManager.h"
#include "CameraController.h"
#include "OnimaruBullet.h"
#include "AIMeleeGrunt.h"
#include "RangedAI.h"

#include "Shield.h"

bool Onimaru::CanShoot() {
	return !shootingOnCooldown && !GameController::IsGameplayBlocked();
}

bool Onimaru::CanBlast() const {
	return !blastInCooldown && !IsShielding() && !ultimateOn && !blastInUse && !GameController::IsGameplayBlocked();
}

void Onimaru::GetHit(float damage_) {
	//We assume that the player is always alive when this method gets called, so no need to check if character was alive before taking lives
	if (isAlive) {
		if (cameraController) {
			cameraController->StartShake();
		}

		lifePoints -= damage_;
		if (onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::HIT)]) onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::HIT)]->Play();
		isAlive = lifePoints > 0.0f;

		if (!isAlive) {
			if (onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::DEATH)]) onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::DEATH)]->Play();
			OnDeath();
		}
	}
}

void Onimaru::IncreaseUltimateCounter() {
	if (!ultimateOn) ++ultimateChargePoints;
}

void Onimaru::Shoot() {
	if (CanShoot()) {
		shootingOnCooldown = true;
		//	shooting = true;
		attackCooldownRemaining = 1.f / attackSpeed;

		//NullRef on ultimate values
		if (onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::SPECIAL_SHOOT)] == nullptr) {
			onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::SPECIAL_SHOOT)] = onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::SHOOT)];
		}
		if (ultimateBullet == nullptr) {
			ultimateBullet = bullet;
		}

		ComponentAudioSource* audioSourceToPlay = !ultimateOn ? onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::SHOOT)] : onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::SPECIAL_SHOOT)];

		if (audioSourceToPlay) {
			audioSourceToPlay->Play();
		}
	}
}

void Onimaru::Blast() {
	bool releaseBlast = currentBlastDuration <= blastDelay ? false : true;
	if (releaseBlast && calculateEnemiesInRange) {
		if (blastParticles) blastParticles->PlayChildParticles();
		calculateEnemiesInRange = false;
		for (GameObject* enemy : enemiesInMap) {
			AIMeleeGrunt* meleeScript = GET_SCRIPT(enemy, AIMeleeGrunt);
			RangedAI* rangedScript = GET_SCRIPT(enemy, RangedAI);
			if (rangedScript || meleeScript) {
				if (rightHand && playerMainTransform) {
					float3 onimaruRightArmPos = rightHand->GetGlobalPosition();
					float3 enemyPos = enemy->GetComponent<ComponentTransform>()->GetGlobalPosition();
					onimaruRightArmPos = float3(onimaruRightArmPos.x, 0.f, onimaruRightArmPos.z);
					enemyPos = float3(enemyPos.x, 0.f, enemyPos.z);
					float distance = enemyPos.Distance(onimaruRightArmPos);
					float3 direction = (enemyPos - onimaruRightArmPos).Normalized();
					if (distance <= blastDistance) {
						float angle = 0.f;
						angle = RadToDeg(playerMainTransform->GetFront().AngleBetweenNorm(direction));
						if (angle <= blastAngle / 2.0f) {
							if (meleeScript) meleeScript->EnableBlastPushBack();
							else if (rangedScript) rangedScript->EnableBlastPushBack();
						}
					}
					else {
						if (meleeScript) {
							if (!meleeScript->IsBeingPushed()) meleeScript->DisableBlastPushBack();
						}
						else if (rangedScript) {
							if (!rangedScript->IsBeingPushed()) rangedScript->DisableBlastPushBack();
						}
					}
				}
			}
		}
	}
	else {
		currentBlastDuration += Time::GetDeltaTime();
		if (currentBlastDuration >= 1.8f) OnAnimationSecondaryFinished(); // Temporary hack
	}
}

void Onimaru::PlayAnimation() {
	if (!compAnimation) return;
	if (!isAlive) return; //Ultimate will block out all movement and idle from happening

	if (!UltimateStarted()) {
		if (!ultimateOn) {
			if (compAnimation->GetCurrentState()) {
				if (movementInputDirection == MovementDirection::NONE) {
					//Primery state machine idle when alive, without input movement
					if (compAnimation->GetCurrentState()->name != states[static_cast<int>(IDLE)]) {
						compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[static_cast<int>(IDLE)]);
					}
				}
				else {
					//If Movement is found, Primary state machine will be in charge of getting movement animations
					if (compAnimation->GetCurrentState()->name != (states[GetMouseDirectionState()])) {
						compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[GetMouseDirectionState()]);
						ResourceClip* clip = GameplaySystems::GetResource<ResourceClip>(compAnimation->GetCurrentState()->clipUid);
						SetClipSpeed(clip, agent->GetMaxSpeed());
					}
				}
			}
		}
	}
	else {
		if (compAnimation->GetCurrentState()) {
			if (movementInputDirection == MovementDirection::NONE) {
				//Primery state machine ultimate loop when alive, without input movement
				if (compAnimation->GetCurrentState()->name != states[static_cast<int>(ULTI_LOOP)]) {
					compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[static_cast<int>(ULTI_LOOP)]);
				}
			}
			else {
				//If Movement is found, Primary state machine will be in charge of getting movement animations
				if (compAnimation->GetCurrentState()->name != (states[static_cast<int>(ULTI_LOOP_WALKING)])) {
					compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[static_cast<int>(ULTI_LOOP_WALKING)]);
				}
			}
		}
	}
}

void Onimaru::StartUltimate() {
	if (!compAnimation) return;
	if (compAnimation->GetCurrentState()) {
		if (compAnimation->GetCurrentStateSecondary()) {
			if (compAnimation->GetCurrentStateSecondary()->name != compAnimation->GetCurrentState()->name) {
				compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + compAnimation->GetCurrentState()->name);
			}
		}
		compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[static_cast<int>(ULTI_INTRO)]);
	}

	if (hudManagerScript) {
		hudManagerScript->SetCooldownRetreival(HUDManager::Cooldowns::ONIMARU_SKILL_3);
		hudManagerScript->StartUsingSkill(HUDManager::Cooldowns::ONIMARU_SKILL_3);
	}

	if (onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::ULTIMATE)] == nullptr) {
		if (!onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::ULTIMATE)]->IsPlaying()) {
			onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::ULTIMATE)]->Play();
		}
	}

	ultimateChargePoints = 0;
	orientationSpeed = ultimateOrientationSpeed;
	attackSpeed = ultimateAttackSpeed;
	movementSpeed = ultimateMovementSpeed;
	movementInputDirection = MovementDirection::NONE;
	Player::MoveTo();
	ultimateOn = true;
}

void Onimaru::FinishUltimate() {
	ultimateTimeRemaining = 0;
	orientationSpeed = normalOrientationSpeed;
	attackSpeed = originalAttackSpeed;
	ultimateOn = false;

	if (hudManagerScript) {
		hudManagerScript->StopUsingSkill(HUDManager::Cooldowns::ONIMARU_SKILL_3);
	}

	movementSpeed = normalMovementSpeed;
	compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[static_cast<int>(IDLE)]);
}

bool Onimaru::IsShielding() const {
	return shield->GetIsActive();
}

bool Onimaru::IsVulnerable() const {
	return !ultimateOn;
}

float Onimaru::GetNormalizedRemainingUltimateTime() const {
	if (ultimateOn) {
		if (ultimateTimeRemaining <= 0.0f) {
			return 1.0f;
		}
		else {
			return ultimateTimeRemaining / ultimateTotalTime;
		}
	}

	return 0.0f;
}

float Onimaru::GetRealShieldCooldown() {
	if (shield == nullptr || shieldGO == nullptr) return 0.0f;
	float realShieldCooldown = 1.0f;
	realShieldCooldown = (float)shield->currentAvailableCharges / shield->maxCharges;

	return realShieldCooldown;
}

void Onimaru::CheckCoolDowns(bool noCooldownMode) {
	//aimingLaser

	if (shooting) {
		if (onimaruLaser && !onimaruLaser->IsActive()) {
			onimaruLaser->Enable();
		}
	}
	else {
		if (onimaruLaser && onimaruLaser->IsActive()) {
			onimaruLaser->Disable();
		}
	}

	//AttackCooldown
	if (attackCooldownRemaining <= 0.f) {
		attackCooldownRemaining = 0.f;
		shootingOnCooldown = false;
	}
	else {
		attackCooldownRemaining -= Time::GetDeltaTime();
	}

	if (noCooldownMode) {
		ultimateChargePoints = ultimateChargePointsTotal;
	}
	//Blast Cooldown
	if (noCooldownMode || blastCooldownRemaining <= 0.f) {
		blastCooldownRemaining = 0.f;
		blastInCooldown = false;
	}
	else {
		if (!blastInUse) blastCooldownRemaining -= Time::GetDeltaTime();
	}
	//ShieldCooldown
	if (shield->NeedsRecharging()) {
		if (shieldCooldownRemainingCharge <= 0.f) {
			shield->IncreaseCharge();
			shieldCooldownRemainingCharge = shield->GetChargeCooldown();
			if (!shield->NeedsRecharging()) {
				if (hudManagerScript) {
					hudManagerScript->SetCooldownRetreival(HUDManager::Cooldowns::ONIMARU_SKILL_1);
				}
			}
		} else {
			shieldCooldownRemainingCharge -= Time::GetDeltaTime();
		}
	}
}

bool Onimaru::IsAiming() const
{
	return aiming;
}

void Onimaru::OnDeath() {
	if (compAnimation == nullptr) return;
	if (compAnimation->GetCurrentState()) {
		if (compAnimation->GetCurrentState()->name != states[static_cast<int>(DEATH)]) {
			if (compAnimation->GetCurrentStateSecondary()) {
				compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + compAnimation->GetCurrentState()->name);
			}
			compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[static_cast<int>(DEATH)]);
		}
	}
	ultimateOn = blastInUse = false;
	shield->SetIsActive(false);
}

bool Onimaru::CanSwitch() const {
	return isAlive && ultimateTimeRemaining <= 0 && !ultimateOn && !IsShielding() && !blastInUse && (!GameController::IsGameplayBlocked() || GameController::IsSwitchTutorialActive());
}

void Onimaru::OnAnimationSecondaryFinished() {
	if (blastInUse) {
		blastInUse = false;
		blastInCooldown = true;
		currentBlastDuration = 0.f;
		calculateEnemiesInRange = true;
		if (compAnimation) {
			if (compAnimation->GetCurrentStateSecondary() && compAnimation->GetCurrentState()) {
				if (shooting) {
					compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + states[static_cast<int>(SHOOTING)]);
				}
				else {
					compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + compAnimation->GetCurrentState()->name);
				}
			}
		}
	}
}

bool Onimaru::IsInstantOrientation(bool useGamepad) const {
	//This must return true only when ultimate not in use and Gamepad is either not used or not connected
	return !ultimateOn && (!useGamepad || !Input::IsGamepadConnected(0));
}

void Onimaru::OnAnimationEvent(StateMachineEnum stateMachineEnum, const char* eventName) {
	if (stateMachineEnum == StateMachineEnum::PRINCIPAL) {
		if (std::strcmp(eventName, "FootstepRight")) {
			if (onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::FOOTSTEP_RIGHT)]) {
				onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::FOOTSTEP_RIGHT)]->Play();
			}
			if (rightFootstepsVFX) rightFootstepsVFX->PlayChildParticles();
		}
		else if (std::strcmp(eventName, "FootstepLeft")) {
			if (onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::FOOTSTEP_LEFT)]) {
				onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::FOOTSTEP_LEFT)]->Play();
			}
			if (leftFootstepsVFX) leftFootstepsVFX->PlayChildParticles();
		}
	}
}

void Onimaru::Init(UID onimaruUID, UID onimaruLaserUID, UID onimaruBulletUID, UID onimaruGunUID, UID onimaruRightHandUID, UID shieldUID, UID onimaruUltimateBulletUID, UID onimaruBlastEffectsUID, UID cameraUID, UID HUDManagerObjectUID, UID rightFootVFX, UID leftFootVFX) {
	SetTotalLifePoints(lifePoints);
	characterGameObject = GameplaySystems::GetGameObject(onimaruUID);
	if (characterGameObject && characterGameObject->GetParent()) {
		playerMainTransform = characterGameObject->GetParent()->GetComponent<ComponentTransform>();
		agent = characterGameObject->GetParent()->GetComponent<ComponentAgent>();
		compAnimation = characterGameObject->GetComponent<ComponentAnimation>();

		GameObject* cameraAux = GameplaySystems::GetGameObject(cameraUID);
		if (cameraAux) {
			lookAtMouseCameraComp = cameraAux->GetComponent<ComponentCamera>();
			cameraController = GET_SCRIPT(cameraAux, CameraController);
		}
		shieldingMaxSpeed = normalMovementSpeed / 2;
		movementSpeed = normalMovementSpeed;
		if (agent) {
			agent->SetMaxSpeed(movementSpeed);
			agent->SetMaxAcceleration(MAX_ACCELERATION);
		}

		GameObject* bulletAux = GameplaySystems::GetGameObject(onimaruBulletUID);
		if (bulletAux) {
			bullet = bulletAux->GetComponent<ComponentParticleSystem>();
			if (bullet) {
				bullet->SetParticlesPerSecond(float2(0.0f, 0.0f));
				bullet->Play();
			}
		}

		onimaruLaser = GameplaySystems::GetGameObject(onimaruLaserUID);

		GameObject* ultimateBulletAux = GameplaySystems::GetGameObject(onimaruUltimateBulletUID);
		if (bulletAux) {
			ultimateBullet = ultimateBulletAux->GetComponent<ComponentParticleSystem>();
		}
	}

	originalAttackSpeed = attackSpeed;

	GameObject* onimaruGun = GameplaySystems::GetGameObject(onimaruGunUID);
	if (onimaruGun) {
		gunTransform = onimaruGun->GetComponent<ComponentTransform>();
	}
	GameObject* lookAtPoint = GameplaySystems::GetGameObject(lookAtPointUID);
	if (lookAtPoint) {
		lookAtMousePlanePosition = lookAtPoint->GetComponent<ComponentTransform>()->GetGlobalPosition();
	}
	GameObject* HUDManagerGO = GameplaySystems::GetGameObject(HUDManagerObjectUID);
	if (HUDManagerGO) {
		hudManagerScript = GET_SCRIPT(HUDManagerGO, HUDManager);
	}

	shieldGO = GameplaySystems::GetGameObject(shieldUID);
	if (shieldGO) {
		shield = GET_SCRIPT(shieldGO, Shield);
		shieldGO->Disable();
		shieldCooldownRemainingCharge = shield->GetChargeCooldown();
	}

	if (characterGameObject) {
		//Get audio sources
		int i = 0;

		for (ComponentAudioSource& src : characterGameObject->GetComponents<ComponentAudioSource>()) {
			if (i < static_cast<int>(ONIMARU_AUDIOS::TOTAL)) onimaruAudios[i] = &src;
			i++;
		}
	}

	GameObject* rightHandGO = GameplaySystems::GetGameObject(onimaruRightHandUID);
	if (rightHandGO) rightHand = rightHandGO->GetComponent<ComponentTransform>();

	GameObject* blastParticlesGO = GameplaySystems::GetGameObject(onimaruBlastEffectsUID);
	if (blastParticlesGO) blastParticles = blastParticlesGO->GetComponent<ComponentParticleSystem>();

	GameObject* leftFootVFXGO = GameplaySystems::GetGameObject(leftFootVFX);
	if (leftFootVFXGO) leftFootstepsVFX = leftFootVFXGO->GetComponent<ComponentParticleSystem>();

	GameObject* rightFootVFXGO = GameplaySystems::GetGameObject(rightFootVFX);
	if (rightFootVFXGO) rightFootstepsVFX = rightFootVFXGO->GetComponent<ComponentParticleSystem>();

	if (characterGameObject) characterGameObject->Disable();
}

void Onimaru::OnAnimationFinished() {
	if (!compAnimation)return;
	if (ultimateOn) {
		ultimateTimeRemaining = ultimateTotalTime;
		if (compAnimation->GetCurrentState()) {
			compAnimation->SendTrigger(compAnimation->GetCurrentState()->name + states[static_cast<int>(ULTI_LOOP)]);
		}
	}
}

bool Onimaru::CanShield() const {
	if (shield == nullptr || shieldGO == nullptr) return false;
	return !ultimateOn && !shield->GetIsActive() && shield->CanUse() && !GameController::IsGameplayBlocked();
}

bool Onimaru::CanUltimate() const {
	return !blastInUse && !IsShielding() && ultimateChargePoints >= ultimateChargePointsTotal && !GameController::IsGameplayBlocked();
}

bool Onimaru::UltimateStarted() const {
	return ultimateOn && ultimateTimeRemaining > 0;
}

void Onimaru::InitShield() {
	if (shield == nullptr || shieldGO == nullptr) return;
	if (CanShield()) {
		shield->InitShield();
		if (shieldParticles) {
			shieldParticles->Play();
		}

		if (agent) {
			agent->SetMaxSpeed(shieldingMaxSpeed);
		}
		if (!shooting) {
			if (compAnimation->GetCurrentState()) {
				compAnimation->SendTriggerSecondary(compAnimation->GetCurrentState()->name + states[static_cast<int>(SHIELD)]);
			}
		}
		else {
			if (compAnimation->GetCurrentStateSecondary()) {
				compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + states[static_cast<int>(SHOOTSHIELD)]);
			}
		}
		if (onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::SHIELD_ON)]) {
			onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::SHIELD_ON)]->Play();
		}

		if (hudManagerScript) {
			hudManagerScript->StartUsingSkill(HUDManager::Cooldowns::ONIMARU_SKILL_1);
			hudManagerScript->SetCooldownRetreival(HUDManager::Cooldowns::ONIMARU_SKILL_1);
		}

		shieldGO->Enable();
	}
}

void Onimaru::FadeShield() {
	if (shield == nullptr || shieldGO == nullptr) return;
	shield->FadeShield();

	movementSpeed = normalMovementSpeed;
	if (agent) agent->SetMaxSpeed(movementSpeed);

	if (!shooting) {
		if (compAnimation->GetCurrentStateSecondary() && compAnimation->GetCurrentState()) {
			compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + compAnimation->GetCurrentState()->name);
		}
	}
	else {
		if (compAnimation->GetCurrentStateSecondary()) {
			compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + states[static_cast<int>(SHOOTING)]);
		}
	}
	if (onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::SHIELD_OFF)]) {
		onimaruAudios[static_cast<int>(ONIMARU_AUDIOS::SHIELD_OFF)]->Play();
	}

	if (hudManagerScript) {
		hudManagerScript->StopUsingSkill(HUDManager::Cooldowns::ONIMARU_SKILL_1);
	}

	shieldGO->Disable();
}

void Onimaru::Update(bool useGamepad, bool lockMovement, bool lockRotation) {
	if (shield == nullptr || shieldGO == nullptr) return;
	if (isAlive) {
		Player::Update(useGamepad, lockMovement, false);

		if (!ultimateOn) {
			if (GetInputBool(InputActions::ABILITY_3, useGamepad)) {
				if (CanUltimate()) {
					StartUltimate();
				}
			}
		}
		else {
			//Ultimate execution
			if (ultimateTimeRemaining > 0) {
				ultimateTimeRemaining -= Time::GetDeltaTime();
				Shoot();
				shooting = true;
				ultimateBullet->PlayChildParticles();
				bullet->SetParticlesPerSecond(float2(0.0f, 0.0f));

				if (ultimateTimeRemaining <= 0) {
					FinishUltimate();
					ultimateBullet->StopChildParticles();
				}
			}
		}

		if (!ultimateOn) {
			if (GetInputBool(InputActions::ABILITY_1, useGamepad)) {
				if (!shield->GetIsActive() && shield->CanUse() && !blastInUse) {
					InitShield();
				}
			}

			if ((!GetInputBool(InputActions::ABILITY_1, useGamepad) || !shield->CanUse()) && shield->GetIsActive()) {
				FadeShield();
			}

			if (GetInputBool(InputActions::SHOOT, useGamepad)) {
				if (CanShoot()) {
					if (!shooting) {
						shooting = true;
						if (bullet) {
							bullet->PlayChildParticles();
							bullet->SetParticlesPerSecond(float2(attackSpeed, attackSpeed));
						}
						if (compAnimation) {
							if (!shield->GetIsActive()) {
								if (compAnimation->GetCurrentState()) {
									compAnimation->SendTriggerSecondary(compAnimation->GetCurrentState()->name + states[static_cast<int>(SHOOTING)]);
								}
							}
							else {
								if (compAnimation->GetCurrentStateSecondary()) {
									compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + states[static_cast<int>(SHOOTSHIELD)]);
								}
							}
						}
					}
					else {
						Shoot();
					}
				}
			}
		}

		if (shooting) {
			if (!GetInputBool(InputActions::SHOOT, useGamepad) || GameController::IsGameplayBlocked()) {
				shooting = false;
				if (compAnimation) {
					if (shield->GetIsActive()) {
						if (compAnimation->GetCurrentState() && compAnimation->GetCurrentStateSecondary()) {
							compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + states[static_cast<int>(SHIELD)]);
						}
					}
					else {
						if (compAnimation->GetCurrentStateSecondary() && compAnimation->GetCurrentState()) {
							compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + compAnimation->GetCurrentState()->name);
						}
					}
					if (bullet) bullet->StopChildParticles();
				}
			}
		}

		if (CanBlast()) {
			if (GetInputBool(InputActions::ABILITY_2, useGamepad)) {
				blastInUse = true;
				if (compAnimation && compAnimation->GetCurrentState()) {
					if (shooting) {
						if (compAnimation->GetCurrentStateSecondary()) {
							compAnimation->SendTriggerSecondary(compAnimation->GetCurrentStateSecondary()->name + states[static_cast<int>(SHOOTBLAST)]);
						}
					}
					else {
						compAnimation->SendTriggerSecondary(compAnimation->GetCurrentState()->name + states[static_cast<int>(BLAST)]);
					}
				}

				if (hudManagerScript) {
					hudManagerScript->SetCooldownRetreival(HUDManager::Cooldowns::ONIMARU_SKILL_2);
				}

				blastCooldownRemaining = blastCooldown;
				Blast();
			}
		}
	}
	else {
		if (agent) agent->RemoveAgentFromCrowd();
		movementInputDirection = MovementDirection::NONE;
		if (bullet) bullet->StopChildParticles();
	}

	if (blastInUse) {
		Blast();
	}
	PlayAnimation();
}

float Onimaru::GetRealUltimateCooldown() {
	return static_cast<float>(ultimateChargePoints) / static_cast<float>(ultimateChargePointsTotal);
}

float Onimaru::GetRealBlastCooldown() {
	return 1.f - (blastCooldownRemaining / blastCooldown);
}

void Onimaru::AddEnemy(GameObject* enemy) {
	enemiesInMap.push_back(enemy);
}

void Onimaru::RemoveEnemy(GameObject* enemy) {
	std::vector<GameObject*>::iterator enemyToRemove;
	enemyToRemove = std::find(enemiesInMap.begin(), enemiesInMap.end(), enemy);

	if (enemyToRemove != enemiesInMap.end()) {
		enemiesInMap.erase(enemyToRemove);
	}
}