#include "Resume.h"

#include "GameObject.h"
#include "GameplaySystems.h"

EXPOSE_MEMBERS(Resume) {
	MEMBER(MemberType::GAME_OBJECT_UID, gameControllerUID)
};

GENERATE_BODY_IMPL(Resume);

void Resume::Start() {
	GameObject* gameControllerGameObject = GameplaySystems::GetGameObject(gameControllerUID);
	if (gameControllerGameObject) {
		gameController = GET_SCRIPT(gameControllerGameObject, GameController);
	}

	/* Audio */
	selectable = GetOwner().GetComponent<ComponentSelectable>();

	int i = 0;
	for (ComponentAudioSource& src : GetOwner().GetComponents<ComponentAudioSource>()) {
		if (i < static_cast<int>(UIAudio::TOTAL)) audios[i] = &src;
		++i;
	}
}

void Resume::Update() {
	/* Audio */
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
				}
				else {
					playHoveredAudio = true;
				}
			}
			else {
				playHoveredAudio = true;
			}
		}
	}
}

void Resume::OnButtonClick() {
	PlayAudio(UIAudio::CLICKED);
	if (gameController) {
		gameController->ResumeGame();
	}
}

void Resume::PlayAudio(UIAudio type) {
	if (audios[static_cast<int>(type)]) audios[static_cast<int>(type)]->Play();
}
