#pragma once

#include "Scripting/Script.h"

class ComponentImage;
class ComponentTransform2D;

class AbilityRefreshEffect : public Script {
	GENERATE_BODY(AbilityRefreshEffect);

public:

	void Start() override;
	void Update() override;
	void Play();


public:
	float totalEffectTime = 0;
	float effectScale = 0;
	UID effectMember1UID = 0;
	UID skillParentObjectUID = 0;
	bool debugPlay = false;
private:

	float3 effectScaleVector = float3(0, 0, 0);
	float3 originalEffectScaleVector = float3(0, 0, 0);
	float3 originalScaleVector = float3(0, 0, 0);
	float effectTimer = 0;

	float originalAlpha = 0;
	float3 originalColor = float3(0, 0, 0);

	bool isPlaying = false;

	ComponentImage* effectMember1 = nullptr;
	ComponentTransform2D* skillObjTransform2D = nullptr;
	ComponentTransform2D* effectTransform2D = nullptr;



};

