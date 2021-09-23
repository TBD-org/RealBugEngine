#pragma once

#include "Scripting/Script.h"

class TrailScript : public Script
{
	GENERATE_BODY(TrailScript);

public:

	void Start() override;
	void Update() override;
	void OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle = nullptr) override;

public:
	int speed = 200;
	float life = 0.2f;
	float offsetPosition = 2.0f;
private:
	float restLife = 0.0f;
	bool itsVertical = false;
	bool firstTime = false;
};
