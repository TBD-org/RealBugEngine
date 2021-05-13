#pragma once

#include "Scripting/Script.h"
#include "Math/float3.h"
#include <vector>

class GameObject;
class ComponentTransform;
class ComponentCamera;
class ComponentAudioSource;
class ComponentParticleSystem;

class HUDController;

class ComponentAnimation;
class State;
struct TesseractEvent;

enum class MovementDirection {
		NONE = 0, UP = 1, UP_LEFT = 2, LEFT = 3, DOWN_LEFT = 4, DOWN = 5, DOWN_RIGHT = 6, RIGHT = 7, UP_RIGHT = 8
};

class PlayerController : public Script
{
	GENERATE_BODY(PlayerController);

public:

	void Start() override;
	void Update() override;
	void ReceiveEvent(TesseractEvent& e) override;

	void HitDetected();
	
public:

	GameObject* gameObject = nullptr;
	GameObject* camera = nullptr;
	GameObject* fang = nullptr;
	GameObject* onimaru = nullptr;
	GameObject* fangParticle = nullptr;
	GameObject* onimaruParticle = nullptr;

	UID fangUID = 0;
	UID onimaruUID = 0;
	UID fangParticleUID = 0;
	UID onimaruParticleUID = 0;
	UID mainNodeUID = 0;
	UID cameraUID = 0;

	UID canvasUID = 0;

	UID switchAudioSourceUID = 0;
	UID dashAudioSourceUID = 0;

	bool hitTaken = false;
	int lifePointsFang = 5;
	int lifePointsOni = 5;

	float distanceRayCast = 2.f;
	float dashCooldown = 5.f; //seconds
	float switchCooldown = 5.f;
	float movementSpeed = 5.f;
	float dashSpeed = 100.f;
	float dashDistance = 10.f;
	float cameraOffsetZ = 20.f;
	float cameraOffsetY = 10.f;
	float shootCooldown = 0.1f;

	std::vector<std::string> states {"Idle" , 
								"RunBackward" , "RunForward" , "RunLeft" , "RunRight" , 
								"DashBackward", "DashForward" , "DashLeft" , "DashRight" , 
								"Death" , "Hurt" , "LeftShot" , "RightShot"  
	};

private:

	void PlayAnimation(MovementDirection md, bool isFang);
	void MoveTo(MovementDirection md);
	void InitDash(MovementDirection md);
	void Dash();
	void LookAtMouse();
	void CheckCoolDowns();
	void SwitchCharacter();
	void Shoot();
	bool CanDash();
	bool CanSwitch();
	bool CanShoot();

	float3 GetDirection(MovementDirection md) const;
	MovementDirection GetInputMovementDirection() const;
	int GetMouseDirectionState(MovementDirection input);

private:
	
	float dashError = 2.f;
	float dashCooldownRemaing = 0.f;
	bool dashInCooldown = false;
	bool dashing = false;

	float switchCooldownRemaing = 0.f;
	bool switchInCooldown = false;

	float shootCooldownRemaing = 0.f;
	bool shooting = false;

	float3 initialPosition = float3(0, 0, 0);
	float3 dashDestination = float3(0, 0, 0);
	float3 dashDirection = float3(0, 0, 0);
	float3 facePointDir = float3(0,0,0);
	MovementDirection dashMovementDirection = MovementDirection::NONE;
	ComponentTransform* transform = nullptr;
	ComponentCamera* compCamera = nullptr;
	
	//Animation
	ComponentAnimation* fangAnimation = nullptr;
	State* fangCurrentState = nullptr;
	ComponentAnimation* onimaruAnimation = nullptr;
	State* onimaruCurrentState = nullptr;

	//Particles
	ComponentParticleSystem* fangCompParticle = nullptr;
	ComponentParticleSystem* onimaruCompParticle = nullptr;
	
	//Audio
	ComponentAudioSource* shootAudioSource = nullptr;
	ComponentAudioSource* dashAudioSource = nullptr;
	ComponentAudioSource* switchAudioSource = nullptr;

	HUDController* hudControllerScript = nullptr;
};

