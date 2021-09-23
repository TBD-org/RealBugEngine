#pragma once

#include "Scripting/Script.h"

class ComponentImage;
class ComponentTransform2D;

class HealthLostInstantFeedback : public Script {
	GENERATE_BODY(HealthLostInstantFeedback);

public:

	void Start() override;
	void Update() override;
	void Play();
	void Stop();
public:
	float growthDuration = 0.25f;
	float fadeOutDuration = 0.75f;
	float growthMaxScale = 1.3f;
	UID imageUID = 0;
	ComponentImage* image = nullptr;
	ComponentTransform2D* transform = nullptr;
	float2 originalSize = { 0,0 };
private:
	bool playing = false;
	float growthTimer = 0;
	float fadeOutTimer = 0;
	float4 originalColor = float4(1, 0, 0, 1);
};

