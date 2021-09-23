#pragma once

#include "Scripting/Script.h"
#include "GameObject.h"

class RangerProjectileScript : public Script {
	GENERATE_BODY(RangerProjectileScript);

public:
	void Start() override;
	void Update() override;
	void OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle = nullptr) override;
	void SetRangerDirection(Quat rangerDirection_);
	Quat& GetRangerDirection();
public:
	int speed = 200;
	float life = 0.2f;

private:
	float restLife = 0.0f;
	bool itsVertical = false;
	bool firstTime = false;
	Quat rangerDirection = Quat(0, 0, 0, 0);
};
