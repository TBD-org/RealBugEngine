#pragma once

#include "Scripting/Script.h"

class GameObject;
class ComponentTransform2D;

class FullHealthBarFeedback : public Script
{
	GENERATE_BODY(FullHealthBarFeedback);

public:

	UID fillUID = 0;
	UID strokeUID = 0;

	float growingTime = 0.3f;
	float shrinkingTime = 0.3f;
	float growingScale = 1.3f;

public:

	void Start() override;
	void Update() override;
	void Play();
	void Stop();
	void Reset();

private:
	GameObject* stroke = nullptr;
	GameObject* fill = nullptr;
	bool playing = false;

	float growTimer = 0.0f;
	float shrinkTimer = 0.0f;

	ComponentTransform2D* fillTransform = nullptr;

	float2 originalSize = { 0.f, 0.f };
};

