#pragma once

#include "Scripting/Script.h"
#include "UIAudioType.h"

class GameObject;
class ComponentImage;
class ComponentAudioSource;
class ComponentSelectable;

class SetUpgradesButtonColor : public Script
{
	GENERATE_BODY(SetUpgradesButtonColor);

public:

	void Start() override;
	void Update() override;

	void OnButtonClick() override;

	void PlayAudio(UIAudio type);

public:
	UID onUID = 0;
	UID offUID = 0;

private:
	GameObject* buttonOn = nullptr;
	GameObject* buttonOff = nullptr;

	ComponentImage* imageOn = nullptr;
	ComponentImage* imageOff = nullptr;

	float4 selected = float4(0.92f, 0.23f, 0.54f, 1.f);
	float4 notSelected = float4(0.f, 0.34f, 0.53f, 0.9f);

	bool playHoveredAudio = true;
	ComponentSelectable* selectable = nullptr;
	ComponentAudioSource* audios[static_cast<int>(UIAudio::TOTAL)] = { nullptr };

};

