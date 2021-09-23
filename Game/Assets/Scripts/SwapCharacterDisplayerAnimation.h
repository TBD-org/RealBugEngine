#pragma once

#include "Scripting/Script.h"

class SwapCharacterDisplayerAnimation : public Script
{
	GENERATE_BODY(SwapCharacterDisplayerAnimation);

public:

	void Start() override;
	void Update() override;
	void Play();

public:
	float breakTimeForPopups = .2f;					// Time between the progress bar loading up and the "pop" effect. This is the time that needs to pass AFTER the progress bar loads up
	float durationFirstEmission = .1f;
	float breakTimeBetweenFirstAndSecond = .1f;
	float durationSecondEmission = .1f;
	float breakTimeBetweenSecondAndThird = .1f;
	float durationThirdEmission = .1f;
	float3 minScale = float3(1, 0, 1);		// Scales for the pop up effect
	float3 maxScale = float3(1, 1, 1);		// Scales for the pop up effect
	float minTransparency = 0;
	float maxTransparency = 1;
	float breakTimeForSwap = .2f;
	float swappingDuration = .2f;
	float positionToLerpSwapMain = 90;				// This is the final position to set on the lerp of swapAnimation
	float positionToLerpSwapAlternative = 90;
	bool isFang = true;								// Default value to who's set atm
	float3 initialPositionMainCanvas = float3(-45.200, 168.650, 0);
	float3 initialPositionAlternativeCanvas = float3(26.400, -9.000, 0);

	bool debugPlay = false;

	UID firstBarEffectUID = 0;
	UID secondBarEffectUID = 0;
	UID thirdBarEffectUID = 0;

	UID fangMainDisplayerUID = 0;
	UID fangPrimaryUID = 0;
	UID fangAlternativeUID = 0;
	UID onimaruMainDisplayerUID = 0;
	UID onimaruPrimaryUID = 0;
	UID onimaruAlternativeUID = 0;

private:
	void IncreaseOverTime(GameObject* image, float currentTime, float maxTime);
	void SwapAnimation(GameObject* targetCanvas, bool isMain, bool playReverse, float currentTime, float maxTime);

private:
	GameObject* firstBarEffect = nullptr;
	GameObject* secondBarEffect = nullptr;
	GameObject* thirdBarEffect = nullptr;

	GameObject* fangMainDisplayer = nullptr;
	GameObject* fangPrimary = nullptr;
	GameObject* fangAlternative = nullptr;
	GameObject* onimaruMainDisplayer = nullptr;
	GameObject* onimaruPrimary = nullptr;
	GameObject* onimaruAlternative = nullptr;

	float currentTime = 0.0f;
	bool animationLoadUpFinished = true;
	bool animationPopUpFinished = true;
	bool animationSwapFinished = true;

	float3 initialScaleCanvas = float3(1, 1, 1);		// Scale of the canvas to shrink
};

