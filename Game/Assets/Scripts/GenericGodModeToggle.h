#pragma once

#include "Scripting/Script.h"

class GameObject;
class ComponentToggle;

class GenericGodModeToggle : public Script {
	GENERATE_BODY(GenericGodModeToggle);

public:
	UID godControllerUID = 0;
	unsigned int index = 0;

public:
	void Start() override;
	void Update() override;
	void OnToggled(bool toggled_) override;

private:
	GameObject* gameObject = nullptr;
	GameObject* godController = nullptr;
	ComponentToggle* toggle = nullptr;
};
