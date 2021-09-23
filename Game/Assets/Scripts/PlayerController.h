#pragma once

#include "Player.h"
#include "Scripting/Script.h"
#include "Math/float3.h"
#include <vector>
#include "Fang.h"
#include "Onimaru.h"

class GameObject;
class ComponentTransform;
class ComponentCamera;
class ComponentAudioSource;
class ComponentParticleSystem;
class HUDController;
class HUDManager;
class OnimaruBullet;
class ComponentAgent;
class ComponentAnimation;
class State;
class ResourcePrefab;
class DialogueManager;
struct TesseractEvent;

// We should get these two values from the Character class


class PlayerController : public Script {
	GENERATE_BODY(PlayerController);

public:
	enum class AudioType {
		SWITCH,
		TOTAL
	};

	void Start() override;
	void Update() override;

	//Debug
	void SetInvincible(bool status);
	void SetOverpower(bool status);
	void SetNoCooldown(bool status);
	int GetOverPowerMode();
	float GetOnimaruMaxHealth() const;
	float GetFangMaxHealth() const;
	bool IsPlayerDead();
	bool IsActiveCharacterDead() { return (playerFang.characterGameObject->IsActive() && !playerFang.isAlive) || (playerOnimaru.characterGameObject->IsActive() && !playerOnimaru.isAlive); }
	bool AreBothCharactersAlive() { return playerFang.isAlive && playerOnimaru.isAlive; }
	void TakeDamage(float damage);
	static void SetUseGamepad(bool useGamepad_);

	void AddEnemyInMap(GameObject* enemy);
	void RemoveEnemyFromMap(GameObject* enemy);

	void OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle = nullptr) override;
	void ObtainUpgradeCell();
	void OnCharacterDeath();
	void OnCharacterResurrect();
public:
	//Debug
	bool invincibleMode = false;
	int overpowerMode = 0;
	bool noCooldownMode = false;
	bool debugGetHit = false;

	static bool useGamepad;
	static int currentLevel;

	Onimaru playerOnimaru = Onimaru();
	Fang playerFang = Fang();

	//MainGameObject
	UID mainNodeUID = 0;

	//Fang
	UID fangUID = 0;
	UID fangParticleDashUID = 0;
	UID fangDashDamageUID = 0;
	UID fangTrailGunUID = 0;
	UID fangLeftGunUID = 0;
	UID fangRightGunUID = 0;
	UID fangLeftBulletUID = 0;
	UID fangRightBulletUID = 0;
	UID fangLaserUID = 0;
	UID EMPUID = 0;
	UID EMPEffectsUID = 0;
	UID fangUltimateUID = 0;
	UID fangUltimateVFXUID = 0;
	UID fangRightFootVFX = 0;
	UID fangLeftFootVFX = 0;

	//Onimaru
	UID onimaruUID = 0;
	UID onimaruParticleUID = 0;
	UID onimaruBulletUID = 0;
	UID onimaruGunUID = 0;
	UID onimaruRightHandUID = 0;
	UID onimaruShieldUID = 0;
	UID onimaruBlastEffectsUID = 0;
	UID onimaruUltimateBulletUID = 0;
	UID onimaruLaserUID = 0;
	UID onimaruRightFootVFX = 0;
	UID onimaruLeftFootVFX = 0;

	//HUD
	UID HUDManagerObjectUID = 0;
	float fangRecoveryRate = 1.0f;
	float onimaruRecoveryRate = 1.0f;
	bool hitTaken = false;

	//Camera
	UID cameraUID = 0;
	GameObject* camera = nullptr;
	float cameraOffsetZ = 20.f;
	float cameraOffsetY = 10.f;
	float cameraOffsetX = 0.f;
	bool useSmoothCamera = true;
	float smoothCameraSpeed = 5.0f;

	//Switch
	UID switchParticlesUID = 0;
	float switchCooldown = 5.f;
	bool switchInProgress = false;
	float switchDelay = 0.37f;
	float switchSphereRadius = 5.f;
	float switchDamage = 1.f;

	//Upgrades
	int obtainedUpgradeCells = 0;

private:
	void CheckCoolDowns();
	void SwitchCharacter();
	void UpdatePlayerStats();
	bool CanSwitch();
	void ResetSwitchStatus();
	bool IsVulnerable() const;
private:

	//Switch
	float switchCooldownRemaining = 0.f;
	bool switchInCooldown = false;

	//HUD
	float fangRecovering = 0.f;
	float onimaruRecovering = 0.f;
	HUDManager* hudManagerScript = nullptr;

	//Switch
	float currentSwitchDelay = 0.f;
	bool playSwitchParticles = true;
	GameObject* switchEffects = nullptr;
	ComponentSphereCollider* sCollider = nullptr;
	std::vector<GameObject*> switchCollisionedGO;
	bool switchFirstHit = true;

	//Camera
	ComponentCamera* compCamera = nullptr;
	ComponentTransform* cameraTransform = nullptr;

	//Audio
	ComponentAudioSource* audios[static_cast<int>(AudioType::TOTAL)] = { nullptr };

	friend class DialogueManager;
};
