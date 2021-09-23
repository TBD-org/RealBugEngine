#pragma once

#include "Scripting/Script.h"

#include <vector>

class GameObject;
class ResourcePrefab;
class ComponentLight;

class SpawnPointController : public Script {
	GENERATE_BODY(SpawnPointController);

public:
	/* Enemy prefab UIDs */
	UID meleeEnemyPrefabUID = 0;
	UID rangeEnemyPrefabUID = 0;

	/* Door UID */
	UID initialDoorUID = 0;
	UID finalDoorUID = 0;
	UID gameObjectActivatedOnCombatEndUID = 0;
	UID gameObjectDeactivatedOnCombatEndUID = 0;

	std::string doorEnergyBack = "DoorEnergyBack";
	std::string doorEnergyFront = "DoorEnergyFront";

	bool unlocksInitialDoor = true;				// Set to true if this must unlock first door
	bool isInitiallyLocked = false;				// This should be set to true if it's the first door of Level1, that is initially blocked and MUST NOT unlock/lock
	bool isLastDoor = false;					// Set to true if this is the last door of the Spawn Points
	float timerToUnlock = 0.0f;					// The timer to unlock/lock the door

	/* Dissolve UID */
	UID dissolveMaterialGOUID = 0;

public:
	void Start() override;
	void Update() override;

	/* Enable the spawn points on trigger  */
	void OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle = nullptr) override;
	void OpenDoor();

	ResourcePrefab* GetMeleePrefab() { return meleeEnemyPrefab; };
	ResourcePrefab* GetRangePrefab() { return rangeEnemyPrefab; };

	/* Spawn point status for wave management */
	void SetCurrentEnemyAmount(unsigned int pos, unsigned int amount);
	void SetEnemySpawnStatus(unsigned int pos, bool status);
	bool CanSpawn();

private:
	/* Owner */
	GameObject* gameObject = nullptr;

	/* Enemy prefabs */
	ResourcePrefab* meleeEnemyPrefab = nullptr;
	ResourcePrefab* rangeEnemyPrefab = nullptr;

	/* Door object */
	GameObject* initialDoor = nullptr;
	GameObject* finalDoor = nullptr;

	GameObject* gameObjectActivatedOnCombatEnd = nullptr;	// This gameObject will be disabled when triggering the combat, and will be enabled again when the combat ends (in 'OpenDoor()'). Useful to set up triggers and other gameplay features after a combat encounter.
	GameObject* gameObjectDeactivatedOnCombatEnd = nullptr;	// This gameObject will be enabled when triggering the combat, and will be disabled again when the combat ends (in 'OpenDoor()'). Useful to set down triggers and other gameplay features after a combat encounter.

	/* Spawn points satus*/
	std::vector<unsigned int> enemiesPerSpawnPoint;
	std::vector<bool> enemySpawnPointStatus;

	/* Dissolve UID */
	UID dissolveMaterialID = 0;
	float currentUnlockTime = 0.0f;
	bool unlockStarted = false;				// This tells whether it needs to start the unlock/lock animation 
	bool mustKeepOpen = false;

	/* Door Light */
	std::string doorLightGameObjectName = "LaserDoor";		// This is the name of the GameObject that contains the Light

	ComponentLight* initialDoorLight = nullptr;				// Component Light obtained from LaserDoor of the Initial Door
	float initialDoorLightStartIntensity = 0.0f;			// Initial intensity of the initialDoorLight

	ComponentLight* finalDoorLight = nullptr;				// Component Light obtained from LaserDoor of the final Door
	float finalDoorLightStartIntensity = 0.0f;				// Initial intensity of the finalDoorLight

	bool isClosing = false;

private:
	bool CheckSpawnPointStatus();
	void PlayDissolveAnimation(GameObject* root, bool playReverse);				// Searches on root the GameObject called "DoorEnergyBack" and "DoorEnergyFront" and calls PlayDissolveAnimation on their materials. PlayReverse will play the reverse animation.
	void SetLightIntensity(ComponentLight* light, float newIntensity);			// Sets newIntensity to the light given
	void ResetUnlockAnimation();												// Sets currentUnlockTime to 0
};
