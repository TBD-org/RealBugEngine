#include "StartTitleGlitchOnPlay.h"
#include "GlitchyTitleController.h"

#include "GameplaySystems.h"
#include "CheckpointManager.h"
#include "SceneTransition.h"
#include "PlayerController.h"
#include "SwapPanels.h"

#include "GameplaySystems.h"
#include "GameObject.h"
#include "CanvasFader.h"


EXPOSE_MEMBERS(StartTitleGlitchOnPlay) {
	MEMBER(MemberType::SCENE_RESOURCE_UID, sceneUID),
	MEMBER(MemberType::GAME_OBJECT_UID, fadeToBlackObjectUID),
	MEMBER(MemberType::GAME_OBJECT_UID, controllerObjUID),
	MEMBER(MemberType::GAME_OBJECT_UID, swapPanelsObjUID),
	MEMBER(MemberType::INT, levelNum),
	MEMBER(MemberType::INT, checkpointNum),
};

GENERATE_BODY_IMPL(StartTitleGlitchOnPlay);

void StartTitleGlitchOnPlay::Start() {
	GameObject* controllerObj = GameplaySystems::GetGameObject(controllerObjUID);
	if (controllerObj) {
		controller = GET_SCRIPT(controllerObj, GlitchyTitleController);
	}

	/* Audio */
	selectable = GetOwner().GetComponent<ComponentSelectable>();

	int i = 0;
	for (ComponentAudioSource& src : GetOwner().GetComponents<ComponentAudioSource>()) {
		if (i < static_cast<int>(UIAudio::TOTAL)) audios[i] = &src;
		++i;
	}

	GameObject* fadeToBlackObject = GameplaySystems::GetGameObject(fadeToBlackObjectUID);

	if (fadeToBlackObject) {
		canvasFader = GET_SCRIPT(fadeToBlackObject, CanvasFader);
	}

	GameObject* swapPanelsObj = GameplaySystems::GetGameObject(swapPanelsObjUID);
	if (swapPanelsObj) {
		swapPanelsScript = GET_SCRIPT(swapPanelsObj, SwapPanels);
	}


}

void StartTitleGlitchOnPlay::Update() {
	if (!pressed) {
		if (selectable) {
			ComponentEventSystem* eventSystem = UserInterface::GetCurrentEventSystem();
			if (eventSystem) {
				ComponentSelectable* hoveredComponent = eventSystem->GetCurrentlyHovered();
				if (hoveredComponent) {
					bool hovered = selectable->GetID() == hoveredComponent->GetID() ? true : false;
					if (hovered) {
						if (playHoveredAudio) {
							PlayAudio(UIAudio::HOVERED);
							playHoveredAudio = false;
						}
					} else {
						playHoveredAudio = true;
					}
				} else {
					playHoveredAudio = true;
				}
			}
		}
	}
}

void StartTitleGlitchOnPlay::OnButtonClick() {

	PlayAudio(UIAudio::CLICKED);

	if (swapPanelsScript) {
		swapPanelsScript->DoSwapPanels();
	}

	if (controller) {
		controller->PressedPlay(this);
	}

	if (!canvasFader) {
		DoTransition();
	} else {
		pressed = true;
	}



}

void StartTitleGlitchOnPlay::DoTransition() {
	checkpoint = checkpointNum;
	if (sceneUID != 0) SceneManager::ChangeScene(sceneUID);
	if (levelNum == 2) {
		PlayerController::currentLevel = 2;
		Player::level2Upgrade = false;
	} else if (levelNum == 1) {
		PlayerController::currentLevel = 1;
		Player::level1Upgrade = false;
		Player::level2Upgrade = false;
	}
	if (Time::GetDeltaTime() == 0.f) Time::ResumeGame();

}


void StartTitleGlitchOnPlay::PlayAudio(UIAudio type) {
	if (audios[static_cast<int>(type)]) audios[static_cast<int>(type)]->Play();
}