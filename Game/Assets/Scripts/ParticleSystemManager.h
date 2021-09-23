#pragma once

#include "Scripting/Script.h"

class ComponentParticleSystem;

class ParticleSystemManager : public Script
{
	GENERATE_BODY(ParticleSystemManager);

public:

	void Start() override;
	void Update() override;

	UID mainParticleUID = 0;

private:

	ComponentParticleSystem* mainParticleSystem = nullptr;

};

