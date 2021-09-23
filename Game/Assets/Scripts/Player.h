#pragma once

#include "Character.h"
#include "Math/float3.h"

class CameraController;

enum class MovementDirection {
	NONE = 0,
	UP = 1,
	UP_LEFT = 2,
	LEFT = 3,
	DOWN_LEFT = 4,
	DOWN = 5,
	DOWN_RIGHT = 6,
	RIGHT = 7,
	UP_RIGHT = 8
};

enum class InputActions {
	MOVEMENT,
	ORIENTATION,
	SWITCH,
	SHOOT,
	ABILITY_1,
	ABILITY_2,
	ABILITY_3,
	INTERACT,
	AIM
};

class Player : public Character {
public:
	// ------- Contructors ------- //
	Player() {};

	Player(float lifePoints_, float movementSpeed_, float damageHit_, float attackSpeed_)
		:
		attackSpeed(attackSpeed_) {
		lifePoints = lifePoints_;
		movementSpeed = movementSpeed_;
		damageHit = damageHit_;
		SetTotalLifePoints(lifePoints);
	}

	// ------- Core Functions ------ //
	void SetAttackSpeed(float attackSpeed_);
	virtual void GetHit(float damage_) override;

	void LookAtMouse();
	MovementDirection GetInputMovementDirection(bool useGamepad) const;
	float3 GetDirection() const;
	virtual void Shoot() {}
	virtual void Update(bool lastInputGamepad = false, bool lockMovement = false, bool lockRotation = false);
	virtual void CheckCoolDowns(bool noCooldownMode = false) {}
	virtual bool CanSwitch() const = 0;

	virtual void OnAnimationFinished() = 0;
	virtual void OnAnimationSecondaryFinished() = 0;
	virtual bool IsInstantOrientation(bool useGamepad) const = 0;
	virtual bool IsVulnerable() const = 0;

	int GetMouseDirectionState();
	bool IsActive();
	static bool GetInputBool(InputActions action, bool useGamepad = false);
	float2 GetInputFloat2(InputActions action, bool useGamepad = false) const;
	void UpdateFacePointDir(bool useGamepad, bool faceToFront = false);
	virtual void IncreaseUltimateCounter();
	void SetClipSpeed(ResourceClip* clip, float movementSpeed) {
		clip->speed = movementSpeed * animationSpeedFactor;
	}

public:
	float rangedDamageTaken = 1.0f;
	float meleeDamageTaken = 1.0f;
	float attackSpeed = 1.0f;
	float attackCooldownRemaining = 0.f;
	bool shooting = false;
	float orientationSpeed = 6.0f;
	float orientationThreshold = 1.0f;
	int ultimateChargePoints = 0;
	const int ultimateChargePointsTotal = 10;
	bool ultimateOn = false;
	bool shootingOnCooldown = false;
	float normalOrientationSpeed = 7.5f;
	float sprintMovementSpeed = 12.0f;
	float3 lookAtMousePlanePosition = float3(0.f, 0.f, 0.f);
	float normalMovementSpeed = 4.0f;
	float ultimateMovementSpeed = 4.0f;
	static bool level1Upgrade;
	static bool level2Upgrade;
	ComponentCamera* lookAtMouseCameraComp = nullptr;
	CameraController* cameraController = nullptr;
	float3 facePointDir = float3(0.f, 0.f, 0.f);
	MovementDirection movementInputDirection = MovementDirection::NONE;
	ComponentTransform* playerMainTransform = nullptr;
	float ultimateTimeRemaining = 0.0f;
	float ultimateTotalTime = 4.6f;
	float animationSpeedFactor = 1.f;
	//Combat
	float aimTime = 5.f;
	float decelerationRatio = 16.f;
	UID lookAtPointUID = 0;
protected:
	void MoveTo();
	//Combat
	float timeWithoutCombat = 0.f;
	bool aiming = false;
	bool faceToFront = false;
	float deceleration = 0.f;
	float3 decelerationDirection = float3(0.f,0.f,0.f);
	bool decelerating = false;
private:
	virtual bool CanShoot();
	void ResetSwitchStatus();
	MovementDirection GetControllerMovementDirection(bool useGamepad = false) const;
	float2 GetControllerOrientationDirection() const;
	void LookAtGamepadDir();
	void LookAtFacePointTarget(bool useGamepad);

private:
	float currentSwitchDelay = 0.f;
	bool playSwitchParticles = true;
	bool switchInProgress = false;
	float switchDelay = 0.37f;
};
