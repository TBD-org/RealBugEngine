#pragma once

#include "Scripting/Script.h"

class GameObject;

class CanvasImage : public Script
{
	GENERATE_BODY(CanvasImage);

public:

	void Start() override;
	void Update() override;

private:
	GameObject* background = nullptr;
};
