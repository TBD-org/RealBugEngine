#pragma once

#include "Scripting/Script.h"
#include "Math/float3.h"

class Teleporter : public Script {
	GENERATE_BODY(Teleporter);

public:

	void Start() override;
	void Update() override;

public:
	float3 targetPosition;
	UID targetObjectUID;
};

