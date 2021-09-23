#pragma once

#include "Scripting/Script.h"
#include "UIAudioType.h"

class ComponentAudioSource;
class ComponentSelectable;

class GameObject;
class SceneTransition;

class ExitButton : public Script
{
	GENERATE_BODY(ExitButton);

public:

	void Start() override;
	void Update() override;
	void OnButtonClick() override;
	void PlayAudio(UIAudio type);

public:
	UID transitionUID = 0;

private:
	GameObject* transitionGO = nullptr;
	SceneTransition* sceneTransition = nullptr;

	/* Audio */
	bool playHoveredAudio = true;
	ComponentSelectable* selectable = nullptr;
	ComponentAudioSource* audios[static_cast<int>(UIAudio::TOTAL)] = { nullptr };
};
