#pragma once

#include "Scripting/Script.h"

class ComponentImage;

class CanvasFader : public Script {
	GENERATE_BODY(CanvasFader);

public:

	void Start() override;
	void Update() override;
	void FadeOut();
	void FadeIn();
	void Stop();
	bool IsPlaying() const;

public:
	float fadeDuration = 0.5f;
	UID blackImageObjUID = 0;
	bool fadeInOnStart = false;

private:
	void UpdateBlackImage();
	void Play();

private:
	enum class FadeState { IDLE, FADE_IN, FADE_OUT };
	FadeState fadeState = FadeState::IDLE;
	bool playing = false;
	ComponentImage* blackImage = nullptr;
	float fadeTimer = 0.0f;
	const float4 alphaBlack = float4(0, 0, 0, 1);
	const float4 noAlphaBlack = float4(0, 0, 0, 0);

};

