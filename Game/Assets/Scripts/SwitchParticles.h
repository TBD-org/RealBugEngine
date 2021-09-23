#pragma once

#include "Scripting/Script.h"

class GameObject;
class ComponentParticleSystem;

class SwitchParticles : public Script
{
	GENERATE_BODY(SwitchParticles);

public:
	UID playerUID = 0;
	UID mainParticuleUID = 0;

public:
	void Start() override;
	void Update() override;
	void Play();

private:
	GameObject* player = nullptr;
	ComponentParticleSystem* mainParticleSystem = nullptr;
	bool playing = false;
};

