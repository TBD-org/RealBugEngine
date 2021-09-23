#pragma once

#include "Scripting/Script.h"

class ComponentImage;

class MainMenuUIController : public Script {
	GENERATE_BODY(MainMenuUIController);

public:

	void Start() override;
	void Update() override;

public:
	UID leftFaderObjUID = 0;
	UID rightFaderObjUID = 0;
	float fadeLoopDuration = 4.0f;

	float maxFadeAlpha = 1.0f;
	float minFadeAlpha = 0.7f;

private:
	ComponentImage* leftFaderImage = nullptr;
	ComponentImage* rightFaderImage = nullptr;
	float fadeLoopTimer = 0;
};

