#pragma once

#include "Scripting/Script.h"

class ComponentProgressBar;
class ComponentImage;

class AbilityRefreshEffectProgressBar : public Script
{
	GENERATE_BODY(AbilityRefreshEffectProgressBar);

public:

	void Start() override;
	void Update() override;
	void Play();
	void ResetBar();

public:
	float durationLoadUp = .2f;
	bool debugPlay = false;
	UID progressBarUID = 0;

private:
	void IncreaseOverTime(GameObject* image, float currentTime, float maxTime);

private:
	GameObject* progressBarGO;
	ComponentProgressBar* progressBar;

	float currentTime = 0.0f;
	bool animationLoadUpFinished = true;
};

