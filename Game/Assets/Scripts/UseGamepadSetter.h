#pragma once

#include "Scripting/Script.h"

class PlayerController;

class UseGamepadSetter : public Script {
	GENERATE_BODY(UseGamepadSetter);

public:

	void Start() override;
	void Update() override;
	void OnToggled(bool b) override;

private:
	void SearchForPlayerRef();
	PlayerController* playerController = nullptr;

};

