#pragma once

#include "Scripting/Script.h"

class ComponentImage;
class ComponentAudioSource;
class GameObject;

class LowHPWarning : public Script {
	GENERATE_BODY(LowHPWarning);

public:

	UID effectUID = 0;
	UID audioUID = 0;
	float effectTime = 2.0f; // In seconds
	float fadeOutTime = 1.0f;

public:

	void Start() override;
	void Update() override;
	void Play();
	void Stop();

private:
	bool playing = false;

	float fadeOutCurrentTime = 0.0f;
	float effectCurrentTime = 0.0f;
	float4 color = { 1, 0, 0, 0.5f };

	GameObject* effect = nullptr;
	ComponentImage* vignette = nullptr;
	ComponentAudioSource* audio = nullptr;
};

