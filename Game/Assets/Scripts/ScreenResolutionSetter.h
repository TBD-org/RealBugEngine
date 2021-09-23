#pragma once

#include "Scripting/Script.h"
#include "UIAudioType.h"

class ComponentText;
class ComponentAudioSource;
class ComponentSelectable;

 /// <summary>
 /// This class is meant to modify externs delcared inside ScreenResolution confirmer whenever Button is pressed, as well as setting the text (if found)
 /// </summary>
 class ScreenResolutionSetter : public Script {
	GENERATE_BODY(ScreenResolutionSetter);

public:

	void Start() override;
	void Update() override;
	void OnButtonClick() override;
	void PlayAudio(UIAudio type);

public:
	bool increasing = false;
	UID textObjectID = 0;

private:
	void IncreaseResolution(int multiplier);
	void UpdateResolution();
private:

	ComponentText* text = nullptr;

	bool playHoveredAudio = true;
	ComponentSelectable* selectable = nullptr;
	ComponentAudioSource* audios[static_cast<int>(UIAudio::TOTAL)] = { nullptr };
};

