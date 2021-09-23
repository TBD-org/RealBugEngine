#pragma once

#include "Scripting/Script.h"

class ComponentTransform;
class GameObject; 

class MilibotScript : public Script
{
	GENERATE_BODY(MilibotScript);

public:

	void Start() override;
	void Update() override;

public:
	float angle = 0.0f;
	float rotationSpeed = 0.0f;
	float amplitude = 1.0f;

private:
	ComponentTransform* transform = nullptr;
	float3 initialPosition = float3(0.0f, 0.0f, 0.0f);
	float offset = -pi/2;
};

