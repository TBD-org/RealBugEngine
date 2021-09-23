#include "Player.h"

#include "CameraController.h"
#include "GameController.h"
#include "Geometry/Plane.h"
#include "Geometry/Frustum.h"
#include "Geometry/LineSegment.h"

#define PRESSED_TRIGGER_THRESHOLD 0.3f

bool Player::level1Upgrade = false;
bool Player::level2Upgrade = false;

void Player::SetAttackSpeed(float attackSpeed_) {
	attackSpeed = attackSpeed_;
}

void Player::GetHit(float damage_) {
	return;
}

void Player::ResetSwitchStatus() {
	switchInProgress = false;
	playSwitchParticles = true;
	currentSwitchDelay = 0.f;
}

bool Player::CanShoot() {
	return !shootingOnCooldown;
}

MovementDirection Player::GetControllerMovementDirection(bool useGamepad) const {
	float2 leftAxisInput = GetInputFloat2(InputActions::MOVEMENT, useGamepad);
	MovementDirection md = MovementDirection::NONE;

	if (leftAxisInput.y < 0) {
		md = MovementDirection::UP;
	}

	if (leftAxisInput.y > 0) {
		md = MovementDirection::DOWN;
	}

	if (leftAxisInput.x < 0) {
		if (md == MovementDirection::UP) md = MovementDirection::UP_LEFT;
		else if (md == MovementDirection::DOWN) md = MovementDirection::DOWN_LEFT;
		else md = MovementDirection::LEFT;
	}

	if (leftAxisInput.x > 0) {
		if (md == MovementDirection::UP) md = MovementDirection::UP_RIGHT;
		else if (md == MovementDirection::DOWN) md = MovementDirection::DOWN_RIGHT;
		else md = MovementDirection::RIGHT;
	}

	return md;
}

float2 Player::GetControllerOrientationDirection() const {
	return float2(Input::GetControllerAxisValue(Input::SDL_CONTROLLER_AXIS_RIGHTX, 0), Input::GetControllerAxisValue(Input::SDL_CONTROLLER_AXIS_RIGHTY, 0));
}

void Player::LookAtGamepadDir() {
	float2 lookAtInput = GetControllerOrientationDirection();
	if (lookAtInput.x == 0 && lookAtInput.y == 0) return;
	float3 desiredFacePointDir = float3(lookAtInput.x, 0, lookAtInput.y);
	desiredFacePointDir = Lerp(facePointDir, desiredFacePointDir, Time::GetDeltaTime() * normalOrientationSpeed);
	facePointDir = desiredFacePointDir;
}

void Player::LookAtFacePointTarget(bool useGamepad) {
	if (facePointDir.x == 0 && facePointDir.z == 0) return;
	Quat quat = playerMainTransform->GetGlobalRotation();

	float angle = Atan2(facePointDir.x, facePointDir.z);
	Quat rotation = quat.RotateAxisAngle(float3(0, 1, 0), angle);

	float orientationSpeedToUse = IsInstantOrientation(useGamepad) ? -1 : orientationSpeed;

	if (orientationSpeedToUse == -1) {
		playerMainTransform->SetGlobalRotation(rotation);
	} else {
		float3 aux2 = playerMainTransform->GetFront();
		aux2.y = 0;

		facePointDir.Normalize();

		angle = facePointDir.AngleBetween(aux2);
		float3 cross = Cross(aux2, facePointDir.Normalized());
		float dot = Dot(cross, float3(0, 1, 0));
		float multiplier = 1.0f;

		if (dot < 0) {
			angle *= -1;
			multiplier = -1;
		}

		if (Abs(angle) > DEGTORAD * orientationThreshold) {
			Quat rotationToAdd = Quat::Lerp(quat, rotation, Time::GetDeltaTime() * orientationSpeed);
			playerMainTransform->SetGlobalRotation(rotationToAdd);

		} else {
			playerMainTransform->SetGlobalRotation(rotation);
		}
	}
}

void Player::MoveTo() {
	float3 dir = GetDirection();
	float3 newPosition = playerMainTransform->GetGlobalPosition() + ((deceleration > 0.f) ? decelerationDirection : dir);
	if (decelerating)
	{
		deceleration += decelerationRatio * Time::GetDeltaTime();
	} else if (!float3(0,0,0).Equals(dir))
	{
		decelerationDirection = dir;
	}
	if (deceleration >= movementSpeed) {
		decelerating = false;
		deceleration = 0.f;
	}
	agent->SetMaxSpeed((aiming)? movementSpeed:sprintMovementSpeed - deceleration);

	agent->SetMoveTarget(newPosition, false);
}

MovementDirection Player::GetInputMovementDirection(bool useGamepad) const {
	MovementDirection md = GetControllerMovementDirection(useGamepad);
	return md;
}

int Player::GetMouseDirectionState() {
	float3 inputDirection = GetDirection();

	if (!playerMainTransform) return 0;

	float dot = Dot(inputDirection.Normalized(), playerMainTransform->GetFront());
	float3 cross = Cross(inputDirection.Normalized(), playerMainTransform->GetFront());

	// 45� for all animations (Magic numbers 0.923 , 0.383)
	// 60� for axis and 30 for diagonals animations (Magic numbers 0.866 , 0.5)

	if (dot >= 0.923) {
		return 2; //RunForward
	} else if (dot <= -0.923) {
		return 1; //RunBackward
	} else if (dot >= 0.383 && dot < 0.923) {
		if (cross.y > 0) {
			return 14; //RunForwardRight
		} else {
			return 13; //RunForwardLeft
		}
	} else if (dot > -0.923 && dot <= -0.383) {
		if (cross.y > 0) {
			return 16; //RunBackwardRight
		} else {
			return 15; //RunBackwarLeft
		}
	} else if (cross.y > 0) {
		return 4; //RunRight
	} else return 3; //RunLeft
}

bool Player::IsActive() {
	return (characterGameObject) ? characterGameObject->IsActive() : false;
}

void Player::IncreaseUltimateCounter() {
	ultimateChargePoints++;
}

bool Player::GetInputBool(InputActions action, bool useGamepad) {
	switch (action) {
	case InputActions::SWITCH:
		if (useGamepad && Input::IsGamepadConnected(0)) {
			return Input::GetControllerButtonDown(Input::SDL_CONTROLLER_BUTTON_Y, 0);
		} else {
			return Input::GetKeyCodeDown(Input::KEYCODE::KEY_R);
		}
		break;
	case InputActions::SHOOT:
		if (useGamepad && Input::IsGamepadConnected(0)) {
			return Input::GetControllerAxisValue(Input::SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 0) > PRESSED_TRIGGER_THRESHOLD;
		} else {
			return Input::GetMouseButtonRepeat(0) || Input::GetMouseButtonDown(0);
		}
		break;
	case InputActions::ABILITY_1:
		if (useGamepad && Input::IsGamepadConnected(0)) {
			return Input::GetControllerAxisValue(Input::SDL_CONTROLLER_AXIS_TRIGGERLEFT, 0) > PRESSED_TRIGGER_THRESHOLD;
		} else {
			return Input::GetMouseButtonRepeat(2) || Input::GetMouseButtonDown(2);
		}
	case InputActions::ABILITY_2:
		if (useGamepad && Input::IsGamepadConnected(0)) {
			return Input::GetControllerButton(Input::SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 0);
		} else {
			return Input::GetKeyCodeDown(Input::KEYCODE::KEY_Q);
		}
		break;
	case InputActions::ABILITY_3:
		if (useGamepad && Input::IsGamepadConnected(0)) {
			return Input::GetControllerButton(Input::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 0);
		} else {
			return Input::GetKeyCodeDown(Input::KEYCODE::KEY_E);
		}
		break;
	case InputActions::INTERACT:
		if (useGamepad && Input::IsGamepadConnected(0)) {
			return Input::GetControllerButton(Input::SDL_CONTROLLER_BUTTON_A, 0);
		} else {
			return Input::GetKeyCodeDown(Input::KEYCODE::KEY_F);
		}
	case InputActions::AIM:
		if (useGamepad && Input::IsGamepadConnected(0)) {
			//return Input::GetControllerButton(Input::SDL_CONTROLLER_BUTTON_A, 0);
		}
		else {
			return Input::GetMouseButtonRepeat(1);
		}
		break;
	default:
		return false;
	}
}

float2 Player::GetInputFloat2(InputActions action, bool useGamepad) const {
	float2 result = float2(0, 0);
	switch (action) {
	case InputActions::MOVEMENT:
		if (useGamepad && Input::IsGamepadConnected(0)) {
			result.x = Input::GetControllerAxisValue(Input::SDL_CONTROLLER_AXIS_LEFTX, 0);
			result.y = Input::GetControllerAxisValue(Input::SDL_CONTROLLER_AXIS_LEFTY, 0);
		} else {
			if (Input::GetKeyCodeDown(Input::KEYCODE::KEY_D) || Input::GetKeyCodeRepeat(Input::KEYCODE::KEY_D)) {
				result.x = 1;
			} else if (Input::GetKeyCodeDown(Input::KEYCODE::KEY_A) || Input::GetKeyCodeRepeat(Input::KEYCODE::KEY_A)) {
				result.x = -1;
			}

			if (Input::GetKeyCodeDown(Input::KEYCODE::KEY_S) || Input::GetKeyCodeRepeat(Input::KEYCODE::KEY_S)) {
				result.y = 1;
			} else if (Input::GetKeyCodeDown(Input::KEYCODE::KEY_W) || Input::GetKeyCodeRepeat(Input::KEYCODE::KEY_W)) {
				result.y = -1;
			}
		}
		break;
	case InputActions::ORIENTATION:
		if (useGamepad && Input::IsGamepadConnected(0)) {
			result.x = Input::GetControllerAxisValue(Input::SDL_CONTROLLER_AXIS_RIGHTX, 0);
			result.y = Input::GetControllerAxisValue(Input::SDL_CONTROLLER_AXIS_RIGHTY, 0);
		} else {
			//Mouse handling
			float2 mousePos = Input::GetMousePositionNormalized();
			LineSegment ray = lookAtMouseCameraComp->frustum.UnProjectLineSegment(mousePos.x, mousePos.y);
			float3 planeTransform = lookAtMousePlanePosition;
			Plane p = Plane(planeTransform, float3(0, 1, 0));
			result = (p.ClosestPoint(ray) - (playerMainTransform->GetGlobalPosition())).xy();

		}
		break;
	default:
		return float2(0, 0);
	}

	return result;
}

void Player::UpdateFacePointDir(bool useGamepad, bool faceToFront) {
		if (useGamepad) {
			float2 inputFloat2 = GetInputFloat2(InputActions::ORIENTATION, useGamepad);

			facePointDir.x = inputFloat2.x;
			facePointDir.z = inputFloat2.y;

		} else {
			if (faceToFront) {
				float2 inputFloat2 = GetInputFloat2(InputActions::MOVEMENT, false);
				facePointDir.x = inputFloat2.x;
				facePointDir.z = inputFloat2.y;
			}
			else LookAtMouse();
		}
	}

float3 Player::GetDirection() const {
	float3 direction;
	switch (movementInputDirection) {
	case MovementDirection::UP:
		direction = float3(0, 0, -1);
		break;
	case MovementDirection::UP_LEFT:
		direction = float3(-0.5, 0, -0.5);
		break;
	case MovementDirection::UP_RIGHT:
		direction = float3(0.5, 0, -0.5);
		break;
	case MovementDirection::DOWN:
		direction = float3(0, 0, 1);
		break;
	case MovementDirection::DOWN_LEFT:
		direction = float3(-0.5, 0, 0.5);
		break;
	case MovementDirection::DOWN_RIGHT:
		direction = float3(0.5, 0, 0.5);
		break;
	case MovementDirection::RIGHT:
		direction = float3(1, 0, 0);
		break;
	case MovementDirection::LEFT:
		direction = float3(-1, 0, 0);
		break;
	default:
		return float3(0, 0, 0);
	}
	return direction.Normalized();
}

void Player::LookAtMouse() {
	if (lookAtMouseCameraComp) {
		float2 mousePos = Input::GetMousePositionNormalized();
		LineSegment ray = lookAtMouseCameraComp->frustum.UnProjectLineSegment(mousePos.x, mousePos.y);
		float3 planeTransform = lookAtMousePlanePosition;
		Plane p = Plane(planeTransform, float3(0, 1, 0));
		facePointDir = float3(0, 0, 0);
		facePointDir = p.ClosestPoint(ray) - (playerMainTransform->GetGlobalPosition());
	}
}

void Player::Update(bool useGamepad, bool lockMovement, bool lockRotation) {

	if (!lockMovement && !GameController::IsGameplayBlocked()) {
		movementInputDirection = GetInputMovementDirection(useGamepad && Input::IsGamepadConnected(0));
		MoveTo();
	} else {
		movementInputDirection = MovementDirection::NONE;
		if (agent) agent->SetMoveTarget(playerMainTransform->GetGlobalPosition(), false);
	}
	if (!lockRotation && !GameController::IsGameplayBlocked()) {
		UpdateFacePointDir(useGamepad && Input::IsGamepadConnected(0), faceToFront);
		LookAtFacePointTarget(useGamepad && Input::IsGamepadConnected(0));
	}
}

