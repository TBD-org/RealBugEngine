#pragma once

#include "Scripting/Script.h"
#include "UIAudioType.h"

class ComponentAudioSource;
class ComponentSelectable;

extern bool screenResolutionChangeConfirmationWasRequested;
extern unsigned preSelectedScreenResolutionPreset;

class ScreenResolutionConfirmer : public Script {
	GENERATE_BODY(ScreenResolutionConfirmer);

public:

	void Start() override;
	void Update() override;
	void OnButtonClick()override;
	void PlayAudio(UIAudio type);

private:
	bool playHoveredAudio = true;
	ComponentSelectable* selectable = nullptr;
	ComponentAudioSource* audios[static_cast<int>(UIAudio::TOTAL)] = { nullptr };
};

