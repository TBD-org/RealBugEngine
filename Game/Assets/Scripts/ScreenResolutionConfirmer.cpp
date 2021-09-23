#include "ScreenResolutionConfirmer.h"
#include "GameplaySystems.h"

EXPOSE_MEMBERS(ScreenResolutionConfirmer) {

};

GENERATE_BODY_IMPL(ScreenResolutionConfirmer);

void ScreenResolutionConfirmer::Start() {

    /* Audio */
    selectable = GetOwner().GetComponent<ComponentSelectable>();
    int i = 0;
    for (ComponentAudioSource& src : GetOwner().GetComponents<ComponentAudioSource>()) {
        if (i < static_cast<int>(UIAudio::TOTAL)) audios[i] = &src;
        ++i;
    }
}

void ScreenResolutionConfirmer::Update() {
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

void ScreenResolutionConfirmer::OnButtonClick() {
    PlayAudio(UIAudio::CLICKED);
	screenResolutionChangeConfirmationWasRequested = true;
}

void ScreenResolutionConfirmer::PlayAudio(UIAudio type) {
    if (audios[static_cast<int>(type)]) audios[static_cast<int>(type)]->Play();
}
