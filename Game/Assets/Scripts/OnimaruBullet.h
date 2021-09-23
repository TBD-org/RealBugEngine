#pragma once

#include "Scripting/Script.h"
#include "Math/Quat.h"

class OnimaruBullet : public Script
{
	GENERATE_BODY(OnimaruBullet);

public:

	void Start() override;
	void Update() override;
	void SetOnimaruDirection(Quat direction);
	void OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle = nullptr) override;

public:
	int speed = 200;
	float life = 0.2f;

private:
	Quat onimaruDirection;
};
