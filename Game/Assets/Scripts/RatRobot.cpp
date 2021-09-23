#include "RatRobot.h"

#include "GameplaySystems.h"
#include "GameController.h"

#include "Math/MathFunc.h"

EXPOSE_MEMBERS(RatRobot) {
	MEMBER(MemberType::GAME_OBJECT_UID, playerId),
		MEMBER(MemberType::FLOAT, fleeRange),
		MEMBER(MemberType::FLOAT, rotationSmoothness)
};

GENERATE_BODY_IMPL(RatRobot);

void RatRobot::Start() {
	player = GameplaySystems::GetGameObject(playerId);
	if (!player) player = GameplaySystems::GetGameObject("Player");
	if (player) playerTransform = player->GetComponent<ComponentTransform>();

	transform = GetOwner().GetComponent<ComponentTransform>();
	agent = GetOwner().GetComponent<ComponentAgent>();
	animation = GetOwner().GetComponent<ComponentAnimation>();
}

void RatRobot::Update() {
	if (!transform || !agent) return;

	float3 position = transform->GetGlobalPosition();
	float3 playerPosition = playerTransform->GetGlobalPosition();
	float3 playerToRat = position - playerPosition;
	float distanceSq = playerToRat.LengthSq();

	if (distanceSq < fleeRange * fleeRange) {
		float3 fleeDestination = playerPosition + playerToRat.Normalized() * fleeRange;

		agent->SetMoveTarget(fleeDestination, true);
	}

	float3 agentVelocity = agent->GetVelocity();
	const float minSpeed = 0.5f;

	switch (state) {
	case RatRobotState::IDLE1:
	case RatRobotState::IDLE2:
	case RatRobotState::IDLE3:
		if (agentVelocity.LengthSq() > minSpeed * minSpeed) {
			ChangeState(RatRobotState::RUN);
		}
		break;
	case RatRobotState::RUN:
		if (agentVelocity.LengthSq() < minSpeed * minSpeed) {
			ChangeState(RatRobotState::IDLE1);
		} else {
			Quat targetRotation = Quat::LookAt(float3(-1, 0, 0), agentVelocity.Normalized(), float3(0, 1, 0), float3(0, 1, 0));
			Quat rotation = Quat::Slerp(transform->GetGlobalRotation(), targetRotation, Min(Time::GetDeltaTime() / Max(rotationSmoothness, 0.000001f), 1.0f));
			transform->SetGlobalRotation(rotation);
		}
		break;
	}
}

void RatRobot::OnAnimationFinished() {
	int newState = rand() % 2;
	switch (state) {
	case RatRobotState::IDLE1:
		if (newState == 0) {
			ChangeState(RatRobotState::IDLE2);
		} else {
			ChangeState(RatRobotState::IDLE3);
		}
		break;
	case RatRobotState::IDLE2:
		if (newState == 0) {
			ChangeState(RatRobotState::IDLE1);
		} else {
			ChangeState(RatRobotState::IDLE3);
		}
		break;
	case RatRobotState::IDLE3:
		if (newState == 0) {
			ChangeState(RatRobotState::IDLE1);
		} else {
			ChangeState(RatRobotState::IDLE2);
		}
		break;
	}
}

void RatRobot::ChangeState(RatRobotState newState) {
	if (!agent || !animation) return;

	switch (newState) {
	case RatRobotState::IDLE1:
		switch (state) {
		case RatRobotState::IDLE2:
			animation->SendTrigger("Idle2Idle1");
			break;
		case RatRobotState::IDLE3:
			animation->SendTrigger("Idle3Idle1");
			break;
		case RatRobotState::RUN:
			animation->SendTrigger("RunIdle1");
			break;
		}

		break;
	case RatRobotState::IDLE2:
		switch (state) {
		case RatRobotState::IDLE1:
			animation->SendTrigger("Idle1Idle2");
			break;
		case RatRobotState::IDLE3:
			animation->SendTrigger("Idle3Idle2");
			break;
		case RatRobotState::RUN:
			animation->SendTrigger("RunIdle2");
			break;
		}

		break;
	case RatRobotState::IDLE3:
		switch (state) {
		case RatRobotState::IDLE1:
			animation->SendTrigger("Idle1Idle3");
			break;
		case RatRobotState::IDLE2:
			animation->SendTrigger("Idle2Idle3");
			break;
		case RatRobotState::RUN:
			animation->SendTrigger("RunIdle3");
			break;
		}

		break;
	case RatRobotState::RUN:
		switch (state) {
		case RatRobotState::IDLE1:
			animation->SendTrigger("Idle1Run");
			break;
		case RatRobotState::IDLE2:
			animation->SendTrigger("Idle2Run");
			break;
		case RatRobotState::IDLE3:
			animation->SendTrigger("Idle3Run");
			break;
		}
		break;
	}

	state = newState;
}