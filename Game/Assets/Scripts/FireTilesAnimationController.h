#pragma once

#include "Scripting/Script.h"

class GameObject;
class ComponentAnimation;

class FireTilesAnimationController : public Script
{
	GENERATE_BODY(FireTilesAnimationController);

public:

	void Start() override;
	void Update() override;
	void OnAnimationFinished() override;

public:
	ComponentAnimation* animation = nullptr;

};

