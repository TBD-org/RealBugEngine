#include "PlayerController.h"
#include "GameObject.h"
#include "GameplaySystems.h"
#include "Math/Quat.h"
#include "Geometry/Plane.h"
#include "Geometry/Frustum.h"
#include "Geometry/LineSegment.h"
#include "Math/float3x3.h"
#include "Math/float2.h"
#include <algorithm>
#include <string>

EXPOSE_MEMBERS(PlayerController) {
	// Add members here to expose them to the engine. Example:
	MEMBER(MemberType::GAME_OBJECT_UID, fangUID),
		MEMBER(MemberType::GAME_OBJECT_UID, robotUID),
		MEMBER(MemberType::GAME_OBJECT_UID, mainNodeUID),
		MEMBER(MemberType::GAME_OBJECT_UID, cameraUID),
		MEMBER(MemberType::GAME_OBJECT_UID, fangParticlesUID),
		MEMBER(MemberType::GAME_OBJECT_UID, onimaruParticlesUID)
};

GENERATE_BODY_IMPL(PlayerController);

void PlayerController::Start() {
	gameObject = GameplaySystems::GetGameObject(mainNodeUID);
	fang = GameplaySystems::GetGameObject(fangUID);
	robot = GameplaySystems::GetGameObject(robotUID);
	camera = GameplaySystems::GetGameObject(cameraUID);
	fParticles = GameplaySystems::GetGameObject(fangParticlesUID);
	onimariParticles = GameplaySystems::GetGameObject(onimaruParticlesUID);

	fangParticles = fParticles->GetComponent<ComponentParticleSystem>();
	fangParticles = onimariParticles->GetComponent<ComponentParticleSystem>();

	if (gameObject) {
		transform = gameObject->GetComponent<ComponentTransform>();
	}
	if (camera) {
		compCamera = camera->GetComponent<ComponentCamera>();
		if (compCamera) GameplaySystems::SetRenderCamera(compCamera);
	}
	if (transform) {
		initialPosition = transform->GetPosition();
	}
	if (fang) {
		fang->Enable();
	}
	if (robot) {
		robot->Disable();
	}
}

void PlayerController::MoveTo(MovementDirection md) {
	if (transform) {
		float modifier = 1.0f;
		float3 newPosition = transform->GetPosition();
		if (Input::GetKeyCode(Input::KEYCODE::KEY_LSHIFT)) {
			modifier = 2.0f;
		}
		newPosition += GetDirection(md) * movementSpeed * Time::GetDeltaTime() * modifier;
		transform->SetPosition(newPosition);
	}
}

void PlayerController::LookAtMouse() {
	float2 mousePos = Input::GetMousePositionNormalized();
	LineSegment ray = compCamera->frustum.UnProjectLineSegment(mousePos.x, mousePos.y);
	float3 cameraGlobalPos = camera->GetComponent<ComponentTransform>()->GetGlobalPosition();
	Plane p = Plane(transform->GetGlobalPosition(), float3(0, 1, 0));
	float dist;
	float3 facePoint = float3(0, 0, 0);
	cameraGlobalPos.z = 0;
	facePoint = p.ClosestPoint(ray) - (transform->GetGlobalPosition());
	//Debug::Log((" x: " + std::to_string(facePoint.x) + " y: " + std::to_string(facePoint.y) + " z: " + std::to_string(facePoint.z)).c_str());
	Quat quat = transform->GetRotation();
	float angle = Atan2(facePoint.x, facePoint.z);
	Quat rotation = quat.RotateAxisAngle(float3(0, 1, 0), angle);
	transform->SetRotation(rotation);
}

float3 PlayerController::GetDirection(MovementDirection md) const {
	float3 direction = float3(0, 0, 0);
	switch (md)
	{
	case MovementDirection::UP:
		direction = float3(0, 0, -1);
		break;
	case MovementDirection::UP_LEFT:
		direction = float3(-1, 0, -1);
		break;
	case MovementDirection::UP_RIGHT:
		direction = float3(1, 0, -1);
		break;
	case MovementDirection::DOWN:
		direction = float3(0, 0, 1);
		break;
	case MovementDirection::DOWN_LEFT:
		direction = float3(-1, 0, 1);
		break;
	case MovementDirection::DOWN_RIGHT:
		direction = float3(1, 0, 1);
		break;
	case MovementDirection::RIGHT:
		direction = float3(1, 0, 0);
		break;
	case MovementDirection::LEFT:
		direction = float3(-1, 0, 0);
		break;
	default:
		break;
	}
	return direction;
}

void PlayerController::InitDash(MovementDirection md) {
	dashDirection = GetDirection(md);
	dashDestination = transform->GetPosition();
	dashDestination += dashDistance * dashDirection;
	dashCooldownRemaing = dashCooldown;
	dashInCooldown = true;
	dashing = true;
}

void PlayerController::Dash() {
	if (dashing) {
		float3 newPosition = transform->GetPosition();
		newPosition += dashSpeed * Time::GetDeltaTime() * dashDirection;
		transform->SetPosition(newPosition);
		if (std::abs(std::abs(newPosition.x) - std::abs(dashDestination.x)) < dashError &&
			std::abs(std::abs(newPosition.z) - std::abs(dashDestination.z)) < dashError) {
			dashing = false;
		}
	}
}

bool const PlayerController::CanDash() const {
	return !dashing && !dashInCooldown;
}

void PlayerController::CheckCoolDowns() {
	dashCooldownRemaing -= Time::GetDeltaTime();
	switchCooldownRemaing -= Time::GetDeltaTime();

	if (dashCooldownRemaing <= 0.f) {
		dashCooldownRemaing = 0.f;
		dashInCooldown = false;
	}

	if (switchCooldownRemaing <= 0.f) {
		switchCooldownRemaing = 0.f;
		switchInCooldown = false;
	}
}

bool const PlayerController::CanSwitch() const {
	return !switchInCooldown;
}

void PlayerController::SwitchCharacter() {
	if (CanSwitch()) {
		switchInCooldown = true;
		if (fang->IsActive()) {
			Debug::Log("Swaping to robot...");
			fang->Disable();
			robot->Enable();
		}
		else {
			Debug::Log("Swaping to fang...");
			robot->Disable();
			fang->Enable();
		}
		switchCooldownRemaing = switchCooldown;
	}
}

void PlayerController::Update() {
	if (!gameObject) return;
	if (!camera) return;
	if (!transform) return;
	if (!fParticles) return;
	if (!onimariParticles) return;
	CheckCoolDowns();
	ComponentTransform* cameraTransform = camera->GetComponent<ComponentTransform>();
	gameObject = GameplaySystems::GetGameObject(mainNodeUID);
	cameraTransform->SetPosition(float3(transform->GetGlobalPosition().x, cameraTransform->GetGlobalPosition().y, (transform->GetGlobalPosition().z + 20.0f)));
	if (cameraTransform) {
		MovementDirection md = MovementDirection::NONE;
		if (!dashing) {
			if (Input::GetKeyCode(Input::KEYCODE::KEY_W)) {
				md = MovementDirection::UP;
			}
			if (Input::GetKeyCode(Input::KEYCODE::KEY_S)) {
				md = MovementDirection::DOWN;
			}
			if (Input::GetKeyCode(Input::KEYCODE::KEY_A)) {
				if (md == MovementDirection::UP) md = MovementDirection::UP_LEFT;
				else if (md == MovementDirection::DOWN) md = MovementDirection::DOWN_LEFT;
				else md = MovementDirection::LEFT;
			}
			if (Input::GetKeyCode(Input::KEYCODE::KEY_D)) {
				if (md == MovementDirection::UP) md = MovementDirection::UP_RIGHT;
				else if (md == MovementDirection::DOWN) md = MovementDirection::DOWN_RIGHT;
				else md = MovementDirection::RIGHT;
			}
			LookAtMouse();
			if (md != MovementDirection::NONE) {
				MoveTo(md);
			}
			if (CanSwitch() && Input::GetKeyCode(Input::KEYCODE::KEY_T)) {
				SwitchCharacter();
			}
		}
		if (CanDash() && Input::GetKeyCode(Input::KEYCODE::KEY_SPACE)) {
			InitDash(md);
		}
		Dash();

		if (timeRestToShoot <= 0) {
			if (Input::GetMouseButtonDown(0)) {
				timeRestToShoot = timeToShoot;
				if (fang->IsActive()) {
					fangParticles->Play();
				}
				else {
					oniParticles->Play();
				}
			}
		}
		else {
			Debug::Log(std::to_string(timeRestToShoot).c_str());
			timeRestToShoot -= Time::GetDeltaTime() * speedShoot;
		}
	}
}