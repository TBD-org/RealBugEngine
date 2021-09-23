#pragma once

#include "Scripting/Script.h"

class GameObject;

class VolumeControl : public Script
{
	GENERATE_BODY(VolumeControl);
public:

	void Start() override;
	void Update() override;
	void OnValueChanged() override;

public:
	UID sliderUID = 0;
	UID gameCameraUID = 0;

private:
	GameObject* sliderGO = nullptr;
	GameObject* gameCamera = nullptr;
};

