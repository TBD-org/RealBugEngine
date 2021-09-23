#pragma once

#include "Scripting/Script.h"
#include <vector>

class ResourcePrefab;

class UltimateFang : public Script
{
	GENERATE_BODY(UltimateFang);

public:

	void Start() override;
	void Update() override;
	void OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle = nullptr) override;
	void StartUltimate();
	void EndUltimate();

private:
	Quat DirectionToQuat(float3 dir);

public:
	float radius = 5.0f;
	UID fangBulletUID = 0;

private:

	ResourcePrefab* bullet = nullptr;

	bool tickOn = false;
	float tickCurrent = 0.2f;
	float tickDuration = 0.2f;
	std::vector<GameObject> collisionedGameObject;
};
