#pragma once

#include "Scripting/Script.h"
#include <vector>

class GameObject;
class ComponentToggle;
class PlayerController;

class GodModeController : public Script {
	GENERATE_BODY(GodModeController);

public:
	/* UI toggles */
	UID uiCanvasUID = 0;
	UID spawnMeleeUID = 0;
	UID spawnRangedUID = 0;

	/* Enemy groups */
	UID enemiesUID = 0;
	UID debugEnemiesUID = 0;

	/* Cameras */
	UID gameCameraUID = 0;
	UID godCameraUID = 0;

	/* Player controller */
	UID playerControllerUID = 0;

	/* Level Doors */
	UID plazaDoorUID = 0;
	UID cafeteriaDoorUID = 0;
	UID bridgeDoorUID = 0;

	/* Dialog triggers */
	UID dialogTriggersUID = 0;

public:
	void Start() override;
	void Update() override;
	void OnChildToggle(unsigned int index, bool isChecked);

private:
	/* Owner */
	GameObject* gameObject = nullptr;

	/* UI toggles */
	GameObject* uiCanvas = nullptr;
	GameObject* spawnMelee = nullptr;
	GameObject* spawnRanged = nullptr;
	std::vector<ComponentToggle*> toggles;

	/* Enemy groups */
	GameObject* enemies = nullptr;
	GameObject* debugEnemies = nullptr;

	/* Cameras */
	GameObject* gameCamera = nullptr;
	GameObject* godCamera = nullptr;

	/* Player controller */
	GameObject* playerController = nullptr;
	PlayerController* playerControllerScript = nullptr;

	/* Level Doors */
	GameObject* plazaDoor = nullptr;
	GameObject* cafeteriaDoor = nullptr;
	GameObject* bridgeDoor = nullptr;

	/* Dialog triggers */
	GameObject* dialogTriggers = nullptr;

	/* Door array to keep previous state */
	std::vector <std::pair<GameObject*, bool>> doorPreviousStates;
};
