#pragma once
#pragma once

#include "Scripting/Script.h"

class GameObject;
class ComponentText;
class AbilityRefreshEffect;
class AbilityRefreshEffectProgressBar;

#define MAX_HEALTH 10
#define LOW_HEALTH_WARNING 2



class HUDController : public Script {
	GENERATE_BODY(HUDController);
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

	void Start() override;
	void Update() override;
	void UpdateCooldowns(float onimaruCooldown1, float onimaruCooldown2, float onimaruCooldown3,
		float fangCooldown1, float fangCooldown2, float fangCooldown3,
		float switchCooldown);

	void UpdateHP(float currentHp, float altHp);
	void UpdateDurableHPLoss(GameObject* targetCanvas);
	void ChangePlayerHUD(int fangLives, int oniLives);
	void HealthRegeneration(float currentHp, float hpRecovered);
	void ResetHealthRegenerationEffects(float currentHp);
	void ResetCooldownProgressBar();

	void UpdateScore(int score_);
	void SetCooldownRetreival(Cooldowns cooldown);
	static float MapValue01(float value, float min, float max);
	void SetFangCanvas(bool value);
	void SetOnimaruCanvas(bool value);

public:


	UID fangUID = 0;
	UID onimaruUID = 0;

	UID fangMainCanvasUID = 0;
	UID onimaruMainCanvasUID = 0;

	UID fangSkillsMainCanvasUID = 0;
	UID onimaruSkillsMainCanvasUID = 0;

	UID fangSkillsSecondCanvasUID = 0;
	UID onimaruSkillsSecondCanvasUID = 0;

	UID fangHealthMainCanvasUID = 0;
	UID onimaruHealthMainCanvasUID = 0;

	UID fangHealthSecondCanvasUID = 0;
	UID onimaruHealthSecondCanvasUID = 0;

	UID lowHealthWarningEffectUID = 0;

	UID swapingSkillCanvasUID = 0;

	UID scoreTextUID = 0;

	UID canvasHUDUID = 0;

	float timeToFadeDurableHealthFeedbackInternal = 2.0f;
	float delaySwitchTime = .2f;// The time that takes to switch the canvas

private:
	void UpdateComponents();
	void UpdateCommonSkill();
	void UpdateVisualCooldowns(GameObject* canvas, bool isMain, int startingAbilityIndex);
	void UpdateCanvasHP(GameObject* targetCanvas, int health, bool darkened);
	void OnHealthLost(GameObject* targetCanvas, int health);
	void StopHealthLostInstantEffects(GameObject* targetCanvas);
	void LoadHealthFeedbackStates(GameObject* targetCanvas, int health);
	void LoadCooldownFeedbackStates(GameObject* targetCanvas, int startingIndex);
	void AbilityCoolDownEffectCheck(Cooldowns cooldown, GameObject* canvas);
	void PlayCoolDownEffect(AbilityRefreshEffect* effect, Cooldowns cooldown);
	void PlayProgressBarEffect(AbilityRefreshEffectProgressBar* effect, Cooldowns cooldown);
	bool AnyTimerCounting(bool isFang);
private:

	GameObject* fang = nullptr;
	GameObject* onimaru = nullptr;

	GameObject* fangCanvas = nullptr;
	GameObject* onimaruCanvas = nullptr;

	GameObject* fangSkillsMainCanvas = nullptr;
	GameObject* onimaruSkillsMainCanvas = nullptr;
	GameObject* fangSkillsSecondCanvas = nullptr;
	GameObject* onimaruSkillsSecondCanvas = nullptr;

	GameObject* fangHealthMainCanvas = nullptr;
	GameObject* onimaruHealthMainCanvas = nullptr;
	GameObject* fangHealthSecondCanvas = nullptr;
	GameObject* onimaruHealthSecondCanvas = nullptr;

	GameObject* lowHealthWarningEffect = nullptr;

	GameObject* swapingSkillCanvas = nullptr;

	GameObject* canvasHUD = nullptr;

	float cooldowns[static_cast<int>(Cooldowns::TOTAL)];

	/* COLORS */

	const float4 colorMagenta = float4(236, 60, 137, 255) / 255;
	const float4 colorWhite = float4(255, 255, 255, 255) / 255;

	const float3 colorBlueForCD = float3(47.0f, 187.0f, 200.0f) / 255.0f;
	const float4 colorBlueBackground = float4(0.0f, 81.0f, 127.0f, 255.0f) / 255.0f;

	int prevLivesFang = MAX_HEALTH;
	int prevLivesOni = MAX_HEALTH;

	bool lowHPWarningActive = false;

	float timeToFadeDurableHealthFeedback = 0.0f;

	float remainingDurableHealthTimesFang[MAX_HEALTH] = { 0,0,0,0,0,0,0,0,0,0 };
	float remainingDurableHealthTimesOni[MAX_HEALTH] = { 0,0,0,0,0,0,0,0,0,0 };
	bool abilityCoolDownsRetreived[static_cast<int>(Cooldowns::TOTAL)] = { false,false,false,false,false,false,false };


	ComponentText* scoreText = nullptr;
	int score = 0;

	// For characte switching
	float currentTime = 0;
	bool isSwitching = false;
};

