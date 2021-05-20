#pragma once

#include "Scripting/Script.h"

class GameObject;
class ComponentTransform;

class ControllerTest : public Script
{
	GENERATE_BODY(ControllerTest);

public:

	void Start() override;
	void Update() override;

public:
	UID kinematicUID = 0;

private:
	GameObject* kinematic = nullptr;
	ComponentTransform* transform = nullptr;
};

