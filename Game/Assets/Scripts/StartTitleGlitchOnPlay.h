#pragma once

#include "Scripting/Script.h"
#include "UIAudioType.h"

class GlitchyTitleController;
class GameObject;
class ComponentSelectable;
class ComponentAudioSource;
class CanvasFader;
class SwapPanels;

class StartTitleGlitchOnPlay : public Script {
	GENERATE_BODY(StartTitleGlitchOnPlay);

public:
	void Start() override;
	void Update() override;

	void OnButtonClick() override;
	void DoTransition();

public:
	UID controllerObjUID = 0;
	UID sceneUID = 0;
	UID fadeToBlackObjectUID = 0;
	UID swapPanelsObjUID = 0;
	int levelNum = 1;
	int checkpointNum = -1;

private:
	GlitchyTitleController* controller = nullptr;
	void PlayAudio(UIAudio type);

private:
	GameObject* player = nullptr;

	/* UI */
	CanvasFader* canvasFader = nullptr;

	/* Audio */
	bool playHoveredAudio = true;
	bool pressed = false;
	ComponentSelectable* selectable = nullptr;
	ComponentAudioSource* audios[static_cast<int>(UIAudio::TOTAL)] = { nullptr };
	SwapPanels* swapPanelsScript = nullptr;
};

