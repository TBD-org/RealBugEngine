#pragma once

#include "Scripting/Script.h"

class PlayerController;

class UseControllerSetter : public Script {
	GENERATE_BODY(UseControllerSetter);

public:

	void Start() override;
	void Update() override;
	void OnToggled(bool toggled_) override;

private:
	void SearchForReferences();

private:
	PlayerController* playerController = nullptr;

};

