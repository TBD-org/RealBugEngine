#pragma once

#include "Scripting/Script.h"
#include "UIAudioType.h"

class GameObject;
class ComponentAudioSource;
class ComponentSelectable;

class ReturnMenu : public Script
{
	GENERATE_BODY(ReturnMenu);

public:

	void Start() override;
	void Update() override;
	void OnButtonClick() override;
	void PlayAudio(UIAudio type);

public:
	UID buttonUID = 0;
	UID sceneUID = 0;
	float padding = 20.f;

private:
	bool playHoveredAudio = true;
	ComponentSelectable* selectable = nullptr;
	ComponentAudioSource* audios[static_cast<int>(UIAudio::TOTAL)] = { nullptr };
};

