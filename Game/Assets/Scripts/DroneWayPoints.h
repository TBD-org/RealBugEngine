#pragma once

#include "Scripting/Script.h"
#include <vector>

class GameObject;
class ComponentTransform;
class DroneWayPoints : public Script
{
	GENERATE_BODY(DroneWayPoints);

public:

	void Start() override;
	void Update() override;
	std::string initialSubString = "waypoint";
	int totalWayPoint = 0;
	std::vector<GameObject*> wayPoints;
	float distanceOffset = 2;
	int initialWayPoint = 0;
	float speed = 5.0f;
	float rotationSpeed = 0.2f;

private:
	ComponentTransform *ownerTransform = nullptr;
	int currentWayPoint = 0;
	void CheckWayPoint();
	void Seek(const float3 &currentPosition,const float3& newPosition);
};

