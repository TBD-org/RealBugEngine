#pragma once

#include "Script.h"

class GameObject;

class Camera : public Script
{
	GENERATE_BODY(Camera);

public:

	void Start() override;
	void Update() override;

public:

	GameObject* gameObject = nullptr;
	GameObject* player = nullptr;
};
