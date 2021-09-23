#include "SwapPanels.h"

#include "GameplaySystems.h"

EXPOSE_MEMBERS(SwapPanels) {
	MEMBER(MemberType::GAME_OBJECT_UID, targetUID),
	MEMBER(MemberType::GAME_OBJECT_UID, currentUID)
};

GENERATE_BODY_IMPL(SwapPanels);

void SwapPanels::Start() {

	target = GameplaySystems::GetGameObject(targetUID);
	current = GameplaySystems::GetGameObject(currentUID);

	/* Audio */
	selectable = GetOwner().GetComponent<ComponentSelectable>();

	int i = 0;
	for (ComponentAudioSource& src : GetOwner().GetComponents<ComponentAudioSource>()) {
		if (i < static_cast<int>(UIAudio::TOTAL)) audios[i] = &src;
		++i;
	}
}

void SwapPanels::Update() {
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
				} else {
					playHoveredAudio = true;
				}
			} else {
				playHoveredAudio = true;
			}
		}
	}
}

void SwapPanels::OnButtonClick() {
	PlayAudio(UIAudio::CLICKED);
	DoSwapPanels();
}

void SwapPanels::PlayAudio(UIAudio type) {
	if (audios[static_cast<int>(type)]) audios[static_cast<int>(type)]->Play();
}

void SwapPanels::DoSwapPanels() {
	if (target != nullptr && current != nullptr) {
		target->Enable();
		current->Disable();
	}
}
