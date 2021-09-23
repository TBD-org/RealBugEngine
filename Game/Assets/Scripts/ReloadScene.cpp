#include "ReloadScene.h"
#include "GameplaySystems.h"
#include "PlayerController.h"
#include "PauseController.h"
#include "GameController.h"

EXPOSE_MEMBERS(ReloadScene) {
	MEMBER(MemberType::SCENE_RESOURCE_UID, sceneUID),
	MEMBER(MemberType::INT, levelNum),
	MEMBER(MemberType::GAME_OBJECT_UID, gameControllerUID)
};

GENERATE_BODY_IMPL(ReloadScene);

void ReloadScene::Start() {

}

void ReloadScene::Update() {

}

void ReloadScene::OnButtonClick() {
	if(sceneUID != 0) SceneManager::ChangeScene(sceneUID);
	PlayerController::currentLevel = levelNum;

	// If this is clicked from a Menu Button in which everything is frozen, this should resume it
	PauseController::SetIsPause(false);
	GameObject* gameController = GameplaySystems::GetGameObject(gameControllerUID);
	if (gameController) {
		GameController* gameControllerScript = GET_SCRIPT(gameController, GameController);
		if (gameControllerScript) {
			gameControllerScript->ResumeGame();
		}
	}
}