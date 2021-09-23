#pragma once

#include "Scripting/Script.h"

class ComponentImage;
class ComponentTransform2D;
class ComponentAudioSource;

class AbilityRefeshFX : public Script {
	GENERATE_BODY(AbilityRefeshFX);

public:

	void Start() override;
	void Update() override;
	void PlayEffect();

public:
	float totalEffectTime = 0;
	float effectScale = 0;

	UID pictoObjectUID = 0;
	UID keyObjectUID = 0;
	bool debugPlay = false;

private:
	float3 effectScaleVector = float3(0, 0, 0);
	float3 originalEffectScaleVector = float3(0, 0, 0);
	float3 originalScaleVector = float3(0, 0, 0);
	float effectTimer = 0;

	bool isPlaying = false;

	ComponentImage* effectMember1 = nullptr;
	ComponentTransform2D* pictoTransform2D = nullptr;
	ComponentTransform2D* keyTransform2D = nullptr;
	ComponentAudioSource* effectAudio = nullptr;
	std::vector<GameObject*>children;
	ComponentImage* glowImage = nullptr;
};

