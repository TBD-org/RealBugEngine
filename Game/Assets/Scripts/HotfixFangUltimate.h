#pragma once

#include "Scripting/Script.h"

class HotfixFangUltimate : public Script
{
	GENERATE_BODY(HotfixFangUltimate);

public:

	void Start() override;
	void Update() override;

public:
	float lifeTime = 1.0f;
};

