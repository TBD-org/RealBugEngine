#pragma once

#include "Scripting/Script.h"

class VSyncSetter : public Script
{
	GENERATE_BODY(VSyncSetter);

public:

	void Start() override;
	void Update() override;
	void OnToggled(bool toggled) override;
};

