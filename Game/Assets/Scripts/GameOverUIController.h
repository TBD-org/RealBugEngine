#pragma once

#include "Scripting/Script.h"

class UISpriteSheetPlayer;
class ComponentImage;
class ComponentText;

class GameOverUIController : public Script {
	GENERATE_BODY(GameOverUIController);

public:

	void Start() override;
	void Update() override;
	void GameOver();

public:
	float minVibrationTime = 2.0f;
	float maxVibrationTime = 3.0f;
	float fadeInTime = 1.0f;
	float scrollDuration = 20.0f;
	enum class GameOverState { OFFLINE, FADE_IN, START, IDLE, VIBRATION };

	UID inOutPlayerUID = 0;
	UID loopPlayerUID = 0;
	UID vibrationPlayerUID = 0;

	UID playAgainButtonUID = 0;
	UID mainMenuButtonUID = 0;
	UID backgroundUID = 0;
	UID scrollingBackgroundObjUID = 0;

private:
	void EnterIdleState();
	void SetColors(float delta);
private:
	GameOverState state = GameOverState::OFFLINE;
	float vibrationTimer = 0.0f;
	float fadeInTimer = 0.0f;
	float scrollingTimer = 0.0f;
	UISpriteSheetPlayer* inOutPlayer = nullptr;
	UISpriteSheetPlayer* loopPlayer = nullptr;
	UISpriteSheetPlayer* vibrationPlayer = nullptr;

	ComponentImage* playAgainButtonImage = nullptr;
	ComponentImage* mainMenuButtonImage = nullptr;
	ComponentImage* backgroundImage = nullptr;
	ComponentImage* scrollingBackgroundImage = nullptr;

	ComponentText* playAgainButtonText = nullptr;
	ComponentText* mainMenuButtonText = nullptr;
	float4 backgroundOriginalColor = float4(0, 0, 0, 0);
	float4 playAgainButtonImageOriginalColor = float4(0, 0, 0, 0);
	float4 mainMenuButtonImageOriginalColor = float4(0, 0, 0, 0);
	float4 scrollingBackgroundImageOriginalColor = float4(20,70,100,225);
};

