#pragma once

#include "Scripting/Script.h"

class ComponentImage;
class ComponentAudioSource;
class UISpriteSheetPlayer;
class CanvasFader;
class StartTitleGlitchOnPlay;

class GlitchyTitleController : public Script {
	GENERATE_BODY(GlitchyTitleController);

public:

	void Start() override;
	void Update() override;
	void PressedPlay(StartTitleGlitchOnPlay* pressedButton_); //Method to be called to trigger FadeOut when play is pressed and whiel fade to black is happening
	bool ReadyForTransition()const;

public:

	enum class GlitchTitleAudio { FADE_IN, GLITCH, TOTAL };

	float maxTimeForTimeBetweenGlitches = 5.0f;
	float minTimeForTimeGlitches = 15.0f;
	float fadeInTotalTime = 0.5f;
	float glitchDuration = 0.3f;
	UID startUpTitleObjUID = 0;
	UID loopingTitleObjUID = 0;
	UID glitchTitleObjUID = 0;
	UID startPlayingTitleObjUID = 0;
	UID blackImageObjUID = 0;
	StartTitleGlitchOnPlay* pressedButton = nullptr;


private:
	void StartStateIdle();
	void PlayAudio(GlitchTitleAudio type);

private:
	enum class GlitchState { FADE_IN, WAIT_START, WAIT_START_SPRITESHEET, START, IDLE, GLITCH, PLAY, WAIT_FADE_OUT, FADE_OUT };
	GlitchState glitchState = GlitchState::FADE_IN;
	float timerBetweenGlitches = 10.0f;
	float glitchTimer = 0.0f;
	float stateTimer = 0.0f;

	UISpriteSheetPlayer* startUpTitleSpriteSheetPlayer = nullptr;
	UISpriteSheetPlayer* loopingTitleSpriteSheetPlayer = nullptr;
	UISpriteSheetPlayer* glitchTitleSpriteSheetPlayer = nullptr;
	UISpriteSheetPlayer* startPlayingTitleSpriteSheetPlayer = nullptr;
	CanvasFader* canvasFader = nullptr;
	const float4 alphaBlack = float4(0, 0, 0, 1);
	const float4 noAlphaBlack = float4(0, 0, 0, 0);
	ComponentAudioSource* audios[static_cast<int>(GlitchTitleAudio::TOTAL)] = { nullptr };


};

