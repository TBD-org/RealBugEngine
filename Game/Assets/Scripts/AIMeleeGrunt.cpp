#include "AIMeleeGrunt.h"

#include "GameObject.h"
#include "GameplaySystems.h"
#include "GameController.h"
#include "PlayerController.h"
#include "PlayerDeath.h"
#include "EnemySpawnPoint.h"
#include "HUDController.h"
#include "AIMovement.h"
#include "Onimaru.h"

#include <math.h>
#include <random>


EXPOSE_MEMBERS(AIMeleeGrunt) {
	MEMBER(MemberType::GAME_OBJECT_UID, playerUID),
		MEMBER(MemberType::GAME_OBJECT_UID, materialsUID),
		MEMBER(MemberType::GAME_OBJECT_UID, fangUID),
		MEMBER(MemberType::GAME_OBJECT_UID, damageMaterialPlaceHolderUID),
		MEMBER_SEPARATOR("Enemy stats"),
		MEMBER(MemberType::FLOAT, gruntCharacter.lifePoints),
		MEMBER(MemberType::FLOAT, gruntCharacter.movementSpeed),
		MEMBER(MemberType::FLOAT, gruntCharacter.damageHit),
		MEMBER(MemberType::INT, gruntCharacter.fallingSpeed),
		MEMBER(MemberType::FLOAT, gruntCharacter.searchRadius),
		MEMBER(MemberType::FLOAT, gruntCharacter.attackRange),
		MEMBER(MemberType::FLOAT, gruntCharacter.barrelDamageTaken),
		MEMBER_SEPARATOR("Push variables"),
		MEMBER(MemberType::FLOAT, gruntCharacter.pushBackDistance),
		MEMBER(MemberType::FLOAT, gruntCharacter.pushBackSpeed),
		MEMBER(MemberType::FLOAT, gruntCharacter.slowedDownSpeed),
		MEMBER(MemberType::FLOAT, gruntCharacter.slowedDownTime),
		MEMBER_SEPARATOR("Stun variables"),
		MEMBER(MemberType::FLOAT, hurtFeedbackTimeDuration),
		MEMBER(MemberType::FLOAT, stunDuration),
		MEMBER(MemberType::FLOAT, groundPosition),
		MEMBER_SEPARATOR("Attack1"),
		MEMBER(MemberType::FLOAT, att1AttackSpeed),
		MEMBER(MemberType::FLOAT, att1MovementSpeedWhileAttacking),
		MEMBER(MemberType::INT, att1AbilityChance),
		MEMBER_SEPARATOR("Attack2"),
		MEMBER(MemberType::FLOAT, att2AttackSpeed),
		MEMBER(MemberType::FLOAT, att2MovementSpeedWhileAttacking),
		MEMBER(MemberType::INT, att2AbilityChance),
		MEMBER_SEPARATOR("Attack3"),
		MEMBER(MemberType::FLOAT, att3AttackSpeed),
		MEMBER(MemberType::FLOAT, att3MovementSpeedWhileAttacking),
		MEMBER(MemberType::INT, att3AbilityChance),
		MEMBER_SEPARATOR("Dissolve properties"),
		MEMBER(MemberType::GAME_OBJECT_UID, dissolveMaterialObj),
		MEMBER(MemberType::GAME_OBJECT_UID, dissolveMaterialWeaponObj),
		MEMBER(MemberType::FLOAT, dissolveTimerToStart)
};

GENERATE_BODY_IMPL(AIMeleeGrunt);

void AIMeleeGrunt::Start() {
	player = GameplaySystems::GetGameObject(playerUID);

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

	agent = GetOwner().GetComponent<ComponentAgent>();
	if (agent) {
		agent->SetMaxSpeed(gruntCharacter.movementSpeed);
		agent->SetMaxAcceleration(AIMovement::maxAcceleration);
		agent->SetAgentObstacleAvoidance(true);
		agent->RemoveAgentFromCrowd();
	}

	animation = GetOwner().GetComponent<ComponentAnimation>();
	ownerTransform = GetOwner().GetComponent<ComponentTransform>();

	movementScript = GET_SCRIPT(&GetOwner(), AIMovement);
	rightBladeCollider = movementScript->SearchReferenceInHierarchy(&GetOwner(),"RightBlade");
	leftBladeCollider = movementScript->SearchReferenceInHierarchy(&GetOwner(), "LeftBlade");
	if (rightBladeCollider->IsActive()) rightBladeCollider->Disable();
	if (leftBladeCollider->IsActive()) leftBladeCollider->Disable();
	int i = 0;
	for (ComponentAudioSource& src : GetOwner().GetComponents<ComponentAudioSource>()) {
		if (i < static_cast<int>(AudioType::TOTAL)) audios[i] = &src;
		++i;
	}

	enemySpawnPointScript = GET_SCRIPT(GetOwner().GetParent(), EnemySpawnPoint);

	// Hit feedback material retrieval
	GameObject* gameObject = nullptr;
	gameObject = GameplaySystems::GetGameObject(damageMaterialPlaceHolderUID);
	if (gameObject) {
		ComponentMeshRenderer* meshRenderer = gameObject->GetComponent<ComponentMeshRenderer>();
		if (meshRenderer) {
			damageMaterialID = meshRenderer->GetMaterialID();
		}
	}

	// Body reference
	GameObject* dissolveObj = GameplaySystems::GetGameObject(dissolveMaterialObj);
	if (dissolveObj) {
		ComponentMeshRenderer* dissolveMeshRenderer = dissolveObj->GetComponent<ComponentMeshRenderer>();
		if (dissolveMeshRenderer) {
			dissolveMaterialID = dissolveMeshRenderer->GetMaterialID();
		}
	}

	// Weapon reference
	dissolveObj = GameplaySystems::GetGameObject(dissolveMaterialWeaponObj);
	if (dissolveObj) {
		ComponentMeshRenderer* dissolveMeshRenderer = dissolveObj->GetComponent<ComponentMeshRenderer>();
		if (dissolveMeshRenderer) {
			dissolveMaterialWeaponID = dissolveMeshRenderer->GetMaterialID();
		}
	}

	gameObject = &GetOwner();
	if (gameObject) {
		// Workaround get the first children - Create a Prefab overrides childs IDs
		gameObject = gameObject->GetChildren()[0];
		if (gameObject) {
			// Since we are not getting a vector of Components, this is the only workarround possible
			// !! IMPORTANT !! if the order of meshes changes, this will have to be reviewed
			int meshCount = 0;
			for (ComponentMeshRenderer& mesh : gameObject->GetComponents<ComponentMeshRenderer>()) {
				if (meshCount == 0) componentMeshRenderer = &mesh;					// Body mesh
				else if (meshCount == 1) componentMeshRendererLeftBlade = &mesh;	// Left blade
				else if (meshCount == 2) componentMeshRendererRightBlade = &mesh;	// Right blade
				else break;
				meshCount++;
			}

			if (componentMeshRendererLeftBlade) {
				bladesMaterialID = componentMeshRendererLeftBlade->GetMaterialID();
			}
		}
	}

	pushBackRealDistance = gruntCharacter.pushBackDistance;
	SetRandomMaterial();
}

void AIMeleeGrunt::Update() {
	if (!GetOwner().IsActive()) return;
	if (!player) return;
	if (!agent) return;
	if (!movementScript) return;
	if (!playerController) return;
	if (!ownerTransform) return;
	if (!animation) return;
	if (!componentMeshRenderer) return;
	if (!playerDeath) return;
	if (!rightBladeCollider || !leftBladeCollider) return;

	if (!dissolveAlreadyPlayed && componentMeshRenderer) {
		if (timeSinceLastHurt < hurtFeedbackTimeDuration) {
			timeSinceLastHurt += Time::GetDeltaTime();
			if (timeSinceLastHurt > hurtFeedbackTimeDuration) {
				SetMaterial(componentMeshRenderer, defaultMaterialID);
				SetMaterial(componentMeshRendererLeftBlade, bladesMaterialID);
				SetMaterial(componentMeshRendererRightBlade, bladesMaterialID);
			}
		}
	}

	UpdateDissolveTimer();

	float speedToUse = gruntCharacter.slowedDown ? gruntCharacter.slowedDownSpeed : gruntCharacter.movementSpeed;

	if (gruntCharacter.slowedDown) {
		if (currentSlowedDownTime >= gruntCharacter.slowedDownTime) {
			agent->SetMaxSpeed(gruntCharacter.movementSpeed);
			gruntCharacter.slowedDown = false;
		}
		currentSlowedDownTime += Time::GetDeltaTime();
	}

	if (GameController::IsGameplayBlocked() && state != AIState::START && state != AIState::SPAWN) {
		state = AIState::IDLE;
	}

	switch (state) {
	case AIState::START:
		movementScript->Seek(state, float3(ownerTransform->GetGlobalPosition().x, 0, ownerTransform->GetGlobalPosition().z), gruntCharacter.fallingSpeed, true);
		if (ownerTransform->GetGlobalPosition().y < 3.5f + 0e-5f) {
			ownerTransform->SetGlobalPosition(float3(ownerTransform->GetGlobalPosition().x, groundPosition, ownerTransform->GetGlobalPosition().z));
			animation->SendTrigger("StartSpawn");
			if (audios[static_cast<int>(AudioType::SPAWN)]) audios[static_cast<int>(AudioType::SPAWN)]->Play();
			state = AIState::SPAWN;
		}
		break;
	case AIState::SPAWN:
		break;
	case AIState::IDLE:
		if (!playerController->IsPlayerDead()) {
			if (movementScript->CharacterInSight(player, gruntCharacter.searchRadius) && !GameController::IsGameplayBlocked()) {
				animation->SendTrigger("IdleWalkForward");
				if (agent) agent->SetMaxSpeed(speedToUse);
				state = AIState::RUN;
			} else {
				movementScript->Stop();
				if (animation->GetCurrentState()->name != "Idle") animation->SendTrigger(animation->GetCurrentState()->name + "Idle");
			}
		}
		break;
	case AIState::RUN:
		movementScript->Seek(state, player->GetComponent<ComponentTransform>()->GetGlobalPosition(), speedToUse, true);
		if (movementScript->CharacterInAttackRange(player, gruntCharacter.attackRange)) {
			int random = std::rand() % 100;
			if (random < att1AbilityChance) {
				attackNumber = 1;
				attackSpeed = att1AttackSpeed;
				attackMovementSpeed = att1MovementSpeedWhileAttacking;
			}
			else if (random < att1AbilityChance + att2AbilityChance) {
				attackNumber = 2;
				attackSpeed = att2AttackSpeed;
				attackMovementSpeed = att2MovementSpeedWhileAttacking;
			}
			else {
				attackNumber = 3;
				attackSpeed = att3AttackSpeed;
				attackMovementSpeed = att3MovementSpeedWhileAttacking;
			}

			animation->SendTrigger("WalkForwardAttack" + std::to_string(attackNumber));
			movementScript->SetClipSpeed(animation->GetCurrentState()->clipUid, attackSpeed);
			if (audios[static_cast<int>(AudioType::ATTACK)]) audios[static_cast<int>(AudioType::ATTACK)]->Play();
			state = AIState::ATTACK;
		}
		break;
	case AIState::ATTACK:
		if (track) {
			movementScript->Orientate(player->GetComponent<ComponentTransform>()->GetGlobalPosition() - ownerTransform->GetGlobalPosition());
		}
		if (attackStep) {
			movementScript->Seek(state, ownerTransform->GetGlobalPosition() + ownerTransform->GetFront()*10, attackMovementSpeed, false);
		}
		else {
			movementScript->Stop();
		}
		break;
	case AIState::STUNNED:
		if (stunTimeRemaining <= 0.f) {
			stunTimeRemaining = 0.f;
			animation->SendTrigger("StunStunEnd");
		}
		else {
			stunTimeRemaining -= Time::GetDeltaTime();
		}
		break;
	case AIState::PUSHED:
		UpdatePushBackPosition();
		break;
	case AIState::DEATH:
		if (!dissolveAlreadyStarted) {
			dissolveAlreadyStarted = true;
		}
		break;
	}

	if (gruntCharacter.destroying) {
		if (!killSent) {
			if (enemySpawnPointScript) enemySpawnPointScript->UpdateRemainingEnemies();
			killSent = true;

			if (playerController) {
				if (playerController->playerOnimaru.characterGameObject->IsActive()) {
					playerController->playerOnimaru.IncreaseUltimateCounter();
				}
				else if (playerController->playerFang.characterGameObject->IsActive()) {
					playerController->playerFang.IncreaseUltimateCounter();
				}
			}
		}
		if (componentMeshRenderer && componentMeshRenderer->HasDissolveAnimationFinished()) {
			if (playerController) playerController->RemoveEnemyFromMap(&GetOwner());
			GameplaySystems::DestroyGameObject(&GetOwner());
		}
	}

	if (!gruntCharacter.isAlive) {
		Death();
	}
}

void AIMeleeGrunt::OnAnimationFinished() {
	if (state == AIState::SPAWN) {
		animation->SendTrigger("SpawnIdle");
		state = AIState::IDLE;
		agent->AddAgentToCrowd();
	}
	else if (state == AIState::STUNNED) {
		State* current = animation->GetCurrentState();
		if (current->name == "StunStart") {
			animation->SendTrigger("StunStartStun");
		}
		else if (current->name == "StunEnd") {
			animation->SendTrigger("StunEndIdle");
			agent->AddAgentToCrowd();
			state = AIState::IDLE;
		}
	}

	else if (state == AIState::DEATH) {
		gruntCharacter.destroying = true;
	}
	else if (state == AIState::ATTACK) {
		animation->SendTrigger("Attack"+ std::to_string(attackNumber) + "Idle");
		state = AIState::IDLE;
	}
}

void AIMeleeGrunt::ParticleHit(GameObject& collidedWith, void* particle, Player& player) {
	if (!particle) return;
	ComponentParticleSystem::Particle* p = (ComponentParticleSystem::Particle*)particle;
	ComponentParticleSystem* pSystem = collidedWith.GetComponent<ComponentParticleSystem>();
	if (pSystem) pSystem->KillParticle(p);
	if (state == AIState::STUNNED && player.level2Upgrade) {
		gruntCharacter.GetHit(99);
	}
	else {
		gruntCharacter.GetHit(player.damageHit + playerController->GetOverPowerMode());
	}
}

void AIMeleeGrunt::OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle) {
	if (state != AIState::START && state != AIState::SPAWN) {
		if (gruntCharacter.isAlive && playerController) {
			bool hitTaken = false;
			if (collidedWith.name == "FangBullet") {
				hitTaken = true;
				ParticleHit(collidedWith, particle, playerController->playerFang);
				gruntCharacter.GetHit(playerController->playerFang.damageHit + playerController->GetOverPowerMode());
			}else if (collidedWith.name == "FangRightBullet" || collidedWith.name == "FangLeftBullet") {
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
				hitTaken = true;
				gruntCharacter.GetHit(gruntCharacter.barrelDamageTaken);
			}
			else if (collidedWith.name == "DashDamage" && playerController->playerFang.level1Upgrade) {
				hitTaken = true;
				gruntCharacter.GetHit(playerController->playerFang.dashDamage + playerController->GetOverPowerMode());
			}
			else if (collidedWith.name == "RangerProjectile" && playerController->playerOnimaru.level1Upgrade) {
				hitTaken = true;
				gruntCharacter.GetHit(playerController->playerOnimaru.shieldReboundedDamage + playerController->GetOverPowerMode());
				GameplaySystems::DestroyGameObject(&collidedWith);
			}
			else if (collidedWith.name == "VFXShield") {
				if (state == AIState::RUN) {
					int random = std::rand() % 100;
					if (random < att1AbilityChance) {
						attackNumber = 1;
						attackSpeed = att1AttackSpeed;
						attackMovementSpeed = att1MovementSpeedWhileAttacking;
					}
					else if (random < att1AbilityChance + att2AbilityChance) {
						attackNumber = 2;
						attackSpeed = att2AttackSpeed;
						attackMovementSpeed = att2MovementSpeedWhileAttacking;
					}
					else {
						attackNumber = 3;
						attackSpeed = att3AttackSpeed;
						attackMovementSpeed = att3MovementSpeedWhileAttacking;
					}

					animation->SendTrigger("WalkForwardAttack" + std::to_string(attackNumber));
					movementScript->SetClipSpeed(animation->GetCurrentState()->clipUid, attackSpeed);
					if (audios[static_cast<int>(AudioType::ATTACK)]) audios[static_cast<int>(AudioType::ATTACK)]->Play();
					state = AIState::ATTACK;
				}
			}
			else if (collidedWith.name == "Impenetrable") {
				if (agent) {
					agent->RemoveAgentFromCrowd();
					float3 actualPenDistance = penetrationDistance.ProjectTo(collisionNormal);
					GetOwner().GetComponent<ComponentTransform>()->SetGlobalPosition(GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition() + actualPenDistance);
					agent->AddAgentToCrowd();
				}
			}

			if (hitTaken) {
				PlayHit();
			}

			if (collidedWith.name == "EMP") {
				if (state != AIState::STUNNED) {
					if (animation->GetCurrentState()) {
						animation->SendTrigger(animation->GetCurrentState()->name + "StunStart");
					}
					agent->RemoveAgentFromCrowd();
					stunTimeRemaining = stunDuration;
					state = AIState::STUNNED;
				}
			}
		}
	}
}

void AIMeleeGrunt::EnableBlastPushBack() {
	if (state != AIState::START && state != AIState::SPAWN && state != AIState::DEATH) {
		gruntCharacter.beingPushed = true;
		state = AIState::PUSHED;
		if (animation->GetCurrentState()) animation->SendTrigger(animation->GetCurrentState()->name + "Hurt");
		CalculatePushBackRealDistance();
		// Damage
		if (playerController->playerOnimaru.level2Upgrade) {
			gruntCharacter.GetHit(playerController->playerOnimaru.blastDamage + playerController->GetOverPowerMode());

			if (audios[static_cast<int>(AudioType::HIT)]) audios[static_cast<int>(AudioType::HIT)]->Play();
			PlayHitMaterialEffect();
			timeSinceLastHurt = 0.0f;
		}
	}
}

void AIMeleeGrunt::DisableBlastPushBack() {
	if (state != AIState::START && state != AIState::SPAWN && state != AIState::DEATH) {
		gruntCharacter.beingPushed = false;
		if (animation->GetCurrentState()) animation->SendTrigger(animation->GetCurrentState()->name + "Idle");
		state = AIState::IDLE;
	}
}

bool AIMeleeGrunt::IsBeingPushed() const {
	return gruntCharacter.beingPushed;
}

void AIMeleeGrunt::PlayHit()
{
	if (audios[static_cast<int>(AudioType::HIT)]) audios[static_cast<int>(AudioType::HIT)]->Play();
	PlayHitMaterialEffect();
	timeSinceLastHurt = 0.0f;
}

void AIMeleeGrunt::UpdatePushBackPosition() {
	float3 playerPos = player->GetComponent<ComponentTransform>()->GetGlobalPosition();
	float3 enemyPos = GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition();
	float3 initialPos = enemyPos;

	float3 direction = (enemyPos - playerPos).Normalized();

	if (agent) {
		enemyPos += direction * gruntCharacter.pushBackSpeed * Time::GetDeltaTime();
		agent->SetMoveTarget(enemyPos, false);
		agent->SetMaxSpeed(gruntCharacter.pushBackSpeed);
		float distance = enemyPos.Distance(initialPos);
		currentPushBackDistance += distance;

		if (currentPushBackDistance >= pushBackRealDistance) {
			DisableBlastPushBack();
			gruntCharacter.slowedDown = true;
			currentPushBackDistance = 0.f;
			currentSlowedDownTime = 0.f;
			pushBackRealDistance = gruntCharacter.pushBackDistance;
		}
	}
}

void AIMeleeGrunt::CalculatePushBackRealDistance() {
	float3 playerPos = player->GetComponent<ComponentTransform>()->GetGlobalPosition();
	float3 enemyPos = GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition();

	float3 direction = (enemyPos - playerPos).Normalized();

	bool hitResult = false;

	float3 finalPos = enemyPos + direction * gruntCharacter.pushBackDistance;
	float3 resultPos = { 0,0,0 };

	Navigation::Raycast(enemyPos, finalPos, hitResult, resultPos);

	if (hitResult) {
		pushBackRealDistance = resultPos.Distance(enemyPos) - 1; // Should be agent radius but it's not exposed
	}
}

void AIMeleeGrunt::OnAnimationEvent(StateMachineEnum stateMachineEnum, const char* eventName) {
	switch (stateMachineEnum)
	{
	case PRINCIPAL:
		if (strcmp(eventName,"FootstepRight") == 0) {
			if (audios[static_cast<int>(AudioType::FOOTSTEP_RIGHT)]) audios[static_cast<int>(AudioType::FOOTSTEP_RIGHT)]->Play();
		}
		else if (strcmp(eventName, "FootstepLeft") == 0) {
			if (audios[static_cast<int>(AudioType::FOOTSTEP_LEFT)]) audios[static_cast<int>(AudioType::FOOTSTEP_LEFT)]->Play();
		}
		if(attackNumber == 1 || attackNumber == 2){
			if (strcmp(eventName, "StopTrack") == 0) {
				track = false;
			}
			if (strcmp(eventName, "BladeDamageOn") == 0) {
				if (attackNumber == 1) attackStep = true;
				if (!rightBladeCollider->IsActive()) rightBladeCollider->Enable();
				if (!leftBladeCollider->IsActive()) leftBladeCollider->Enable();
			}
			if (strcmp(eventName, "BladeDamageOff") == 0) {
				if (rightBladeCollider->IsActive()) rightBladeCollider->Disable();
				if (leftBladeCollider->IsActive()) leftBladeCollider->Disable();
			}
			if (strcmp(eventName, "StopStep") == 0) {
				attackStep = false;
				agent->SetMaxSpeed(gruntCharacter.movementSpeed);
			}
		}
		else if (attackNumber == 3) {
			if (strcmp(eventName, "RightBladeDamageOn") == 0) {
				track = false;
				if (!rightBladeCollider->IsActive()) rightBladeCollider->Enable();
				agent->SetMaxSpeed(gruntCharacter.movementSpeed + 3);
			}
			if (strcmp(eventName, "RightBladeDamageOff") == 0) {
				track = true;
				attackStep = false;
				if (rightBladeCollider->IsActive()) rightBladeCollider->Disable();
				agent->SetMaxSpeed(gruntCharacter.movementSpeed);
			}
			if (strcmp(eventName, "LeftBladeDamageOn") == 0) {
				track = false;
				attackStep = true;
				if (!leftBladeCollider->IsActive()) leftBladeCollider->Enable();
				agent->SetMaxSpeed(gruntCharacter.movementSpeed + 3);
			}
			if (strcmp(eventName, "LeftBladeDamageOff") == 0) {
				track = true;
				attackStep = false;
				if (leftBladeCollider->IsActive()) leftBladeCollider->Disable();
				agent->SetMaxSpeed(gruntCharacter.movementSpeed);
			}
		}
		break;
	default:
		break;
	}
}

void AIMeleeGrunt::Death()
{
	if (!GameController::IsGameplayBlocked()) {
		if (animation->GetCurrentState() && state != AIState::DEATH) {
			std::string changeState = animation->GetCurrentState()->name + "Death";
			deathType = 1 + rand() % 2;
			std::string deathTypeStr = std::to_string(deathType);
			animation->SendTrigger(changeState + deathTypeStr);

			if (audios[static_cast<int>(AudioType::DEATH)]) audios[static_cast<int>(AudioType::DEATH)]->Play();
			ComponentCapsuleCollider* collider = GetOwner().GetComponent<ComponentCapsuleCollider>();
			if (collider) collider->Disable();

			agent->RemoveAgentFromCrowd();
			if (gruntCharacter.beingPushed) gruntCharacter.beingPushed = false;
			state = AIState::DEATH;
		}
	}
}

void AIMeleeGrunt::PlayerHit() {
	alreadyHit = true;
}

void AIMeleeGrunt::PlayHitMaterialEffect()
{
	if (!dissolveAlreadyStarted) {
		SetMaterial(componentMeshRenderer, damageMaterialID);
		SetMaterial(componentMeshRendererLeftBlade, damageMaterialID);
		SetMaterial(componentMeshRendererRightBlade, damageMaterialID);
	}
}

void AIMeleeGrunt::UpdateDissolveTimer() {
	if (dissolveAlreadyStarted && !dissolveAlreadyPlayed) {
		if (currentDissolveTime >= dissolveTimerToStart) {
			if (dissolveMaterialID != 0) {
				SetMaterial(componentMeshRenderer, dissolveMaterialID, true);
				SetMaterial(componentMeshRendererLeftBlade, dissolveMaterialWeaponID, true);
				SetMaterial(componentMeshRendererRightBlade, dissolveMaterialWeaponID, true);
			}
			dissolveAlreadyPlayed = true;
		}
		else {
			currentDissolveTime += Time::GetDeltaTime();
		}
	}
}

void AIMeleeGrunt::SetRandomMaterial()
{
	if (!componentMeshRenderer) return;
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

			int position = distrib(gen)-1;
			componentMeshRenderer->ChangeMaterial(materials[position]);
			defaultMaterialID = materials[position];
		}
	}
}

void AIMeleeGrunt::SetMaterial(ComponentMeshRenderer* mesh, UID newMaterialID, bool needToPlayDissolve) {
	if (newMaterialID > 0 && mesh) {
		mesh->ChangeMaterial(newMaterialID);
		if (needToPlayDissolve) {
			mesh->PlayDissolveAnimation();
		}
	}
}