#include "StartController.h"

#include "StartButton.h"
#include "GameplaySystems.h"

#include "GameObject.h"
#include "Components/ComponentCamera.h"

EXPOSE_MEMBERS(StartController) {
	// Add members here to expose them to the engine. Example:
	MEMBER(MemberType::GAME_OBJECT_UID, gameCameraUID)
};

GENERATE_BODY_IMPL(StartController);

void StartController::Start() {
	gameCamera = GameplaySystems::GetGameObject(gameCameraUID);
	if (gameCamera) {
		ComponentCamera* camera = gameCamera->GetComponent<ComponentCamera>();
		if(camera) GameplaySystems::SetRenderCamera(camera);
	}
}

void StartController::Update() {
}