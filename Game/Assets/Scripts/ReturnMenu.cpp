#include "ReturnMenu.h"

#include "GameObject.h"
#include "GameplaySystems.h"

EXPOSE_MEMBERS(ReturnMenu) {
	MEMBER(MemberType::SCENE_RESOURCE_UID, sceneUID),
	MEMBER(MemberType::FLOAT, padding)
};

GENERATE_BODY_IMPL(ReturnMenu);

void ReturnMenu::Start() {
    /* Audio */
    selectable = GetOwner().GetComponent<ComponentSelectable>();

    int i = 0;
    for (ComponentAudioSource& src : GetOwner().GetComponents<ComponentAudioSource>()) {
        if (i < static_cast<int>(UIAudio::TOTAL)) audios[i] = &src;
        ++i;
    }
}

void ReturnMenu::Update() {
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

void ReturnMenu::OnButtonClick() {
    PlayAudio(UIAudio::CLICKED);
	if(sceneUID != 0)SceneManager::ChangeScene(sceneUID);
	if (Time::GetDeltaTime() == 0.f) Time::ResumeGame();
}

void ReturnMenu::PlayAudio(UIAudio type) {
    if (audios[static_cast<int>(type)]) audios[static_cast<int>(type)]->Play();
}
