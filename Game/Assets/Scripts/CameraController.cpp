#include "CameraController.h"

#include "PlayerController.h"
#include "GameController.h"
#include "Components/ComponentTransform.h"

#include "Geometry/LineSegment.h"
#include "Geometry/Plane.h"

#define PI 3.14159


EXPOSE_MEMBERS(CameraController) {
	MEMBER(MemberType::GAME_OBJECT_UID, playerControllerObjUID),
	MEMBER_SEPARATOR("Camera Positioning"),
	MEMBER(MemberType::FLOAT, cameraOffsetX),
	MEMBER(MemberType::FLOAT, cameraOffsetY),
	MEMBER(MemberType::FLOAT, cameraOffsetZ),
	MEMBER(MemberType::FLOAT, smoothCameraSpeed),
	MEMBER(MemberType::FLOAT, aimingCameraSpeed),
	MEMBER(MemberType::FLOAT, aimingDistance),
	MEMBER(MemberType::BOOL, useSmoothCamera),
	MEMBER_SEPARATOR("Shaker Control"),
	MEMBER(MemberType::FLOAT, shakeTotalTime),
	MEMBER(MemberType::FLOAT, shakeTimer),
	MEMBER(MemberType::FLOAT, shakeMultiplier),
	MEMBER(MemberType::BOOL, useAimingCamera),
};

GENERATE_BODY_IMPL(CameraController);

void CameraController::Start() {
	transform = GetOwner().GetComponent<ComponentTransform>();
	camera = GetOwner().GetComponent<ComponentCamera>();
	GameObject* playerControllerObj = GameplaySystems::GetGameObject(playerControllerObjUID);
	if (playerControllerObj) {
		playerController = GET_SCRIPT(playerControllerObj, PlayerController);
	}

	cameraInitialOffsetX = cameraOffsetX;
	cameraInitialOffsetY = cameraOffsetY;
	cameraInitialOffsetZ = cameraOffsetZ;

	RestoreCameraOffset();
}

void CameraController::Update() {
	if (playerController == nullptr || transform == nullptr || camera == nullptr) return;

	float3 playerGlobalPos = playerController->playerFang.playerMainTransform->GetGlobalPosition();
	float3 desiredPosition = playerGlobalPos + float3(cameraOffsetX, cameraOffsetY, cameraOffsetZ);
	float3 smoothedPosition = desiredPosition;

	if (useSmoothCamera) {
		if (useAimingCamera && (playerController->playerFang.IsAiming() || playerController->playerOnimaru.IsAiming())) {
			float2 mousePosition = Input::GetMousePositionNormalized();
			LineSegment ray = camera->frustum.UnProjectLineSegment(mousePosition.x, mousePosition.y);
			Plane p = Plane(playerGlobalPos, float3(0, 1, 0));
			float3 aimingPosition = playerGlobalPos + (p.ClosestPoint(ray) - playerGlobalPos) * aimingDistance + float3(cameraOffsetX, cameraOffsetY, cameraOffsetZ);

			smoothedPosition = float3::Lerp(transform->GetGlobalPosition(), aimingPosition, aimingCameraSpeed * Time::GetDeltaTime());

		} else {
			smoothedPosition = float3::Lerp(transform->GetGlobalPosition(), desiredPosition, smoothCameraSpeed * Time::GetDeltaTime());
		}
	}

	if (shakeTimer > 0 && !GameController::IsGameplayBlocked()) {
		float2 shakeDir = GetRandomPosInUnitaryCircle(float2(0, 0));
		transform->SetGlobalPosition(smoothedPosition + transform->GetRight() * shakeDir.x * shakeMultiplier + transform->GetUp() * shakeDir.y * shakeMultiplier);
		shakeTimer -= Time::GetDeltaTime();
		Screen::SetChromaticAberration(true);

	} else {
		transform->SetGlobalPosition(smoothedPosition);
		Screen::SetChromaticAberration(false);
	}
}



void CameraController::StartShake() {
	shakeTimer = shakeTotalTime;
}

void CameraController::ChangeCameraOffset(float x, float y, float z) {
	cameraOffsetZ = z;
	cameraOffsetY = y;
	cameraOffsetX = x;
}

void CameraController::RestoreCameraOffset() {
	cameraOffsetZ = cameraInitialOffsetZ;
	cameraOffsetY = cameraInitialOffsetY;
	cameraOffsetX = cameraInitialOffsetX;
}


float2 CameraController::GetRandomPosInUnitaryCircle(float2 center) {
	float random = (static_cast<float>(rand() % 101)) / 100.0f;
	float random2 = (static_cast<float>(rand() % 101)) / 100.0f;

	float r = sqrt(random);
	float theta = random2 * 2 * PI;
	float x = center.x + r * cos(theta);
	float y = center.y + r * sin(theta);

	return float2(x, y);
}