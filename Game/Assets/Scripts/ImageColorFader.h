#pragma once

#include "Scripting/Script.h"

class ComponentImage;

class ImageColorFader : public Script {
	GENERATE_BODY(ImageColorFader);

public:

	void Start() override;
	void Update() override;
	void Play();
	void Stop();
public:
	float3 originalColor = float3(-1, -1, -1);
	float originalAlpha = -1.0f;
	float3 targetColor = float3(-1, -1, -1);
	float targetAlpha = -1.0f;
	float totalTransitionTime = 1.0f;
	bool disableOnFinish = true;
	bool running = false;
	bool startOnAwake = false;
	UID imageHoldingObjectUID = 0;
	bool debugPlay = false;
private:
	void UpdateImageColor();

private:
	ComponentImage* targetImage = nullptr;
	float transitionTime = 0;
	float4 originalColorToUse = float4(0, 0, 0, 0);
	float4 targetColorToUse = float4(0, 0, 0, 0);

};

