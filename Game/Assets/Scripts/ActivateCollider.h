#pragma once

#include "Scripting/Script.h"

class ActivateCollider : public Script
{
	GENERATE_BODY(ActivateCollider);

public:

	void Start() override;
	void Update() override;

};

