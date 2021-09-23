#include "SetUpgradesButtonColor.h"

#include "GameObject.h"
#include "Components/UI/ComponentImage.h"
#include "GameplaySystems.h"

EXPOSE_MEMBERS(SetUpgradesButtonColor) {
    // Add members here to expose them to the engine. Example:
    MEMBER(MemberType::GAME_OBJECT_UID, onUID),
    MEMBER(MemberType::GAME_OBJECT_UID, offUID)
};

GENERATE_BODY_IMPL(SetUpgradesButtonColor);

void SetUpgradesButtonColor::Start() {
    buttonOn = GameplaySystems::GetGameObject(onUID);
    buttonOff = GameplaySystems::GetGameObject(offUID);

    if (!buttonOn || !buttonOff) return;
    imageOn = buttonOn->GetComponent<ComponentImage>();
    imageOff = buttonOff->GetComponent<ComponentImage>();

    /* Audio */
    selectable = GetOwner().GetComponent<ComponentSelectable>();

    int i = 0;
    for (ComponentAudioSource& src : GetOwner().GetComponents<ComponentAudioSource>()) {
        if (i < static_cast<int>(UIAudio::TOTAL)) audios[i] = &src;
        ++i;
    }
}

void SetUpgradesButtonColor::Update() {
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

void SetUpgradesButtonColor::OnButtonClick() {
    if (!buttonOn || !buttonOff) return;
    if (!imageOn || !imageOff) return;

    PlayAudio(UIAudio::CLICKED);

    if (GetOwner().GetID() == buttonOn->GetID()) {
        imageOn->SetColor(selected);
        imageOff->SetColor(notSelected);
    }
    else {
        imageOff->SetColor(selected);
        imageOn->SetColor(notSelected);
    }
}

void SetUpgradesButtonColor::PlayAudio(UIAudio type) {
    if (audios[static_cast<int>(type)]) audios[static_cast<int>(type)]->Play();
}
