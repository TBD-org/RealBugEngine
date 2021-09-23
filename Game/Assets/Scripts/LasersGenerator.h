#pragma once

#include "Scripting/Script.h"

class ComponentAnimation;

class LasersGenerator : public Script
{
	GENERATE_BODY(LasersGenerator);

	enum class GeneratorState {
		START = 0,
		SHOOT,
		IDLE
	};

public:

	void Start() override;
	void Update() override;

	void Init();

	UID laserTargetUID = 0;
	UID laserWarningUID = 0;
	UID pairGeneratorUID = 0;

	float coolDownOnTimer = 0.0f;
	float coolDownOn = 2.0f;

	float coolDownOffTimer = 0.0f;
	float coolDownOff = 2.0f;

	float chargingDuration = 1.f;

private:

	ComponentAnimation* animationComp = nullptr;
	ComponentAnimation* pairAnimationComp = nullptr;
	GeneratorState currentState = GeneratorState::IDLE;

	GameObject* laserObject = nullptr;
	GameObject* laserWarning = nullptr;
	LasersGenerator* pairScript = nullptr;

	bool beingUsed = false;

	float chargingTimer = 0.f;

	const std::string states[3] = { "Shooting", "Shooting", "Idle"};
};

