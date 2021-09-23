#pragma once

#include "Scripting/Script.h"

class StartController : public Script
{
	GENERATE_BODY(StartController);

public:

	void Start() override;
	void Update() override;

public:
	UID gameCameraUID;

private:
	GameObject* gameCamera = nullptr;
};