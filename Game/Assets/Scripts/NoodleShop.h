#pragma once

#include "Scripting/Script.h"

class GameObject;
class FactoryDoors;

class NoodleShop : public Script
{
	GENERATE_BODY(NoodleShop);

public:
	UID doorUID = 0;
	UID noodleShopObstacleUID = 0;

public:
	void Start() override;
	void Update() override;
	void OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle = nullptr) override;

private:
	GameObject* gameObject = nullptr;
	FactoryDoors* script = nullptr;

};

