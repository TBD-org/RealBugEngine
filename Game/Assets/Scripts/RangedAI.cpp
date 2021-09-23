#include "RangedAI.h"

#include "GameController.h"
#include "PlayerController.h"
#include "PlayerDeath.h"
#include "HUDController.h"
#include "AIMovement.h"
#include "RangerProjectileScript.h"
#include "EnemySpawnPoint.h"
#include "Onimaru.h"

#include "GameObject.h"
#include "GameplaySystems.h"
#include "Components/ComponentTransform.h"
#include "Components/ComponentAudioSource.h"
#include "Components/ComponentAgent.h"
#include "Components/ComponentAnimation.h"
#include "Components/ComponentMeshRenderer.h"
#include "Resources/ResourcePrefab.h"
//clang-format off
#include <random>

#define HIERARCHY_POSITION_WEAPON 2
#define HIERARCHY_POSITION_BACKPACK 3

EXPOSE_MEMBERS(RangedAI) {
	MEMBER(MemberType::GAME_OBJECT_UID, playerUID),
		MEMBER(MemberType::GAME_OBJECT_UID, materialsUID),
		MEMBER(MemberType::GAME_OBJECT_UID, fangUID),
		MEMBER(MemberType::GAME_OBJECT_UID, playerMeshUIDFang),
		MEMBER(MemberType::GAME_OBJECT_UID, playerMeshUIDOnimaru),
		MEMBER(MemberType::GAME_OBJECT_UID, meshUID1),
		MEMBER(MemberType::GAME_OBJECT_UID, meshUID2),
		MEMBER_SEPARATOR("Enemy stats"),
		MEMBER(MemberType::FLOAT, rangerGruntCharacter.movementSpeed),
		MEMBER(MemberType::FLOAT, rangerGruntCharacter.lifePoints),
		MEMBER(MemberType::FLOAT, rangerGruntCharacter.searchRadius),
		MEMBER(MemberType::FLOAT, rangerGruntCharacter.attackRange),
		MEMBER(MemberType::FLOAT, rangerGruntCharacter.barrelDamageTaken),
		MEMBER_SEPARATOR("Push variables"),
		MEMBER(MemberType::FLOAT, rangerGruntCharacter.pushBackDistance),
		MEMBER(MemberType::FLOAT, rangerGruntCharacter.pushBackSpeed),
		MEMBER(MemberType::FLOAT, rangerGruntCharacter.slowedDownSpeed),
		MEMBER(MemberType::FLOAT, rangerGruntCharacter.slowedDownTime),
		MEMBER(MemberType::FLOAT, attackSpeed),
		MEMBER(MemberType::FLOAT, fleeingRange),
		MEMBER(MemberType::GAME_OBJECT_UID, dmgMaterialObj),
		MEMBER(MemberType::FLOAT, timeSinceLastHurt),
		MEMBER(MemberType::FLOAT, approachOffset), //This variable should be a positive float, it will be used to make AIs get a bit closer before stopping their approach
		MEMBER(MemberType::FLOAT, stunDuration),
		MEMBER(MemberType::FLOAT, hurtFeedbackTimeDuration),
		MEMBER(MemberType::FLOAT, groundPosition),
		MEMBER(MemberType::FLOAT, fleeingUpdateTime),
		MEMBER_SEPARATOR("Dissolve properties"),
		MEMBER(MemberType::GAME_OBJECT_UID, dissolveMaterialObj),
		MEMBER(MemberType::FLOAT, dissolveTimerToStart)
};//clang-format on

GENERATE_BODY_IMPL(RangedAI);

void RangedAI::Start() {
	player = GameplaySystems::GetGameObject(playerUID);
	meshObj = GetOwner().GetChildren()[0];
	meshObjForFrustumPresenceCheck1 = GameplaySystems::GetGameObject(meshUID1);
	meshObjForFrustumPresenceCheck2 = GameplaySystems::GetGameObject(meshUID2);
	animation = GetOwner().GetComponent<ComponentAnimation>();
	ownerTransform = GetOwner().GetComponent<ComponentTransform>();
	fangMeshObj = GameplaySystems::GetGameObject(playerMeshUIDFang);
	onimaruMeshObj = GameplaySystems::GetGameObject(playerMeshUIDOnimaru);
	
	weapon = GetOwner().GetChild("Weapon")->GetChild("WeaponParticles");
	if (weapon) {
		shootTrailPrefab = weapon->GetComponent<ComponentParticleSystem>();
	}

	if (meshObj) {
		ComponentBoundingBox* bb = meshObj->GetComponent<ComponentBoundingBox>();
		bbCenter = (bb->GetLocalMinPointAABB() + bb->GetLocalMaxPointAABB()) / 2;
		meshRenderer = meshObj->GetComponent<ComponentMeshRenderer>();
	}

	int numChildren = GetOwner().GetChildren().size();
	if (numChildren > HIERARCHY_POSITION_WEAPON) {
		GameObject* weaponGO = GetOwner().GetChildren()[HIERARCHY_POSITION_WEAPON];
		if (weaponGO) {
			weaponMeshRenderer = weaponGO->GetComponent<ComponentMeshRenderer>();
			if (weaponMeshRenderer) {
				weaponMaterialID = weaponMeshRenderer->GetMaterialID();
			}
		}
	}

	if (numChildren > HIERARCHY_POSITION_BACKPACK) {
		GameObject* backpackGO = GetOwner().GetChildren()[HIERARCHY_POSITION_BACKPACK];
		if (backpackGO) {
			backpackMeshRenderer = backpackGO->GetComponent<ComponentMeshRenderer>();
			if (backpackMeshRenderer) {
				backpackMaterialID = backpackMeshRenderer->GetMaterialID();
			}
		}
	}

	GameObject* damagedObj = GameplaySystems::GetGameObject(dmgMaterialObj);
	if (damagedObj) {
		ComponentMeshRenderer* dmgMeshRenderer = damagedObj->GetComponent<ComponentMeshRenderer>();
		if (dmgMeshRenderer) {
			damageMaterialID = dmgMeshRenderer->GetMaterialID();
		}
	}

	GameObject* dissolveObj = GameplaySystems::GetGameObject(dissolveMaterialObj);
	if (dissolveObj) {
		ComponentMeshRenderer* dissolveMeshRenderer = dissolveObj->GetComponent<ComponentMeshRenderer>();
		if (dissolveMeshRenderer) {
			dissolveMaterialID = dissolveMeshRenderer->GetMaterialID();
		}
	}

	agent = GetOwner().GetComponent<ComponentAgent>();
	if (agent) {
		agent->SetMaxSpeed(rangerGruntCharacter.movementSpeed);
		agent->SetMaxAcceleration(static_cast<float>(AIMovement::maxAcceleration));
		agent->SetAgentObstacleAvoidance(true);
		agent->RemoveAgentFromCrowd();
	}

	if (player) {
		playerController = GET_SCRIPT(player, PlayerController);
	}
	else {
		player = GameplaySystems::GetGameObject("Player");
		if (player) {
			playerController = GET_SCRIPT(player, PlayerController);
		}
	}

	fang = GameplaySystems::GetGameObject(fangUID);

	if (fang) {
		playerDeath = GET_SCRIPT(fang, PlayerDeath);
	}
	if (fangMeshObj == nullptr) {
		if (player != nullptr) {
			GameObject* fangObj = player->GetChild("Fang");
			if (fangObj != nullptr) {
				fangMeshObj = fangObj->GetChild("Fang");
			}
		}
	}

	if (onimaruMeshObj == nullptr) {
		if (player != nullptr) {
			GameObject* onimaruObj = player->GetChild("Onimaru");
			if (onimaruObj != nullptr) {
				onimaruMeshObj = onimaruObj->GetChild("Onimaru");
			}
		}
	}

	aiMovement = GET_SCRIPT(&GetOwner(), AIMovement);

	// TODO: ADD CHECK PLS
	ChangeState(AIState::START);

	int i = 0;
	for (ComponentAudioSource& src : GetOwner().GetComponents<ComponentAudioSource>()) {
		if (i < static_cast<int>(AudioType::TOTAL)) audios[i] = &src;
		++i;
	}

	enemySpawnPointScript = GET_SCRIPT(GetOwner().GetParent(), EnemySpawnPoint);

	pushBackRealDistance = rangerGruntCharacter.pushBackDistance;
	SetRandomMaterial();
}

void RangedAI::OnAnimationFinished() {
	if (animation == nullptr) return;
	if (agent == nullptr) return;

	if (state == AIState::SPAWN) {
		animation->SendTrigger("SpawnIdle");
		agent->AddAgentToCrowd();
		state = AIState::IDLE;
	}
	else if (state == AIState::STUNNED) {
		State* current = animation->GetCurrentState();
		if (current->name == "BeginStun") {
			animation->SendTrigger("BeginStunStunned");
		}
		else if (current->name == "EndStun") {
			animation->SendTrigger("EndStunIdle");
			agent->AddAgentToCrowd();
			state = AIState::IDLE;
		}
	}
	else if (state == AIState::DEATH) {
		rangerGruntCharacter.destroying = true;
	}
}

void RangedAI::OnAnimationSecondaryFinished() {
	std::string currentStateString = "";
	if (!animation) return;
	if (shot) {
		if (animation->GetCurrentState()) {
			animation->SendTriggerSecondary("Shoot" + animation->GetCurrentState()->name);
			shot = false;
		}
	}
}

void RangedAI::OnAnimationEvent(StateMachineEnum stateMachineEnum, const char* eventName) {
	if (stateMachineEnum == StateMachineEnum::PRINCIPAL) {
		if (eventName == "FootstepRight") {
			if (audios[static_cast<int>(AudioType::FOOTSTEP_RIGHT)]) audios[static_cast<int>(AudioType::FOOTSTEP_RIGHT)]->Play();
		}
		else if (eventName == "FootstepLeft") {
			if (audios[static_cast<int>(AudioType::FOOTSTEP_LEFT)]) audios[static_cast<int>(AudioType::FOOTSTEP_LEFT)]->Play();
		}
	}
}
void RangedAI::ParticleHit(GameObject& collidedWith, void* particle, Player& player) {
	if (!particle) return;
	ComponentParticleSystem::Particle* p = (ComponentParticleSystem::Particle*)particle;
	ComponentParticleSystem* pSystem = collidedWith.GetComponent<ComponentParticleSystem>();
	if (pSystem) pSystem->KillParticle(p);
	if (state == AIState::STUNNED && player.level2Upgrade) {
		rangerGruntCharacter.GetHit(99);
	}
	else {
		rangerGruntCharacter.GetHit(player.damageHit + playerController->GetOverPowerMode());
	}
}

void RangedAI::OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle) {
	if (state != AIState::START && state != AIState::SPAWN && state != AIState::DEATH) {
		if (rangerGruntCharacter.isAlive && playerController) {
			bool hitTaken = false;
			if (collidedWith.name == "FangBullet") {
				hitTaken = true;
				ParticleHit(collidedWith, particle, playerController->playerFang);
				rangerGruntCharacter.GetHit(playerController->playerFang.damageHit + playerController->GetOverPowerMode());
			}
			else if (collidedWith.name == "FangRightBullet" || collidedWith.name == "FangLeftBullet") {
				hitTaken = true;
				ParticleHit(collidedWith, particle, playerController->playerFang);
			}
			else if (collidedWith.name == "OnimaruBullet") {
				hitTaken = true;
				ParticleHit(collidedWith, particle, playerController->playerOnimaru);
			}
			else if (collidedWith.name == "OnimaruBulletUltimate") {
				hitTaken = true;
				ParticleHit(collidedWith, particle, playerController->playerOnimaru);
			}
			else if (collidedWith.name == "Barrel") {
				rangerGruntCharacter.GetHit(rangerGruntCharacter.barrelDamageTaken);
				hitTaken = true;
			}
			else if (collidedWith.name == "DashDamage" && playerController->playerFang.level1Upgrade) {
				hitTaken = true;
				rangerGruntCharacter.GetHit(playerController->playerFang.dashDamage + playerController->GetOverPowerMode());
			}
			else if (collidedWith.name == "RangerProjectile" && playerController->playerOnimaru.level1Upgrade) {
				hitTaken = true;
				rangerGruntCharacter.GetHit(playerController->playerOnimaru.shieldReboundedDamage + playerController->GetOverPowerMode());
				GameplaySystems::DestroyGameObject(&collidedWith);
			}

			if (hitTaken) {
				PlayHit();
			}

			if (collidedWith.name == "EMP") {
				if (agent) agent->RemoveAgentFromCrowd();
				stunTimeRemaining = stunDuration;
				if (state != AIState::STUNNED) ChangeState(AIState::STUNNED);
			}
		}
	}
}

void RangedAI::Update() {
	if (!agent) return;

	if (!dissolveAlreadyStarted && meshRenderer) {
		if (timeSinceLastHurt < hurtFeedbackTimeDuration) {
			timeSinceLastHurt += Time::GetDeltaTime();
			if (timeSinceLastHurt > hurtFeedbackTimeDuration) {
				SetMaterial(meshRenderer, defaultMaterialID);
				SetMaterial(backpackMeshRenderer, backpackMaterialID);
				SetMaterial(weaponMeshRenderer, weaponMaterialID);
			}
		}
	}

	UpdateDissolveTimer();

	if (!GetOwner().IsActive()) return;

	if ((state == AIState::IDLE || state == AIState::RUN || state == AIState::FLEE) && !GameController::IsGameplayBlocked()) {
		attackTimePool = Max(attackTimePool - Time::GetDeltaTime(), 0.0f);
		if (attackTimePool == 0) {
			if (actualShotTimer == -1) {
				ShootPlayerInRange();
			}
		}
	}

	if (actualShotTimer > 0) {
		actualShotTimer = Max(actualShotTimer - Time::GetDeltaTime(), 0.0f);
		if (actualShotTimer == 0) {
			ActualShot();
		}
	}

	if (rangerGruntCharacter.slowedDown) {
		if (currentSlowedDownTime >= rangerGruntCharacter.slowedDownTime) {
			agent->SetMaxSpeed(rangerGruntCharacter.movementSpeed);
			rangerGruntCharacter.slowedDown = false;
		}
		currentSlowedDownTime += Time::GetDeltaTime();
	}

	if (!rangerGruntCharacter.isAlive && state != AIState::DEATH && !GameController::IsGameplayBlocked()) {
		PlayAudio(AudioType::DEATH);
		ComponentCapsuleCollider* collider = GetOwner().GetComponent<ComponentCapsuleCollider>();
		if (collider) collider->Disable();
		if (rangerGruntCharacter.beingPushed) DisableBlastPushBack();
		ChangeState(AIState::DEATH);
		if (playerController) playerController->RemoveEnemyFromMap(&GetOwner());
	}

	UpdateState();
}

void RangedAI::EnterState(AIState newState) {
	if (!agent) return;
	if (!animation) return;

	switch (newState) {
	case AIState::START:
		break;
	case AIState::SPAWN:
		PlayAudio(AudioType::SPAWN);
		break;
	case AIState::IDLE:
		if (state == AIState::FLEE) {
			animation->SendTrigger("RunBackwardIdle");
		}
		else if (state == AIState::RUN) {
			animation->SendTrigger("RunForwardIdle");
		}

		if (aiMovement) aiMovement->Stop();
		break;
	case AIState::RUN:
		if (state == AIState::IDLE) {
			animation->SendTrigger("IdleRunForward");
		}
		else if (state == AIState::FLEE) {
			animation->SendTrigger("RunBackwardRunForward");
		}
		break;
	case AIState::FLEE:
		if (state == AIState::RUN) {
			animation->SendTrigger("RunForwardRunBackward");
		}
		else if (state == AIState::IDLE) {
			animation->SendTrigger("IdleRunBackward");
		}
		break;
	case AIState::STUNNED:
		if (shot) {
			animation->SendTriggerSecondary("ShootBeginStun");
		}
		if (animation->GetCurrentState()) {
			animation->SendTrigger(animation->GetCurrentState()->name + "BeginStun");
		}
		break;
	case AIState::DEATH:
		if (enemySpawnPointScript) enemySpawnPointScript->UpdateRemainingEnemies();
		if (playerController) {
			if (playerController->playerOnimaru.characterGameObject->IsActive()) {
				playerController->playerOnimaru.IncreaseUltimateCounter();
			}
			else if (playerController->playerFang.characterGameObject->IsActive()) {
				playerController->playerFang.IncreaseUltimateCounter();
			}
		}

		if (shot) {
			animation->SendTriggerSecondary("ShootDeath");
		}
		std::string changeState = animation->GetCurrentState()->name + "Death";
		deathType = 1 + rand() % 2;
		std::string deathTypeStr = std::to_string(deathType);
		animation->SendTrigger(changeState + deathTypeStr);
		agent->RemoveAgentFromCrowd();
		state = AIState::DEATH;
		break;
	}
}

void RangedAI::UpdateState() {
	if (!animation) return;

	float speedToUse = rangerGruntCharacter.slowedDown ? rangerGruntCharacter.slowedDownSpeed : rangerGruntCharacter.movementSpeed;

	if (GameController::IsGameplayBlocked() && state != AIState::START && state != AIState::SPAWN) {
		state = AIState::IDLE;
	}

	switch (state) {
	case AIState::START:
		if (aiMovement) aiMovement->Seek(state, float3(ownerTransform->GetGlobalPosition().x, 0, ownerTransform->GetGlobalPosition().z), rangerGruntCharacter.fallingSpeed, true);
		if (ownerTransform->GetGlobalPosition().y < 3.5f + 0e-5f) {
			ownerTransform->SetGlobalPosition(float3(ownerTransform->GetGlobalPosition().x, groundPosition, ownerTransform->GetGlobalPosition().z));
			animation->SendTrigger("StartSpawn");
			ChangeState(AIState::SPAWN);
		}
		break;
	case AIState::SPAWN:
		break;
	case AIState::IDLE:
		if (player) {
			if (aiMovement) {
				aiMovement->Stop();
				if (aiMovement->CharacterInSight(player, rangerGruntCharacter.searchRadius) && !GameController::IsGameplayBlocked()) {
					if (aiMovement->CharacterInSight(player, fleeingRange)) {
						ChangeState(AIState::FLEE);
						break;
					}

					if (!CharacterInRange(player, rangerGruntCharacter.attackRange, true)) {
						ChangeState(AIState::RUN);
						break;
					}

					OrientateTo(player->GetComponent<ComponentTransform>()->GetGlobalPosition() - ownerTransform->GetGlobalPosition());
				}
				else {
					if (animation->GetCurrentState()->name != "Idle") animation->SendTrigger(animation->GetCurrentState()->name + "Idle");
				}
			}
		}
		break;
	case AIState::RUN:
		if (aiMovement) {
			if (aiMovement->CharacterInSight(player, rangerGruntCharacter.searchRadius)) {
				OrientateTo(player->GetComponent<ComponentTransform>()->GetGlobalPosition() - ownerTransform->GetGlobalPosition());

				if (!CharacterInRange(player, rangerGruntCharacter.attackRange - approachOffset, true)) {
					if (!aiMovement->CharacterInSight(player, fleeingRange)) {
						if (aiMovement) aiMovement->Seek(state, player->GetComponent<ComponentTransform>()->GetGlobalPosition(), static_cast<int>(speedToUse), false);
					}
					else {
						ChangeState(AIState::FLEE);
					}
				}
				else {
					ChangeState(AIState::IDLE);
				}
			}
		}
		break;
	case AIState::FLEE:
		OrientateTo(player->GetComponent<ComponentTransform>()->GetGlobalPosition() - ownerTransform->GetGlobalPosition());

		if (aiMovement->CharacterInSight(player, fleeingRange)) {
			if (aiMovement) {
				if (currentFleeingUpdateTime > fleeingUpdateTime && !fleeingFarAway) {  //Detecting it is time to move far away from player
					if (animation->GetCurrentState() && animation->GetCurrentState()->name != "RunBackward") {
						animation->SendTrigger(animation->GetCurrentState()->name + "RunBackward");
						currentFleeDestination = ownerTransform->GetGlobalPosition() + (ownerTransform->GetFront() * -1 * fleeingRange);
					}
					fleeingFarAway = true;
				}
				else if (currentFleeingUpdateTime >= 0 && fleeingFarAway) {   //Moving far away from player
					currentFleeingUpdateTime -= Time::GetDeltaTime();
					aiMovement->Seek(state, currentFleeDestination, static_cast<int>(speedToUse), false);

					if (currentFleeingUpdateTime <= 0) {
						fleeingFarAway = false;
						currentFleeingUpdateTime = 0;
					}
				}
				else { //Staying in same position
					if (animation->GetCurrentState() && animation->GetCurrentState()->name != "Idle") animation->SendTrigger(animation->GetCurrentState()->name + "Idle");
					currentFleeingUpdateTime += Time::GetDeltaTime();
					aiMovement->Stop();
				}
			}
		}
		else {
			currentFleeingUpdateTime = 0;
			fleeingFarAway = false;
			ChangeState(AIState::IDLE);
		}
		break;
	case AIState::STUNNED:
		if (stunTimeRemaining <= 0.f) {
			stunTimeRemaining = 0.f;
			animation->SendTrigger("StunnedEndStun");
		}
		else {
			stunTimeRemaining -= Time::GetDeltaTime();
		}
		break;
	case AIState::DEATH:
		if (!dissolveAlreadyStarted) {
			dissolveAlreadyStarted = true;
		}
		if (rangerGruntCharacter.destroying) {
			if (meshRenderer && meshRenderer->HasDissolveAnimationFinished()) {
				if (playerController) playerController->RemoveEnemyFromMap(&GetOwner());
				GameplaySystems::DestroyGameObject(&GetOwner());
			}
		}

		break;
	case AIState::PUSHED:
		UpdatePushBackPosition();
		break;
	default:
		break;
	}
}

void RangedAI::ChangeState(AIState newState) {
	EnterState(newState);
	state = newState;
}

bool RangedAI::CharacterInRange(const GameObject* character, float range, bool useRange) {
	bool inFrustum0 = meshObj != nullptr ? Camera::CheckObjectInsideFrustum(meshObj) : true;
	bool inFrustum1 = meshObjForFrustumPresenceCheck1 != nullptr ? Camera::CheckObjectInsideFrustum(meshObjForFrustumPresenceCheck1) : true;
	bool inFrustum2 = meshObjForFrustumPresenceCheck2 != nullptr ? Camera::CheckObjectInsideFrustum(meshObjForFrustumPresenceCheck2) : true;
	bool inFrustum = inFrustum0 && inFrustum1 && inFrustum2;
	if (!useRange) {
		return inFrustum;
	}

	ComponentTransform* target = character->GetComponent<ComponentTransform>();
	if (target) {
		float3 posTarget = target->GetGlobalPosition();
		return posTarget.Distance(ownerTransform->GetGlobalPosition()) < range && inFrustum;
	}
	return false;
}

bool RangedAI::FindsRayToPlayer(bool useForward) {
	if (!meshObj) return false;

	ComponentBoundingBox* box = meshObj->GetComponent<ComponentBoundingBox>();
	float3 offset(0, 0, 0);

	if (box) {
		float y = (box->GetWorldAABB().maxPoint + box->GetWorldAABB().minPoint).y / 4;
		offset.y = y;
	}

	float3 start = GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition() + offset;

	if (fangMeshObj != nullptr && onimaruMeshObj != nullptr) {
		GameObject* activePlayerMeshObj = fangMeshObj;

		if (!activePlayerMeshObj->IsActive()) {
			activePlayerMeshObj = onimaruMeshObj;
		}

		float3 dir = GetOwner().GetComponent<ComponentTransform>()->GetGlobalRotation() * float3(0, 0, 1);

		if (!useForward) {
			dir = activePlayerMeshObj->GetComponent<ComponentTransform>()->GetGlobalPosition() - start;
			dir.y = 0;
			dir.Normalize();
		}

		int mask = static_cast<int>(MaskType::PLAYER);
		GameObject* hitGo = Physics::Raycast(start, start + dir * rangerGruntCharacter.attackRange, mask);
		return hitGo != nullptr;
	}
	return true;
}

void RangedAI::OrientateTo(const float3& direction) {
	Quat newRotation = Quat::LookAt(float3(0, 0, 1), direction.Normalized(), float3(0, 1, 0), float3(0, 1, 0));
	ownerTransform->SetGlobalRotation(newRotation);
}

void RangedAI::ActualShot() {
	if (shootTrailPrefab) {
		//TODO WAIT STRETCH FROM LOWY AND IMPLEMENT SOME SHOOT EFFECT
		if (!meshObj) return;

		ComponentBoundingBox* box = meshObj->GetComponent<ComponentBoundingBox>();

		float offsetY = (box->GetWorldAABB().minPoint.y + box->GetWorldAABB().maxPoint.y) / 4;
		shootTrailPrefab->PlayChildParticles();
	}

	attackTimePool = 1.0f / attackSpeed;
	actualShotTimer = -1.0f;

	PlayAudio(AudioType::SHOOT);
}

void RangedAI::PlayAudio(AudioType audioType) {
	if (audios[static_cast<int>(audioType)]) audios[static_cast<int>(audioType)]->Play();
}

void RangedAI::EnableBlastPushBack() {
	if (state != AIState::START && state != AIState::SPAWN && state != AIState::DEATH) {
		ChangeState(AIState::PUSHED);
		rangerGruntCharacter.beingPushed = true;
		CalculatePushBackRealDistance();
		// Damage
		if (playerController->playerOnimaru.level2Upgrade) {
			rangerGruntCharacter.GetHit(playerController->playerOnimaru.blastDamage + playerController->GetOverPowerMode());

			PlayAudio(AudioType::HIT);
			PlayHitMaterialEffect();
			timeSinceLastHurt = 0.0f;
		}
	}
}

void RangedAI::DisableBlastPushBack() {
	if (state != AIState::START && state != AIState::SPAWN && state != AIState::DEATH) {
		ChangeState(AIState::IDLE);
		rangerGruntCharacter.beingPushed = false;
	}
}

bool RangedAI::IsBeingPushed() const {
	return rangerGruntCharacter.beingPushed;
}

void RangedAI::PlayHit()
{
	PlayAudio(AudioType::HIT);
	PlayHitMaterialEffect();
	timeSinceLastHurt = 0.0f;
}

void RangedAI::ShootPlayerInRange() {
	if (!player) return;
	if (!playerController) return;
	if (!playerController->playerFang.characterGameObject || !playerController->playerOnimaru.characterGameObject) return;
	if (playerController->IsPlayerDead()) return;
	if (CharacterInRange(player, rangerGruntCharacter.attackRange, true)) {
		shot = true;

		if (animation) {
			if (animation->GetCurrentState() && animation->GetCurrentState()->name != "Shoot") animation->SendTriggerSecondary(animation->GetCurrentState()->name + "Shoot");
		}

		actualShotTimer = actualShotMaxTime;
	}
}

void RangedAI::UpdatePushBackPosition() {
	float3 playerPos = player->GetComponent<ComponentTransform>()->GetGlobalPosition();
	float3 enemyPos = GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition();
	float3 initialPos = enemyPos;

	float3 direction = (enemyPos - playerPos).Normalized();

	if (agent) {
		enemyPos += direction * rangerGruntCharacter.pushBackSpeed * Time::GetDeltaTime();
		agent->SetMoveTarget(enemyPos, false);
		agent->SetMaxSpeed(rangerGruntCharacter.pushBackSpeed);
		float distance = enemyPos.Distance(initialPos);
		currentPushBackDistance += distance;

		if (currentPushBackDistance >= pushBackRealDistance) {
			agent->SetMaxSpeed(rangerGruntCharacter.slowedDownSpeed);
			DisableBlastPushBack();
			rangerGruntCharacter.slowedDown = true;
			currentPushBackDistance = 0.f;
			currentSlowedDownTime = 0.f;
			pushBackRealDistance = rangerGruntCharacter.pushBackDistance;
		}
	}
}

void RangedAI::CalculatePushBackRealDistance() {
	float3 playerPos = player->GetComponent<ComponentTransform>()->GetGlobalPosition();
	float3 enemyPos = GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition();

	float3 direction = (enemyPos - playerPos).Normalized();

	bool hitResult = false;

	float3 finalPos = enemyPos + direction * rangerGruntCharacter.pushBackDistance;
	float3 resultPos = { 0,0,0 };

	Navigation::Raycast(enemyPos, finalPos, hitResult, resultPos);

	if (hitResult) {
		pushBackRealDistance = resultPos.Distance(enemyPos) - 1; // Should be agent radius but it's not exposed
	}
}

void RangedAI::PlayHitMaterialEffect()
{
	if (!dissolveAlreadyStarted) {
		SetMaterial(meshRenderer, damageMaterialID);
		SetMaterial(backpackMeshRenderer, damageMaterialID);
		SetMaterial(weaponMeshRenderer, damageMaterialID);
	}
}

void RangedAI::UpdateDissolveTimer() {
	if (dissolveAlreadyStarted && !dissolveAlreadyPlayed) {
		if (currentDissolveTime >= dissolveTimerToStart) {

			SetMaterial(meshRenderer, dissolveMaterialID, true);
			SetMaterial(weaponMeshRenderer, dissolveMaterialID, true);
			SetMaterial(backpackMeshRenderer, dissolveMaterialID, true);

			dissolveAlreadyPlayed = true;
		}
		else {
			currentDissolveTime += Time::GetDeltaTime();
		}
	}
}

void RangedAI::SetRandomMaterial()
{
	if (!meshRenderer) return;
	GameObject* materialsHolder = GameplaySystems::GetGameObject(materialsUID);

	if (materialsHolder) {
		std::vector<UID> materials;
		for (const auto& child : materialsHolder->GetChildren()) {
			ComponentMeshRenderer* meshRenderer = child->GetComponent<ComponentMeshRenderer>();
			if (meshRenderer && meshRenderer->GetMaterialID()) {
				materials.push_back(meshRenderer->GetMaterialID());
			}
		}


		if (!materials.empty()) {
			//Random distribution it cant be saved into global 
			std::random_device rd;  //Will be used to obtain a seed for the random number engine
			std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
			std::uniform_int_distribution<int> distrib(1, materials.size());

			int position = distrib(gen) - 1;
			meshRenderer->ChangeMaterial(materials[position]);
			defaultMaterialID = materials[position];
		}
	}
}

void RangedAI::SetMaterial(ComponentMeshRenderer* mesh, UID newMaterialID, bool needToPlayDissolve) {
	if (newMaterialID > 0 && mesh) {
		mesh->ChangeMaterial(newMaterialID);
		if (needToPlayDissolve) {
			mesh->PlayDissolveAnimation();
		}
	}
}