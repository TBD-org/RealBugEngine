#pragma once

#include "Scripting/Script.h"

class GameObject;

class UISpriteSheetPlayer : public Script {
	GENERATE_BODY(UISpriteSheetPlayer);

public:

	void Start() override;
	void Update() override;
	void Play();
	void Stop();
	bool IsPlaying()const;
	float CalcDuration();
public:

	float secondsPerFrame = 0.1f;
	bool loops = false;
	bool playOnAwake = false;

private:
	void FillFrameVector();

private:
	float animationTimer = 0.0f;
	std::vector<GameObject*>frames;
	int currentFrame = 0;
	GameObject* currentFrameObj = nullptr;
	bool playing = false;
};

