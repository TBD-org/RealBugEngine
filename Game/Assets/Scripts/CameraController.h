#pragma once

#include "Scripting/Script.h"

class PlayerController;
class ComponentTransform;
class ComponentCamera;

class CameraController : public Script {
	GENERATE_BODY(CameraController);


public:

	void Start() override;
	void Update() override;
	void StartShake();
	void ChangeCameraOffset(float x, float y, float z);
	void RestoreCameraOffset();

public:
	float cameraInitialOffsetZ = 20.f;
	float cameraInitialOffsetY = 20.f;
	float cameraInitialOffsetX = 0.f;
	float cameraOffsetY = 0.f;
	float cameraOffsetZ = 0.f;
	float cameraOffsetX = 0.f;
	float aimingDistance = 0.25f;
	bool useSmoothCamera = true;
	bool useAimingCamera = true;
	float aimingCameraSpeed = 1.0f;
	float smoothCameraSpeed = 5.0f;
	UID playerControllerObjUID = 0;
	PlayerController* playerController = nullptr;
	ComponentTransform* transform = nullptr;
	ComponentCamera* camera = nullptr;
	float shakeTotalTime = 1.0f;
	float shakeTimer = 0.0f;
	float shakeMultiplier = 0.2f;

	float aimingPositionX = 0.0f;
	float aimingPositionZ = 0.0;

	float3 aimingPosition = float3(0.0f, 0.0f, 0.0f);

private:
	float2 GetRandomPosInUnitaryCircle(float2 center);
	
};

