#pragma once

#include "Scripting/Script.h"

class PlayerController;
class ComponentTransform2D;
class ComponentImage;
class AbilityRefeshFX;
class ComponentAudioSource;

class HUDManager : public Script {
	GENERATE_BODY(HUDManager);

public:

	void Start() override;
	void Update() override;

public:
	enum class Cooldowns {
		FANG_SKILL_1,
		FANG_SKILL_2,
		FANG_SKILL_3,
		ONIMARU_SKILL_1,
		ONIMARU_SKILL_2,
		ONIMARU_SKILL_3,
		SWITCH_SKILL,
		TOTAL
	};

	enum class SwitchState {
		IDLE,
		PRE_COLLAPSE,
		COLLAPSE,
		PRE_DEPLOY,
		DEPLOY,
		POST_DEPLOY
	};

	enum class PictoState {
		AVAILABLE,
		UNAVAILABLE,
		IN_USE
	};

	enum class HUDManagerAudio { CRITICAL_HEALTH, COOLDOWN_RECOVER, TOTAL };
	ComponentAudioSource* audios[static_cast<int>(HUDManagerAudio::TOTAL)] = { nullptr };

	UID playerObjectUID = 0;

	//Skill HUD

	//Texts
	float4 buttonColorNotAvailable = float4(0.f / 255.f, 40.f / 255.f, 60.f / 255.f, 255.f / 255.f);
	float4 buttonColorAvailable = float4(255.f / 255.f, 255.f / 255.f, 255.f / 255.f, 255.f / 255.f);
	float4 buttonTextColorNotAvailable = float4(255.f / 255.f, 255.f / 255.f, 255.f / 255.f, 255.f / 255.f);
	float4 buttonTextColorAvailable = float4(0.f / 255.f, 40.f / 255.f, 60.f / 255.f, 255.f / 255.f);

	//Skills
	float4 skillColorNotAvailable = float4(0.f / 255.f, 93.f / 255.f, 145.f / 255.f, 255.f / 255.f);
	float4 skillColorAvailable = float4(0.f / 255.f, 177.f / 255.f, 227.f / 255.f, 255.f / 255.f);
	float4 skillPictoColorAvailable = float4(255.f / 255.f, 255.f / 255.f, 255.f / 255.f, 255.f / 255.f);
	float4 skillPictoColorNotAvailable = float4(0.f / 255.f, 177.f / 255.f, 227.f / 255.f, 255.f / 255.f);
	float4 skillPictoColorInUse = float4(0.f / 255.f, 40.f / 255.f, 60.f / 255.f, 255.f / 255.f);

	float4 switchSkillColorDeadCharacter = float4(255.0f / 255.f, 0.0f / 255.f, 0.0f / 255.f, 255.0f / 255.f);
	float4 switchSkillColorNotAvailable = float4(133.f / 255.f, 243.f / 255.f, 196.f / 255.f, 150.f / 255.f);
	float4 switchSkillColorAvailable = float4(133.f / 255.f, 243.f / 255.f, 196.f / 255.f, 255.f / 255.f);

	float4 switchPictoColorNotInUse = float4(255.f / 255.f, 255.f / 255.f, 255.f / 255.f, 255.f / 255.f);
	float4 switchPictoColorInUse = float4(0.f / 255.f, 0.f / 255.f, 0.f / 255.f, 255.f / 255.f);

	UID fangSkillParentUID = 0;
	UID onimaruSkillParentUID = 0;
	UID switchSkillParentUID = 0;

	float cooldowns[static_cast<int>(Cooldowns::TOTAL)];
	float3 cooldownTransformOriginalPositions[static_cast<int>(Cooldowns::TOTAL)];

	std::vector<GameObject*> skillsFang;
	std::vector<GameObject*> skillsOni;

	float switchPreCollapseMovementTime = 0.2f;
	float switchCollapseMovementTime = 0.4f;
	float switchDeployMovementTime = 0.4f;
	float switchPostDeployMovementTime = 0.2f;
	float switchExtraOffset = 20.0f;
	SwitchState switchState = SwitchState::IDLE;
	PictoState pictoStates[static_cast<int>(Cooldowns::TOTAL)];

	float switchColorTimer = 0.0f;
	float switchColorTotalTime = 1.25f;
	bool switchColorIncreasing = true;

	float rotationSpeed = 3.1416 / 4.0f;

	// Health HUD
	UID fangHealthParentUID = 0;
	UID onimaruHealthParentUID = 0;
	UID switchHealthParentUID = 0;

	float4 healthBarBackgroundColorInBackground = float4(0.f / 255.f, 40.f / 255.f, 60.f / 255.f, 30.f / 255.f);
	float4 healthFillBarColorInBackground = float4(255.f / 255.f, 0.f / 255.f, 0.f / 255.f, 30.f / 255.f);
	float4 healthOverlayColorInBackground = float4(255.f / 255.f, 255.f / 255.f, 255.f / 255.f, 100.f / 255.f);
	float4 healthBarBackgroundColor = float4(0.f / 255.f, 40.f / 255.f, 60.f / 255.f, 220.f / 255.f);
	float4 healthFillBarColor = float4(255.f / 255.f, 0.f / 255.f, 0.f / 255.f, 255.f / 255.f);
	float4 healthOverlayColor = float4(255.f / 255.f, 255.f / 255.f, 255.f / 255.f, 255.f / 255.f);

	// Health lost feedback
	float4 healthLostFeedbackFillBarInitialColor = float4(0.f / 255.f, 177.f / 255.f, 227.f / 255.f, 204.f / 255.f);
	float4 healthLostFeedbackFillBarFinalColor = float4(0.f / 255.f, 177.f / 255.f, 227.f / 255.f, 0.f / 255.f);
	float lostHealthFeedbackAlpha = 204.f / 255.f;

	// Switch effects
	float4 healthSwitchStrokeInitialColor = float4(7.f / 255.f, 148.f / 255.f, 119.f / 255.f, 80.f / 255.f);
	float4 healthSwitchStrokeChangingColor = float4(255.f / 255.f, 255.f / 255.f, 255.f / 255.f, 255.f / 255.f);

	float switchBarGrowShrinkTime = 0.05f;

	std::vector<GameObject*> fangHealthChildren;
	std::vector<GameObject*> onimaruHealthChildren;
	std::vector<GameObject*> switchHealthChildren;

	// Sides
	UID sidesHUDParentUID = 0;

	std::vector<GameObject*> sidesHUDChildren;

	float4 sideNormalColor = float4(103.f / 255.f, 180.f / 255.f, 169.f / 255.f, 30.f / 255.f);
	float4 sideHitColor = float4(248.f / 255.f, 47.f / 255.f, 47.f / 255.f, 30.f / 255.f);

	float criticalHealthPercentage = 15.f;

	std::string shieldObjName = "VFXShield";
public:
	void UpdateCooldowns(float onimaruCooldown1, float onimaruCooldown2, float onimaruCooldown3, float fangCooldown1, float fangCooldown2, float fangCooldown3, float switchCooldown, float fangUltimateRemainingNormalizedValue, float oniUltimateRemainingNormalizedValue);
	void UpdateHealth(float fangHealth, float onimaruHealth);
	void HealthRegeneration(float health);
	void StartCharacterSwitch();
	void SetCooldownRetreival(Cooldowns cooldown);
	void StartUsingSkill(Cooldowns cooldown);
	void StopUsingSkill(Cooldowns cooldown);
	void OnCharacterDeath();
	void OnCharacterResurrect();

private:

	PlayerController* playerController = nullptr;
	GameObject* fangObj = nullptr;
	GameObject* onimaruObj = nullptr;

	//Skill HUD
	GameObject* fangSkillParent = nullptr;
	GameObject* onimaruSkillParent = nullptr;
	GameObject* switchSkillParent = nullptr;
	ComponentTransform2D* switchShadeTransform = nullptr;
	ComponentImage* switchGlowImage = nullptr;
	float switchTimer = 0.0f;
	bool abilityCoolDownsRetreived[static_cast<int>(Cooldowns::TOTAL)] = { false,false,false,false,false,false,false };
	bool switchSkillActivated = false;

	// Health HUD
	GameObject* fangHealthParent = nullptr;
	GameObject* onimaruHealthParent = nullptr;
	GameObject* switchHealthParent = nullptr;

	float3 originalFangHealthPosition = { 0,0,0 };
	float3 originalOnimaruHealthPosition = { 0,0,0 };
	float2 originalStrokeSize = { 0,0 };
	float3 originalFillPos = { 0,0,0 };

	float healthOffset = 50;
	bool switchHealthStrokeGrowing = true;
	bool switchHealthStrokeShrinking = false;

	float fangPreviousHealth = 0.f;
	float onimaruPreviousHealth = 0.f;

	bool playingLostHealthFeedback = false;
	float lostHealthTimer = 0.0f;
	float lostHealthFeedbackTotalTime = 1.0f;

	bool playingHitEffect = false;
	float hitEffectTimer = 0.0f;
	float hitEffectTotalTime = 1.0f;
	bool criticalHealthWarning = false;

	// HUD sides
	GameObject* sidesHUDParent = nullptr;

private:
	void AbilityCoolDownEffectCheck(Cooldowns cooldown, GameObject* canvas);
	void UpdateVisualCooldowns(GameObject* canvas, int startingIt); //Update visual cooldown on all abilities of a given character
	void SetRemainingDurationNormalizedValue(GameObject* canvas, int index, float normalizedValue);

	void UpdateCommonSkillVisualCooldown(); //Update visual cooldown on switch ability
	void ManageSwitch();	//This method manages visual effects regarding the Switching of characters (UI WISE) as well 
							//as the color changin and rotation of the picto for the switch icon
	void PlayCoolDownEffect(AbilityRefeshFX* effect, Cooldowns cooldown);
	void PlayHitEffect();
	void ShowCriticalHealthWarning();
	void HideCriticalHealthWarning();
	void PlayLostHealthFeedback();
	void StartLostHealthFeedback();
	void StopLostHealthFeedback();
	void ResetLostHealthFeedback();
	void SetPictoState(Cooldowns cooldown, PictoState newState);

	void GetAllHealthColors();

	void InitializeHealth();
	void InitializeHUDSides();

	void ManageSwitchPreCollapseState(GameObject* activeParent, const std::vector<GameObject*>& skills);
	void ManageSwitchCollapseState(GameObject* activeParent, const std::vector<GameObject*>& skills);

	void ManageSwitchGreenEffect(bool growing, float timer);

};

