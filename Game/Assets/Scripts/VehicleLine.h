#pragma once

#include "Scripting/Script.h"

#include <queue>
#include <vector>

class ComponentTransform;
class ResourcePrefab;

class VehicleLine : public Script
{
	GENERATE_BODY(VehicleLine);

public:
	
	void Start() override;
	void Update() override;

private:

	void UpdateVehicles();
	bool VehicleOutsideMap(const float3 posVehicle);

public:

	float speed = 0.f;
	float timeBetweenSpawns = 0.f;

	UID vehicle1 = 0;
	UID vehicle2 = 0;
	UID vehicle3 = 0;
	UID vehicle4 = 0;
	UID vehicle5 = 0;
	UID vehicle6 = 0;
	UID vehicle7 = 0;
	UID vehicle8 = 0;
	UID vehicle9 = 0;

private:
	UID vehiclesUID[9] = {0,0,0,0,0,0,0,0,0};
	std::vector<GameObject*> vehicles;
	
	float timeToSpawn = 0.f;
	int actualVehicle = 0;
	int previousVehicle = 0;
};

