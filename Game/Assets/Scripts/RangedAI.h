#pragma once

#include "Scripting/Script.h"
#include "Enemy.h"
#include "AIState.h"
#include "Player.h"
#include <string>

class ComponentAgent;
class ComponentAudioSource;
class ComponentAnimation;
class ComponentMeshRenderer;
class ComponentParticleSystem;
class ComponentTransform;
class ResourcePrefab;
class HUDController;
class PlayerController;
class PlayerDeath;
class AIMovement;
class EnemySpawnPoint;

class RangedAI : public Script {
	GENERATE_BODY(RangedAI);

public:
	enum class AudioType {
		SPAWN,
		SHOOT,
		FOOTSTEP_RIGHT,
		FOOTSTEP_LEFT,
		HIT,
		DEATH,
		TOTAL
	};

	void Start() override;
	void Update() override;
	void OnAnimationFinished() override;
	void OnAnimationSecondaryFinished() override;
	void OnAnimationEvent(StateMachineEnum stateMachineEnum, const char* eventName) override;
	void OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle = nullptr) override; //This is commented until merge with collisions
	void ShootPlayerInRange(); //Sets in motion the shooting at the player, if found and close enough

	void EnableBlastPushBack();
	void DisableBlastPushBack();
	bool IsBeingPushed() const;
	void PlayHit();

private:
	//State machine handling
	void EnterState(AIState newState);
	void UpdateState();
	void ChangeState(AIState newState);

	bool CharacterInRange(const GameObject* character, float range, bool useRange); //Returns true if distance to player is < than radius
	bool FindsRayToPlayer(bool useForward);											//Returns true if raycasting towards player finds a hit target, later on this will also check if the hit is actually player and not a wall
	void OrientateTo(const float3& direction);										//Makes character look in a particular direcion
	void ActualShot();																//Generates projectile prefab instance
	void PlayAudio(AudioType audioType);											//Plays audio (if not null)

	void UpdatePushBackPosition();
	void CalculatePushBackRealDistance();											// Calculates the real distance of the pushback taking into account any obstacles in the path
	void PlayHitMaterialEffect();													// Changes material hit
	void UpdateDissolveTimer();														// If the currentDissolveTime is reached, Plays animation
	void ParticleHit(GameObject& collidedWith, void* particle, Player& player);
	void SetRandomMaterial();
	void SetMaterial(ComponentMeshRenderer* mesh, UID newMaterialID, bool needToPlayDissolve = false);


public:
	Enemy rangerGruntCharacter = Enemy(5.0f, 8.0f, 1.0f, 30, 40.f, 5.f, 5.f, 5.f, 5.f, 3.f, 2.f); //Enemy class instance (for shared values)
	UID playerUID = 0;				//Reference to player main Gameobject UID, used to check distances
	UID playerMeshUIDFang = 0;		//Reference to player Fang mesh holding Gameobject UID, used for raycasting if fang is active
	UID playerMeshUIDOnimaru = 0;	//Reference to player Fang mesh holding Gameobject UID, used for raycasting if onimaru is active
	UID meshUID1 = 0;				//Second mesh UID for checking frustum presence (if not inside frustum shooting won't happen)
	UID meshUID2 = 0;				//Third mesh UID for checking frustum presence (if not inside frustum shooting won't happen)
	UID fangUID = 0;

	ComponentParticleSystem* shootTrailPrefab = nullptr; //Reference to projectile prefab , for shooting
	GameObject* player = nullptr;				//Reference to player main Gameobject, used to check distances
	GameObject* fang = nullptr;
	GameObject* weapon = nullptr;

	PlayerController* playerController = nullptr; //Reference to player script, used to check distances

	PlayerDeath* playerDeath = nullptr;

	GameObject* fangMeshObj = nullptr;		//Reference to player Fang mesh holding Gameobject, used for raycasting if fang is active
	GameObject* onimaruMeshObj = nullptr;	//Reference to player Fang mesh holding Gameobject, used for raycasting if onimaru is active

	GameObject* meshObj = nullptr;							//Main mesh for Getting MeshRenderer reference and checking frustum presence (if not inside frustum shooting won't happen)
	GameObject* meshObjForFrustumPresenceCheck1 = nullptr;	//Second mesh for checking frustum presence (if not inside frustum shooting won't happen), this being null will make it always return true
	GameObject* meshObjForFrustumPresenceCheck2 = nullptr;	//Third mesh for checking frustum presence (if not inside frustum shooting won't happen), this being null will make it always return true

	UID dmgMaterialObj = 0;		//Reference to damaged material holding gameobject UID, used to be set whenever Ai has been recently hurt
	UID defaultMaterialID = 0;	//Reference to not damaged material UID, used to be set whenever Ai has been recently hurt but it is hurt no more
	UID backpackMaterialID = 0;	//Reference to not damaged material UID, used to be set whenever Ai has been recently hurt but it is hurt no more
	UID weaponMaterialID = 0;	//Reference to not damaged material UID, used to be set whenever Ai has been recently hurt but it is hurt no more
	UID damageMaterialID = 0;	//Reference to damaged material, used to be set whenever Ai has been recently hurt

	ComponentAgent* agent = nullptr;	//Reference to Agent component, for navigation

	float approachOffset = 4.0f;		//Offset to prevent AI from chasing after player immediately after getting close enough whenever player moves slightly
	float fleeingRange = 7.f;			//Distance at which entity will start a flee motion
	float attackSpeed = 0.5f;			//Shots per second
	float actualShotMaxTime = 0.3f;		//Internal variable used to match the shooting animation and the projectile creation
	float timeSinceLastHurt = 0.5f;		//Timer to keep track of how long it's been since AI was hurt, if higher than hurtFeedbackTimeDuration, this tries to make AI turn red with DamagedMaterial
	float stunDuration = 3.f;			//Max time the enemy will be stunned
	float hurtFeedbackTimeDuration = 0.5f;	//Time that damaged material will be shown whenever AI is hit
	float groundPosition = 3.0f;
	float fleeingUpdateTime = 3.0f;        //Time that needs to wait in order to get away from the player in the flee state

	UID dissolveMaterialObj = 0;		//Reference to dissolve material holding gameobject UID, used to be set whenever Ai has been recently hurt
	UID dissolveMaterialID = 0;			//Reference to dissolve material, used to be set whenever Ai has been recently hurt
	float dissolveTimerToStart = 0.0f;	//Timer until the dissolve animation is played
	UID materialsUID = 0;				//Reference to materials placeholder for random

private:

	EnemySpawnPoint* enemySpawnPointScript = nullptr;

	AIMovement* aiMovement = nullptr;	//Reference to movement holding script
	AIState state = AIState::START;		//AI State
	float3 bbCenter = float3(0, 0, 0);	//Bounding box center, to generate an offset for raycasting

	bool shot = false;					//Bool used to make sure shooting event happens only once whenever attackTimePool is low enough

	float stunTimeRemaining = 0.f;			//Time remaining stunned
	int deathType = 0;

	ComponentAnimation* animation = nullptr;		//Refernece to  animatino component
	ComponentTransform* ownerTransform = nullptr;	//Reference to owner transform componenet

	float attackTimePool = 2.0f;			//Pool that counts down to 0 to make AI shoot a projectile
	float actualShotTimer = -1.0f;			//Timer that counts down the seconds to match shooting animation with projectile creation

	ComponentMeshRenderer* meshRenderer = nullptr;	//Reference to a meshRendererComponent, used for material setting on hurt
	ComponentMeshRenderer* backpackMeshRenderer = nullptr;
	ComponentMeshRenderer* weaponMeshRenderer = nullptr;

	ComponentAudioSource* audios[static_cast<int>(AudioType::TOTAL)] = { nullptr }; //Array of ints used to play audios

	float currentPushBackDistance = 0.f;
	float currentSlowedDownTime = 0.f;
	float pushBackRealDistance = 0.f;

	float currentFleeingUpdateTime = 0.f; // Current Time that needs to compare against the fleeingUpdateTime in the flee state
	float3 currentFleeDestination;        // Destination position where it is going to move far away from the player
	bool fleeingFarAway = false;          //Toggle to get away from the player

	float currentDissolveTime = 0.0f;
	bool dissolveAlreadyStarted = false;	//Used to control other material setters so it doesn't interfere with Dissolve's material
	bool dissolveAlreadyPlayed = false;		//Controls whether the animation function has already been played (called material->PlayAnimation) or not
};
