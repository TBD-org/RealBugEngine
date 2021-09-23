#include "GodModeController.h"
#include "SpawnOnClick.h"
#include "PlayerController.h"

#include "GameplaySystems.h"
#include "GameObject.h"

#include "Components/UI/ComponentToggle.h"

EXPOSE_MEMBERS(GodModeController) {
	/* UI toggles*/
	MEMBER(MemberType::GAME_OBJECT_UID, uiCanvasUID),
	MEMBER(MemberType::GAME_OBJECT_UID, spawnMeleeUID),
	MEMBER(MemberType::GAME_OBJECT_UID, spawnRangedUID),
	/* Enemy groups*/
	MEMBER(MemberType::GAME_OBJECT_UID, enemiesUID),
	MEMBER(MemberType::GAME_OBJECT_UID, debugEnemiesUID),
	/* Cameras */
	MEMBER(MemberType::GAME_OBJECT_UID, godCameraUID),
	MEMBER(MemberType::GAME_OBJECT_UID, gameCameraUID),
	/* Player controller */
	MEMBER(MemberType::GAME_OBJECT_UID, playerControllerUID),
	/* Level Doors */
	MEMBER(MemberType::GAME_OBJECT_UID, plazaDoorUID),
	MEMBER(MemberType::GAME_OBJECT_UID, cafeteriaDoorUID),
	MEMBER(MemberType::GAME_OBJECT_UID, bridgeDoorUID),
	/* Dialog triggers */
	MEMBER(MemberType::GAME_OBJECT_UID, dialogTriggersUID)
};

GENERATE_BODY_IMPL(GodModeController);

void GodModeController::Start() {
	/* Owner */
	gameObject = &GetOwner();
	/* UI toggles*/
	uiCanvas = GameplaySystems::GetGameObject(uiCanvasUID);
	spawnMelee = GameplaySystems::GetGameObject(spawnMeleeUID);
	spawnRanged = GameplaySystems::GetGameObject(spawnRangedUID);
	/* Enemy groups*/
	enemies = GameplaySystems::GetGameObject(enemiesUID);
	debugEnemies = GameplaySystems::GetGameObject(debugEnemiesUID);
	/* Cameras */
	godCamera = GameplaySystems::GetGameObject(godCameraUID);
	gameCamera = GameplaySystems::GetGameObject(gameCameraUID);
	/* Player controller */
	playerController = GameplaySystems::GetGameObject(playerControllerUID);
	if (playerController) playerControllerScript = GET_SCRIPT(playerController, PlayerController);
	/* Level doors */
	plazaDoor = GameplaySystems::GetGameObject(plazaDoorUID);
	cafeteriaDoor = GameplaySystems::GetGameObject(cafeteriaDoorUID);
	bridgeDoor = GameplaySystems::GetGameObject(bridgeDoorUID);

	/* Dialog triggers */
	dialogTriggers = GameplaySystems::GetGameObject(dialogTriggersUID);

	doorPreviousStates.emplace_back(plazaDoor, plazaDoor ? plazaDoor->IsActive() : false);
	doorPreviousStates.emplace_back(cafeteriaDoor, cafeteriaDoor ? cafeteriaDoor->IsActive() : false);
	doorPreviousStates.emplace_back(bridgeDoor, bridgeDoor ? bridgeDoor->IsActive() : false);

	for (GameObject* child : uiCanvas->GetChildren()) {
		if (child->HasComponent<ComponentToggle>()) {
			ComponentToggle* toggle = child->GetComponent<ComponentToggle>();
			if (toggle) toggles.push_back(toggle);
		}
	}
}

void GodModeController::Update() {}

void GodModeController::OnChildToggle(unsigned int index, bool isChecked) {
	switch (index) {
	case 0:
		if (isChecked) {
			if (toggles[index + 1]->IsChecked()) {
				toggles[index + 1]->SetChecked(false);
			}
			if (spawnMelee) spawnMelee->Enable();
		}
		else {
			if (spawnMelee) spawnMelee->Disable();
		}
		break;
	case 1:
		if (isChecked) {
			if (toggles[index - 1]->IsChecked()) {
				toggles[index - 1]->SetChecked(false);
			}
			if (spawnRanged) spawnRanged->Enable();
		}
		else {
			if (spawnRanged) spawnRanged->Disable();
		}
		break;
	case 2:
		if (isChecked) {
			if (enemies) enemies->Enable();
			if (debugEnemies) debugEnemies->Enable();
			for (std::vector <std::pair<GameObject*, bool>>::iterator it = doorPreviousStates.begin(), end = doorPreviousStates.end(); it != end; ++it) {
				if (it->first && it->second) it->first->Enable();
			}
		}
		else {
			if (enemies) enemies->Disable();
			if (debugEnemies) debugEnemies->Disable();
			for (std::vector <std::pair<GameObject*, bool>>::iterator it = doorPreviousStates.begin(), end = doorPreviousStates.end(); it != end; ++it) {
				if (it->first && it->second) it->first->Disable();
			}
		}
		break;
	case 3:
		if (isChecked) {
			if (dialogTriggers) dialogTriggers->Enable();
		}
		else {
			if (dialogTriggers) dialogTriggers->Disable();
		}
		break;
	case 4:
		if (isChecked) {
			if (toggles[index + 1]->IsChecked()) {
				toggles[index + 1]->SetChecked(false);
			}
			if (gameCamera) {
				GameplaySystems::SetRenderCamera(gameCamera->GetComponent<ComponentCamera>());
			}
		}
		else if (!toggles[index + 1]->IsChecked()) {
			toggles[index]->SetChecked(true);
		}
		break;
	case 5:
		if (isChecked) {
			if (toggles[index - 1]->IsChecked()) {
				toggles[index - 1]->SetChecked(false);
			}

			if (godCamera) {
				GameplaySystems::SetRenderCamera(godCamera->GetComponent<ComponentCamera>());
			}
		}
		else if (!toggles[index - 1]->IsChecked()) {
			toggles[index]->SetChecked(true);
		}
		break;
	case 6:
		if (isChecked) {
			if (playerControllerScript) playerControllerScript->SetOverpower(true);
		}
		else {
			if (playerControllerScript) playerControllerScript->SetOverpower(false);
		}
		break;
	case 7:
		if (isChecked) {
			if (playerControllerScript) playerControllerScript->SetNoCooldown(true);
		}
		else {
			if (playerControllerScript) playerControllerScript->SetNoCooldown(false);
		}
		break;
	case 8:
		if (isChecked) {
			if (playerControllerScript) playerControllerScript->SetInvincible(true);
		}
		else {
			if (playerControllerScript) playerControllerScript->SetInvincible(false);
		}
		break;

	default:
		break;
	}
}