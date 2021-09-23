#pragma once

#include "Scripting/Script.h"

#include "AIState.h"
#include "Enemy.h"
#include "Player.h"

class GameObject;
class ComponentAnimation;
class ComponentTransform;
class ComponentAgent;
class ComponentAudioSource;
class ComponentMeshRenderer;
class ResourcePrefab;
class HUDController;
class PlayerController;
class PlayerDeath;
class AIMovement;
class EnemySpawnPoint;

class AIMeleeGrunt : public Script {
	GENERATE_BODY(AIMeleeGrunt);

public:
	enum class AudioType {
		SPAWN,
		ATTACK,
		FOOTSTEP_RIGHT,
		FOOTSTEP_LEFT,
		HIT,
		DEATH,
		TOTAL
	};

	void Start() override;
	void Update() override;
	void OnAnimationFinished() override;
	void OnAnimationEvent(StateMachineEnum stateMachineEnum, const char* eventName) override;
	void OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle = nullptr) override;

	void EnableBlastPushBack();
	void DisableBlastPushBack();
	bool IsBeingPushed() const;
	void PlayerHit();
	void PlayHit();

public:

	UID playerUID = 0;
	UID meleePunchUID = 0;
	UID fangUID = 0;
	UID materialsUID = 0;
	// Hit feedback
	UID defaultMaterialPlaceHolderUID = 0;
	UID damageMaterialPlaceHolderUID = 0;
	UID defaultMaterialID = 0;
	UID bladesMaterialID = 0;
	UID damageMaterialID = 0;

	UID rightBladeColliderUID = 0;
	UID leftBladeColliderUID = 0;


	GameObject* player = nullptr;
	GameObject* fang = nullptr;
	GameObject* spawn = nullptr;
	ComponentAgent* agent = nullptr;

	Enemy gruntCharacter = Enemy(5.0f, 8.0f, 1.0f, 30, 40.f, 5.f, 5.f, 5.f, 5.f, 3.f, 2.f);
	bool killSent = false;

	float hurtFeedbackTimeDuration = 0.5f;

	float stunDuration = 3.f;

	float groundPosition = 3.0f;

	UID dissolveMaterialObj = 0;			//Reference to dissolve material holding gameobject UID, used to be set whenever Ai has been recently hurt
	UID dissolveMaterialID = 0;				//Reference to dissolve material, used to be set whenever Ai has been recently hurt
	UID dissolveMaterialWeaponObj = 0;		//Reference to weapon dissolve material holding gameobject UID, used to be set whenever Ai has been recently hurt
	UID dissolveMaterialWeaponID = 0;		//Reference to weapon dissolve material, used to be set whenever Ai has been recently hurt
	float dissolveTimerToStart = 0.0f;		//Timer until the dissolve animation is played

	//Attack1
	float att1AttackSpeed = 1.f;
	float att1MovementSpeedWhileAttacking = 1.f;
	int att1AbilityChance = 33;

	//Attack2
	float att2AttackSpeed = 1.f;
	float att2MovementSpeedWhileAttacking = 1.f;
	int att2AbilityChance = 33;

	//Attack3
	float att3AttackSpeed = 1.f;
	float att3MovementSpeedWhileAttacking = 1.f;
	int att3AbilityChance = 33;

private:
	bool track = true;
	bool attackStep = false;
	bool alreadyHit = false;
	int attackNumber = 3;
	float attackSpeed = 0.f;
	float attackMovementSpeed = 0.f;
	float3 velocity = float3(0, 0, 0);
	AIState state = AIState::START;
	bool hitTaken = false;
	ComponentAnimation* animation = nullptr;
	ComponentTransform* ownerTransform = nullptr;
	int damageRecieved = 0;

	float stunTimeRemaining = 0.f;

	//bool EMPUpgraded = false;
	int deathType = 0;

	PlayerController* playerController = nullptr;
	PlayerDeath* playerDeath = nullptr;
	AIMovement* movementScript = nullptr;
	EnemySpawnPoint* enemySpawnPointScript = nullptr;

	ComponentAudioSource* audios[static_cast<int>(AudioType::TOTAL)] = { nullptr };
	ComponentMeshRenderer* componentMeshRenderer = nullptr;
	ComponentMeshRenderer* componentMeshRendererLeftBlade = nullptr;
	ComponentMeshRenderer* componentMeshRendererRightBlade = nullptr;

	float timeSinceLastHurt = 0.5f;
	GameObject* rightBladeCollider = nullptr;
	GameObject* leftBladeCollider = nullptr;

	float currentPushBackDistance = 0.f;
	float currentSlowedDownTime = 0.f;

	float pushBackRealDistance = 0.f;

	float currentDissolveTime = 0.0f;
	bool dissolveAlreadyStarted = false;	//Used to control other material setters so it doesn't interfere with Dissolve's material
	bool dissolveAlreadyPlayed = false;		//Controls whether the animation function has already been played (called material->PlayAnimation) or not

private:
	void UpdatePushBackPosition();
	void CalculatePushBackRealDistance();	// Calculates the real distance of the pushback taking into account any obstacles in the path
	void Death();
	void ParticleHit(GameObject& collidedWith, void* particle, Player& player);
	void PlayHitMaterialEffect();
	void UpdateDissolveTimer();
	void SetRandomMaterial();
	void SetMaterial(ComponentMeshRenderer* mesh, UID newMaterialID, bool needToPlayDissolve = false);
};
