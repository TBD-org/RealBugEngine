#pragma once

#include "Player.h"

class HUDController;
class HUDManager;
class UltimateFang;
class Fang : public Player {
public:

	enum class FANG_AUDIOS {
		DASH,
		EMP,
		ULTIMATE,
		SHOOT,
		FOOTSTEP_RIGHT,
		FOOTSTEP_LEFT,
		HIT,
		DEATH,
		TOTAL
	};
	enum class FANG_STATES {
		IDLE = 0,
		RUN_BACKWARD,
		RUN_FORWARD,
		RUN_LEFT,
		RUN_RIGHT,
		DASH_BACKWARD,
		DASH_FORWARD,
		DASH_LEFT,
		DASH_RIGHT,
		DEATH,
		SHOOTING,
		DRIFT,
		DASH,
		RUN_FORWARD_LEFT, 
		RUN_FORWARD_RIGHT, 
		RUN_BACKWARD_LEFT,
		RUN_BACKWARD_RIGHT,
		EMP = 21,
		ULTIMATE,
		IDLE_AIM,
		FOOT_SWITCH,
		SPRINT
	};

	// ------- Contructors ------- //
	Fang() {};
	void Update(bool lastInputGamepad = false, bool lockMovement = false, bool lockOrientation = false) override;
	void CheckCoolDowns(bool noCooldownMode = false) override;
	void OnAnimationFinished() override;
	void OnAnimationSecondaryFinished() override {};
	void OnAnimationEvent(StateMachineEnum stateMachineEnum, const char* eventName);
	void GetHit(float damage_) override;;
	bool IsAiming();
	bool CanSwitch() const override;
	bool IsInstantOrientation(bool useGamepad) const override;
	float GetRealDashCooldown();
	float GetRealEMPCooldown();
	float GetRealUltimateCooldown();
	void IncreaseUltimateCounter();
	void Init(UID fangUID = 0, UID dashParticleUID = 0, UID leftGunUID = 0, UID rightGunUID = 0, UID rightBulletUID = 0, UID leftBulletUID = 0, UID laserUID = 0, UID cameraUID = 0, UID HUDManagerObjectUID = 0, UID dashUID = 0, UID EMPUID = 0, UID EMPEffectsUID = 0, UID fangUltimateUID = 0, UID ultimateVFXUID = 0, UID rightFootVFX = 0, UID leftFootVFX = 0);
	bool IsVulnerable() const override;

public:
	std::vector<std::string> states{ 
						"Idle" ,
						"RunBackward" , "RunForward" , "RunLeft" , "RunRight" , //1 - 4
						"DashBackward", "DashForward" , "DashLeft" , "DashRight" , //5 - 8
						"Death" , "Shooting" , "Drift", "Dash", //9 - 12
						"RunForwardLeft", "RunForwardRight", "RunBackwardLeft", "RunBackwardRight", // 13 - 16
						"DashBackward", "DashForward" , "DashLeft" , "DashRight", //17 - 20
						"EMP", "Ultimate","IdleAim","FootSwitch", //21 - 24
						"Sprint" // 25 - 28
	};


	//Dash
	float dashCooldown = 5.f;
	float dashSpeed = 100.f;
	float dashDuration = 0.1f;
	GameObject* dash = nullptr;
	float dashDamage = 1.0f;

	//EMP
	GameObject* EMP = nullptr;
	float EMPRadius = 5.f;
	float EMPCooldown = 7.f;

	//Ultimate
	int ultimateCooldown = 2;
	float ultimateMovementSpeed = 4.0f;
	bool isUltimate = false;

private:

	//Dash
	float dashCooldownRemaining = 0.f;
	float dashRemaining = 0.f;
	bool dashing = false;
	bool dashInCooldown = false;
	bool hasDashed = false;
	bool inCombat = false;
	float3 initialPosition = float3(0, 0, 0);
	float3 dashDirection = float3(0, 0, 0);

	//EMP
	float EMPCooldownRemaining = 0.f;
	bool EMPInCooldown = false;
	ComponentParticleSystem* EMPEffects = nullptr;

	//Shoot
	ComponentTransform* rightGunTransform = nullptr;
	ComponentTransform* leftGunTransform = nullptr;
	ComponentParticleSystem* bullet = nullptr;
	ComponentParticleSystem* dashParticle = nullptr;
	ComponentParticleSystem* rightBullet = nullptr;
	ComponentParticleSystem* leftBullet = nullptr;
	GameObject* rightBulletAux = nullptr;
	GameObject* leftBulletAux = nullptr;
	GameObject* fangLaser = nullptr;
	bool shooting = false;
	int transitioning = 0;

	//Movement
	MovementDirection dashMovementDirection = MovementDirection::NONE;

	//Movement VFX
	ComponentParticleSystem* rightFootstepsVFX = nullptr;
	ComponentParticleSystem* leftFootstepsVFX = nullptr;

	//HUD
	HUDManager* hudManagerScript = nullptr;

	//Ultimate
	int ultimateCooldownRemaining = 0;
	bool ultimateInCooldown = false;
	float oldMovementSpeed = 1.0f;
	UltimateFang* ultimateScript = nullptr;
	ComponentParticleSystem* ultimateVFX = nullptr;

	//Audios
	ComponentAudioSource* fangAudios[static_cast<int>(FANG_AUDIOS::TOTAL)] = { nullptr };
	
	
private:
	void InitDash();
	void Dash();
	bool CanDash();

	void ActivateEMP();
	bool CanEMP();

	bool CanShoot() override;
	void Shoot() override;
	void PlayAnimation();

	void ResetIsInCombatValues();
	void ActiveUltimate();
	bool CanUltimate();
};
