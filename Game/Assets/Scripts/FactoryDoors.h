#pragma once

#include "Scripting/Script.h"

class ComponentAnimation;

class FactoryDoors : public Script {
	GENERATE_BODY(FactoryDoors);

public:
	void Start() override;
	void Update() override;
	void Open();

private:
	ComponentAnimation* animation = nullptr;
};
