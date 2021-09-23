#pragma once

#include "Scripting/Script.h"

#include "AIState.h"

#include "Math/Quat.h"

class ComponentTransform;
class ComponentAgent;


class AIMovement : public Script {
	GENERATE_BODY(AIMovement);

public:

	void Start() override;
	void Update() override;

	void Seek(AIState state, const float3& newPosition, int speed, bool orientateToDir);
	void Flee(AIState state, const float3& fromPosition, int speed, bool orientateToDir);
	void Orientate(const float3& direction);
	void Stop();
	bool CharacterInSight(const GameObject* character, const float searchRadius);
	bool CharacterInAttackRange(const GameObject* character, const float meleeRange);
	void SetClipSpeed(UID clipUID, float speed = 1.f);
	GameObject* SearchReferenceInHierarchy(GameObject* root, std::string name);

public:
	static int maxAcceleration;
	float rotationSmoothness = 0.2f;

private:

	float3 velocity = float3(0, 0, 0);
	Quat targetRotation = Quat(0, 0, 0, 1);
	ComponentTransform* ownerTransform = nullptr;
	ComponentAgent* agent = nullptr;
};
