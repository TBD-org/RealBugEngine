#pragma once

#include "Scripting/Script.h"
#include "UIAudioType.h"

#include "GameObject.h"

class ComponentAudioSource;
class ComponentSelectable;

class SwapPanels : public Script {
	GENERATE_BODY(SwapPanels);

public:

	void Start() override;
	void Update() override;
	void OnButtonClick() override;
	void PlayAudio(UIAudio type);
	void DoSwapPanels();
public:
	UID targetUID;
	UID currentUID;
	GameObject* target;
	GameObject* current;

private:
	bool playHoveredAudio = true;
	ComponentSelectable* selectable = nullptr;
	ComponentAudioSource* audios[static_cast<int>(UIAudio::TOTAL)] = { nullptr };
};

