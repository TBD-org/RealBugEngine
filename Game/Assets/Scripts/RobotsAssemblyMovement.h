#pragma once

#include "Scripting/Script.h"

class RobotsAssemblyMovement : public Script
{
	GENERATE_BODY(RobotsAssemblyMovement);

public:

	void Start() override;
	void Update() override;

public:

	UID robotType1 = 0;
	UID robotType2 = 0;
	int numOfRobots = 10;
	bool changeDirection = false;
	float distanceBetweenRobots = 50.f;
	float lineLenght = 160.f;
	float speed = 1.f;

	float distanceBetweenStops = 5.f;
	float stopTime = 1.f;
	float startToSlowDownAt = 2.f;
	bool activateSlowDown = false;

private:
	GameObject* robotsLine = nullptr;
	float3 initialPos = float3(0, 0, 0);
	float3 finalPos = float3(0, 0, 0);
	float3 direction = float3(0, 0, 0);
	std::vector<GameObject*> robots;
	int forward = 1;

	float currentDistanceBetweenStops = 0.f;
	float stopTimer = 0.f;

	bool robotsStopped = false;
	bool slowedDown = false;
};

