#pragma once

#include "Scripting/Script.h"

class FullScreenSetter : public Script {
	GENERATE_BODY(FullScreenSetter);

public:

	void Start() override;
	void Update() override;
	void OnToggled(bool toggled_) override;
};

