#pragma once

#include "GameObject.h"
#include "Scripting/Script.h"
#include "Components/ComponentAudioSource.h"

class GameOverMusicController : public Script
{
	GENERATE_BODY(GameOverMusicController);

public:

	void Start() override;
	void Update() override;

private:
	bool hasEntered = false;
	GameObject* deathScreen = nullptr;

	//Audio
	ComponentAudioSource* gameOverMusic = nullptr;
	ComponentAudioSource* levelMusic = nullptr;

};

