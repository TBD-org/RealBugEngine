#include "HUDController.h"

#include "GameObject.h"
#include "Components/UI/ComponentImage.h"
#include "HealthLostInstantFeedback.h"
#include "AbilityRefreshEffect.h"
#include "AbilityRefreshEffectProgressBar.h"
#include "LowHPWarning.h"
#include "FullHealthBarFeedback.h"
#include "SwapCharacterDisplayerAnimation.h"
#include "GameplaySystems.h"

#define HIERARCHY_INDEX_MAIN_HEALTH 1

/* Current skill index in hierarchy */
#define HIERARCHY_INDEX_MAIN_ABILITY_EFFECT 0
#define HIERARCHY_INDEX_MAIN_ABILITY_FILL 2
#define HIERARCHY_INDEX_MAIN_ABILITY_IMAGE 4
#define HIERARCHY_INDEX_MAIN_BUTTON_UP 5
#define HIERARCHY_INDEX_MAIN_BUTTON_DOWN 6  

/* Alternative skill index in hierarchy */
#define HIERARCHY_INDEX_SECONDARY_ABILITY_FILL 1

/* Swap chararcters index in hierarchy */
#define HIERARCHY_INDEX_SWAP_ABILITY_EFFECT 2
#define HIERARCHY_INDEX_SWAP_ABILITY_FILL 3
#define HIERARCHY_INDEX_SWAP_ABILITY_IMAGE 4


EXPOSE_MEMBERS(HUDController) {
	// Add members here to expose them to the engine. Example:
	MEMBER(MemberType::GAME_OBJECT_UID, fangMainCanvasUID),
		MEMBER(MemberType::GAME_OBJECT_UID, onimaruMainCanvasUID),
		MEMBER(MemberType::GAME_OBJECT_UID, fangSkillsMainCanvasUID),
		MEMBER(MemberType::GAME_OBJECT_UID, onimaruSkillsMainCanvasUID),
		MEMBER(MemberType::GAME_OBJECT_UID, fangSkillsSecondCanvasUID),
		MEMBER(MemberType::GAME_OBJECT_UID, onimaruSkillsSecondCanvasUID),
		MEMBER(MemberType::GAME_OBJECT_UID, fangHealthMainCanvasUID),
		MEMBER(MemberType::GAME_OBJECT_UID, onimaruHealthMainCanvasUID),
		MEMBER(MemberType::GAME_OBJECT_UID, fangHealthSecondCanvasUID),
		MEMBER(MemberType::GAME_OBJECT_UID, onimaruHealthSecondCanvasUID),
		MEMBER(MemberType::GAME_OBJECT_UID, swapingSkillCanvasUID),
		MEMBER(MemberType::GAME_OBJECT_UID, fangUID),
		MEMBER(MemberType::GAME_OBJECT_UID, onimaruUID),
		MEMBER(MemberType::GAME_OBJECT_UID, scoreTextUID),
		MEMBER(MemberType::GAME_OBJECT_UID, canvasHUDUID),
		MEMBER(MemberType::GAME_OBJECT_UID, lowHealthWarningEffectUID),
		MEMBER(MemberType::FLOAT, timeToFadeDurableHealthFeedbackInternal),
		MEMBER(MemberType::FLOAT, delaySwitchTime)
};

GENERATE_BODY_IMPL(HUDController);

void HUDController::Start() {
	//Resetting preLives and remainingTimes just in case
	prevLivesFang = prevLivesOni = MAX_HEALTH;
	for (int i = 0; i < MAX_HEALTH; i++) {
		remainingDurableHealthTimesFang[i] = remainingDurableHealthTimesOni[i] = 0;
	}

	timeToFadeDurableHealthFeedback = timeToFadeDurableHealthFeedbackInternal;

	fangCanvas = GameplaySystems::GetGameObject(fangMainCanvasUID);
	onimaruCanvas = GameplaySystems::GetGameObject(onimaruMainCanvasUID);

	fangSkillsMainCanvas = GameplaySystems::GetGameObject(fangSkillsMainCanvasUID);
	onimaruSkillsMainCanvas = GameplaySystems::GetGameObject(onimaruSkillsMainCanvasUID);
	fangSkillsSecondCanvas = GameplaySystems::GetGameObject(fangSkillsSecondCanvasUID);
	onimaruSkillsSecondCanvas = GameplaySystems::GetGameObject(onimaruSkillsSecondCanvasUID);

	fangHealthMainCanvas = GameplaySystems::GetGameObject(fangHealthMainCanvasUID);
	onimaruHealthMainCanvas = GameplaySystems::GetGameObject(onimaruHealthMainCanvasUID);
	fangHealthSecondCanvas = GameplaySystems::GetGameObject(fangHealthSecondCanvasUID);
	onimaruHealthSecondCanvas = GameplaySystems::GetGameObject(onimaruHealthSecondCanvasUID);
	swapingSkillCanvas = GameplaySystems::GetGameObject(swapingSkillCanvasUID);

	fang = GameplaySystems::GetGameObject(fangUID);
	onimaru = GameplaySystems::GetGameObject(onimaruUID);

	lowHealthWarningEffect = GameplaySystems::GetGameObject(lowHealthWarningEffectUID);

	canvasHUD = GameplaySystems::GetGameObject(canvasHUDUID);

	GameObject* text = GameplaySystems::GetGameObject(scoreTextUID);
	if (text) scoreText = text->GetComponent<ComponentText>();

	if (onimaru && onimaruCanvas && fangCanvas && fang) {
		onimaru->Disable();
		onimaruCanvas->Disable();
		fangCanvas->Enable();
		fang->Enable();
	}

	if (fangHealthMainCanvas) {
		LoadHealthFeedbackStates(fangHealthMainCanvas, MAX_HEALTH);
	}

	for (int i = 0; i < static_cast<int>(Cooldowns::TOTAL); ++i) {
		cooldowns[i] = 0;
	}
}

void HUDController::Update() {
	if (!fangCanvas || !onimaruCanvas) return;

	// In order to NOT control the canvas switch from SwapCharacterDisplayerAnimation
	if (isSwitching) {
		if (currentTime > delaySwitchTime) {
			isSwitching = false;
			if (fang && !fang->IsActive()) {
				SetFangCanvas(false);
				SetOnimaruCanvas(true);
			} else {
				SetFangCanvas(true);
				SetOnimaruCanvas(false);
			}
		} else {
			currentTime += Time::GetDeltaTime();
		}
	}
}

void HUDController::StopHealthLostInstantEffects(GameObject* targetCanvas) {
	for (int i = 0; i < MAX_HEALTH; i++) {
		GameObject* obj = targetCanvas->GetChild(i);
		if (!obj) return;
		HealthLostInstantFeedback* fb = GET_SCRIPT(obj, HealthLostInstantFeedback);
		if (!fb) return;
		fb->Stop();
	}
}

void HUDController::LoadHealthFeedbackStates(GameObject* targetCanvas, int health) {
	float4 magentaToSet = colorMagenta;
	float4 whiteToSet = colorWhite;
	int i = 0;
	for (GameObject* hpGameObject : targetCanvas->GetChildren()) {
		if (hpGameObject->GetChildren().size() > 0) {
			ComponentImage* hpComponent = hpGameObject->GetChildren()[HIERARCHY_INDEX_MAIN_HEALTH]->GetComponent<ComponentImage>();
			if (hpComponent) {
				if (i < health) {
					hpComponent->SetColor(magentaToSet);
				} else {
					//If health is lower, set alpha to 0 so that effect may override if need be, but if not sets the health to "missing"
					hpComponent->SetColor(float4(whiteToSet.xyz(), 0.0f));
				}
			}
		}
		i++;
	}
}

void HUDController::LoadCooldownFeedbackStates(GameObject* canvas, int startingIndex) {
	if (canvas == nullptr) {
		return;
	}
	std::vector<GameObject*> skills = canvas->GetChildren();
	int skill = startingIndex;
	for (std::vector<GameObject*>::iterator it = skills.begin(); it != skills.end(); ++it) {

		ComponentImage* fillColor = nullptr;
		ComponentImage* imageColor = nullptr;
		if ((*it)->GetChildren().size() > HIERARCHY_INDEX_MAIN_BUTTON_UP) {
			//Cooldown does not return 1, meaning that it is ON Cooldown
			if (cooldowns[skill] < 1) {
				//Button Up is disabled, Button Down is enabled
				(*it)->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_UP)->Disable();
				(*it)->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_DOWN)->Enable();
			} else {

				//Button Up is enabled, Button Down is disabled
				(*it)->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_UP)->Enable();
				(*it)->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_DOWN)->Disable();
			}

			fillColor = (*it)->GetChildren()[HIERARCHY_INDEX_MAIN_ABILITY_FILL]->GetComponent<ComponentImage>();
			imageColor = (*it)->GetChildren()[HIERARCHY_INDEX_MAIN_ABILITY_IMAGE]->GetComponent<ComponentImage>();
			if (fillColor && imageColor) {

				if (cooldowns[skill] < 1) {
					fillColor->SetColor(float4(colorBlueForCD, 0.3f + cooldowns[skill]));
					imageColor->SetColor(colorWhite);
				} else {
					fillColor->SetColor(colorWhite);
					imageColor->SetColor(colorBlueBackground);
				}

				if (fillColor->IsFill()) {
					fillColor->SetFillValue(cooldowns[skill]);
				}
			}
		}

		++skill;
	}
}

void HUDController::ChangePlayerHUD(int fangLives, int oniLives) {
	if (!fang || !onimaru) return;
	currentTime = 0;
	SwapCharacterDisplayerAnimation* animationScript = GET_SCRIPT(canvasHUD, SwapCharacterDisplayerAnimation);
	if (animationScript) {
		animationScript->Play();
	}
	if (!fang->IsActive()) {
		StopHealthLostInstantEffects(onimaruHealthMainCanvas);
		LoadHealthFeedbackStates(onimaruHealthMainCanvas, oniLives);
		LoadCooldownFeedbackStates(onimaruSkillsMainCanvas, static_cast<int>(Cooldowns::ONIMARU_SKILL_1));
	} else {
		StopHealthLostInstantEffects(fangHealthMainCanvas);
		LoadHealthFeedbackStates(fangHealthMainCanvas, fangLives);
		LoadCooldownFeedbackStates(fangSkillsMainCanvas, static_cast<int>(Cooldowns::FANG_SKILL_1));
	}

	//remainingTimeActiveIndexesFang.clear();
	//remainingTimeActiveIndexesOni.clear();
	for (int i = 0; i < MAX_HEALTH; i++) {
		remainingDurableHealthTimesFang[i] = 0;
		remainingDurableHealthTimesOni[i] = 0;
	}
	abilityCoolDownsRetreived[static_cast<int>(Cooldowns::SWITCH_SKILL)] = false;

	prevLivesFang = fangLives;
	prevLivesOni = oniLives;
	isSwitching = true;
}

void HUDController::HealthRegeneration(float currentHp, float hpRecovered) {
	if (!onimaruHealthSecondCanvas || !fangHealthSecondCanvas) return;

	GameObject* targetCanvas = fang && fang->IsActive() ? onimaruHealthSecondCanvas : fangHealthSecondCanvas;
	const GameObject* healthSlot = targetCanvas->GetChildren()[currentHp];

	if (healthSlot) {
		for (GameObject* healthComponents : healthSlot->GetChildren()) {
			ComponentImage* healthFill = healthComponents->GetComponent<ComponentImage>();
			if (healthFill) {
				if (healthFill->IsFill()) {
					healthFill->SetFillValue(hpRecovered);
					if (hpRecovered >= 1.0f) {
						FullHealthBarFeedback* healthScript = GET_SCRIPT(healthSlot, FullHealthBarFeedback);
						if (healthScript) healthScript->Play();
					}
				}
			}
		}
	}
}

void HUDController::ResetHealthRegenerationEffects(float currentHp) {
	if (!onimaruHealthSecondCanvas || !fangHealthSecondCanvas) return;

	GameObject* targetCanvas = fang && fang->IsActive() ? onimaruHealthSecondCanvas : fangHealthSecondCanvas;

	//We need to stop last recovered health bar effect if the switch was made while it was playing
	//This if will change when you don't get Game Over if onimaru or fang has no health
	if (currentHp > 0) {
		const GameObject* healthSlot = targetCanvas->GetChild(currentHp - 1);
		if (healthSlot) {
			FullHealthBarFeedback* healthScript = GET_SCRIPT(healthSlot, FullHealthBarFeedback);
			if (healthScript) healthScript->Stop();
		}
	}

	for (int pos = currentHp; pos < MAX_HEALTH; ++pos) {
		if (pos < targetCanvas->GetChildren().size()) {
			const GameObject* healthSlot = targetCanvas->GetChildren()[pos];
			for (GameObject* healthComponents : healthSlot->GetChildren()) {
				ComponentImage* healthFill = healthComponents->GetComponent<ComponentImage>();
				if (healthFill) {
					if (healthFill->IsFill()) {
						healthFill->SetFillValue(0.0f);
						FullHealthBarFeedback* healthScript = GET_SCRIPT(healthSlot, FullHealthBarFeedback);
						if (healthScript) healthScript->Reset();
					}
				}
			}
		}
	}
}

void HUDController::ResetCooldownProgressBar() {
	if (!swapingSkillCanvas) {
		return;
	}
	GameObject* progressBarHoldingChild = swapingSkillCanvas->GetChild(HIERARCHY_INDEX_SWAP_ABILITY_EFFECT);
	if (progressBarHoldingChild) {
		AbilityRefreshEffectProgressBar* pef = GET_SCRIPT(progressBarHoldingChild, AbilityRefreshEffectProgressBar);
		if (pef != nullptr) {
			pef->ResetBar();
		}
	}

}

void HUDController::UpdateScore(int score_) {
	score += score_;
	if (scoreText) scoreText->SetText(std::to_string(score));
}

void HUDController::SetCooldownRetreival(Cooldowns cooldown) {
	abilityCoolDownsRetreived[static_cast<int>(cooldown)] = false;
}

float  HUDController::MapValue01(float value, float min, float max) {
	return (value - min) / (max - min);
}

void HUDController::SetFangCanvas(bool value) {
	if (!fangCanvas) {
		return;
	}

	if (value) {
		fangCanvas->Enable();
	} else {
		fangCanvas->Disable();
	}
}

void HUDController::SetOnimaruCanvas(bool value) {
	if (!onimaruCanvas) {
		return;
	}

	if (value) {
		onimaruCanvas->Enable();
	} else {
		onimaruCanvas->Disable();
	}
}

void HUDController::UpdateCooldowns(float onimaruCooldown1, float onimaruCooldown2, float onimaruCooldown3, float fangCooldown1, float fangCooldown2, float fangCooldown3, float switchCooldown) {
	//The received cooldowns here range from 0 to 1
	cooldowns[static_cast<int>(Cooldowns::FANG_SKILL_1)] = fangCooldown1;
	cooldowns[static_cast<int>(Cooldowns::FANG_SKILL_2)] = fangCooldown2;
	cooldowns[static_cast<int>(Cooldowns::FANG_SKILL_3)] = fangCooldown3;
	cooldowns[static_cast<int>(Cooldowns::ONIMARU_SKILL_1)] = onimaruCooldown1;
	cooldowns[static_cast<int>(Cooldowns::ONIMARU_SKILL_2)] = onimaruCooldown2;
	cooldowns[static_cast<int>(Cooldowns::ONIMARU_SKILL_3)] = onimaruCooldown3;
	cooldowns[static_cast<int>(Cooldowns::SWITCH_SKILL)] = switchCooldown;

	UpdateComponents();
}

void HUDController::UpdateHP(float currentHp, float altHp) {
	if (!fang || !onimaru) return;
	if (fang->IsActive()) {
		UpdateCanvasHP(fangHealthMainCanvas, currentHp, false);
	} else {
		UpdateCanvasHP(onimaruHealthMainCanvas, currentHp, false);
	}

	bool activateEffect = currentHp <= LOW_HEALTH_WARNING ? true : false;

	if (lowHealthWarningEffect) {
		if (activateEffect) {
			if (!lowHPWarningActive) {
				LowHPWarning* effect = GET_SCRIPT(lowHealthWarningEffect, LowHPWarning);
				effect->Play();
				lowHPWarningActive = true;
			}
		} else {
			if (lowHPWarningActive) {
				LowHPWarning* effect = GET_SCRIPT(lowHealthWarningEffect, LowHPWarning);
				effect->Stop();
				lowHPWarningActive = false;
			}
		}
	}
}

void HUDController::UpdateDurableHPLoss(GameObject* targetCanvas) {
	bool isFang = (targetCanvas->GetID() == fangHealthMainCanvas->GetID());
	if (isFang) {
		if (AnyTimerCounting(isFang)) {
			for (int i = 0; i < MAX_HEALTH; i++) {
				if (remainingDurableHealthTimesFang[i] > 0) {
					float delta = remainingDurableHealthTimesFang[i] / timeToFadeDurableHealthFeedback;


					ComponentImage* image = targetCanvas->GetChildren()[i]->GetChild(HIERARCHY_INDEX_MAIN_HEALTH)->GetComponent<ComponentImage>();
					if (image) {
						//We need access to image->SetAlphaTransparency to prevent possible errors
						if (delta < 0.5f) {

							//Remapping
							delta = (delta) / (0.5f);

							image->SetColor(float4(1.0f, 1.0f, 1.0f, delta));
						} else {
							image->SetColor(float4(1.0f, 1.0f, 1.0f, 1.0f));
						}
					}
					remainingDurableHealthTimesFang[i] -= Time::GetDeltaTime();
				}
			}
		}
	} else {
		if (AnyTimerCounting(isFang)) {
			for (int i = 0; i < MAX_HEALTH; i++) {
				if (remainingDurableHealthTimesOni[i] > 0) {
					float delta = remainingDurableHealthTimesOni[i] / timeToFadeDurableHealthFeedback;


					ComponentImage* image = targetCanvas->GetChildren()[i]->GetChild(HIERARCHY_INDEX_MAIN_HEALTH)->GetComponent<ComponentImage>();
					if (image) {
						//We need access to image->SetAlphaTransparency to prevent possible errors
						if (delta < 0.5f) {

							//Remapping
							delta = (delta) / (0.5f);

							image->SetColor(float4(1.0f, 1.0f, 1.0f, delta));
						} else {
							image->SetColor(float4(1.0f, 1.0f, 1.0f, 1.0f));
						}
					}
					remainingDurableHealthTimesOni[i] -= Time::GetDeltaTime();
				}
			}
		}
	}
}

void HUDController::UpdateComponents() {

	if (!fang || !onimaru) return;

	if (fangHealthMainCanvas) {
		if (fangHealthMainCanvas->IsActive()) {
			UpdateDurableHPLoss(fangHealthMainCanvas);
		}
	}

	if (onimaruHealthMainCanvas) {
		if (onimaruHealthMainCanvas->IsActive()) {
			UpdateDurableHPLoss(onimaruHealthMainCanvas);
		}
	}

	if (!fangSkillsMainCanvas || !onimaruSkillsMainCanvas || !fangSkillsSecondCanvas || !onimaruSkillsSecondCanvas) return;
	if (!fangHealthMainCanvas || !onimaruHealthMainCanvas || !fangHealthSecondCanvas || !onimaruHealthSecondCanvas) return;

	if (!swapingSkillCanvas) return;
	// Update all cooldowns
	if (fang->IsActive()) {
		UpdateVisualCooldowns(fangSkillsMainCanvas, true, static_cast<int>(Cooldowns::FANG_SKILL_1));
		UpdateVisualCooldowns(onimaruSkillsSecondCanvas, false, static_cast<int>(Cooldowns::ONIMARU_SKILL_1));
		UpdateCommonSkill();
	} else if (onimaru->IsActive()) {

		UpdateVisualCooldowns(onimaruSkillsMainCanvas, true, static_cast<int>(Cooldowns::ONIMARU_SKILL_1));
		UpdateVisualCooldowns(fangSkillsSecondCanvas, false, static_cast<int>(Cooldowns::FANG_SKILL_1));
		UpdateCommonSkill();
	}
}

void HUDController::UpdateCommonSkill() {
	if (!swapingSkillCanvas) {
		return;
	}

	std::vector<GameObject*> children = swapingSkillCanvas->GetChildren();
	if (children.size() > HIERARCHY_INDEX_SWAP_ABILITY_FILL - 1) {
		//Hardcoded value in hierarchy, changing the hierarchy without changing the index or the other way around will result in this not working properly
		ComponentImage* fillColor = children[HIERARCHY_INDEX_SWAP_ABILITY_FILL]->GetComponent<ComponentImage>();
		ComponentImage* image = children[HIERARCHY_INDEX_SWAP_ABILITY_IMAGE]->GetComponent<ComponentImage>();
		if (fillColor && image) {
			fillColor->SetFillValue(cooldowns[static_cast<int>(Cooldowns::SWITCH_SKILL)]);
			fillColor->SetColor(cooldowns[static_cast<int>(Cooldowns::SWITCH_SKILL)] < 1 ? float4(colorBlueForCD, 1.0f) : colorWhite);
			image->SetColor(cooldowns[static_cast<int>(Cooldowns::SWITCH_SKILL)] < 1 ? colorWhite : colorBlueBackground);

			AbilityCoolDownEffectCheck(Cooldowns::SWITCH_SKILL, swapingSkillCanvas);
		}

		if (cooldowns[static_cast<int>(Cooldowns::SWITCH_SKILL)] < 1) {
			if (children[HIERARCHY_INDEX_MAIN_BUTTON_UP]->IsActive()) {
				children[HIERARCHY_INDEX_MAIN_BUTTON_UP]->Disable();
				children[HIERARCHY_INDEX_MAIN_BUTTON_DOWN]->Enable();
			}
		}

	}
}

void HUDController::PlayCoolDownEffect(AbilityRefreshEffect* ef, Cooldowns cooldown) {
	if (ef != nullptr) {
		ef->Play();
	}
}

void HUDController::PlayProgressBarEffect(AbilityRefreshEffectProgressBar* ef, Cooldowns cooldown) {
	if (ef != nullptr) {
		ef->Play();
	}
}

bool HUDController::AnyTimerCounting(bool isFang) {
	if (isFang) {
		for (int i = 0; i < MAX_HEALTH; i++) {
			if (remainingDurableHealthTimesFang[i] > 0) {
				return true;
			}
		}
	} else {
		for (int i = 0; i < MAX_HEALTH; i++) {
			if (remainingDurableHealthTimesOni[i] > 0) {
				return true;
			}
		}
	}

	return false;
}

//Hierarchy sensitive method
void HUDController::AbilityCoolDownEffectCheck(Cooldowns cooldown, GameObject* canvas) {
	if (!abilityCoolDownsRetreived[static_cast<int>(cooldown)]) {
		if (cooldowns[static_cast<int>(cooldown)] == 1.0f) {
			if (canvas) {
				AbilityRefreshEffect* ef = nullptr;
				AbilityRefreshEffectProgressBar* pef = nullptr;

				if (cooldown < Cooldowns::ONIMARU_SKILL_1) {
					//Fang skill
					if (canvas->GetChildren().size() > 0) {
						if (canvas->GetChildren()[static_cast<int>(cooldown)]->GetChildren().size() > HIERARCHY_INDEX_MAIN_BUTTON_UP) {
							ef = GET_SCRIPT(canvas->GetChildren()[static_cast<int>(cooldown)]->GetChildren()[HIERARCHY_INDEX_MAIN_ABILITY_EFFECT], AbilityRefreshEffect);
							//Turn on button idle
							canvas->GetChildren()[static_cast<int>(cooldown)]->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_UP)->Enable();
							//Turn off button down
							canvas->GetChildren()[static_cast<int>(cooldown)]->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_DOWN)->Disable();
						}
					}
				} else if (cooldown < Cooldowns::SWITCH_SKILL) {
					//Onimaru skill
					if (canvas->GetChildren().size() > 0) {
						if (canvas->GetChildren()[static_cast<int>(cooldown) - 3]->GetChildren().size() > HIERARCHY_INDEX_MAIN_BUTTON_UP) {
							ef = GET_SCRIPT(canvas->GetChild(static_cast<int>(cooldown) - 3)->GetChildren()[HIERARCHY_INDEX_MAIN_ABILITY_EFFECT], AbilityRefreshEffect);
							//Turn on button idle
							canvas->GetChildren()[static_cast<int>(cooldown) - 3]->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_UP)->Enable();
							//Turn off button down
							canvas->GetChildren()[static_cast<int>(cooldown) - 3]->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_DOWN)->Disable();
						}
					}
				} else {
					if (canvas->GetChildren().size() > 6) {
						ef = GET_SCRIPT(canvas->GetChild(HIERARCHY_INDEX_SWAP_ABILITY_EFFECT), AbilityRefreshEffect);
						pef = GET_SCRIPT(canvas->GetChild(HIERARCHY_INDEX_SWAP_ABILITY_EFFECT), AbilityRefreshEffectProgressBar);
						//Turn on button idle
						canvas->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_UP)->Enable();
						//Turn off button down
						canvas->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_DOWN)->Disable();

					}
				}

				if (ef) {
					PlayCoolDownEffect(ef, cooldown);
					PlayProgressBarEffect(pef, cooldown);
					abilityCoolDownsRetreived[static_cast<int>(cooldown)] = true;
				}
			} else {
				//Canvas was passed as nullptr, which means that no effect must be played, only that the bool must be set
				abilityCoolDownsRetreived[static_cast<int>(cooldown)] = true;
			}
		}
	}
}

void HUDController::UpdateVisualCooldowns(GameObject* canvas, bool isMain, int startingIt) {

	std::vector<GameObject*> skills = canvas->GetChildren();
	int skill = startingIt;
	for (std::vector<GameObject*>::iterator it = skills.begin(); it != skills.end(); ++it) {

		ComponentImage* fillColor = nullptr;
		ComponentImage* imageColor = nullptr;
		if (isMain) {
			if ((*it)->GetChildren().size() > HIERARCHY_INDEX_MAIN_BUTTON_UP) {

				if (cooldowns[skill] < 1) {
					if ((*it)->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_UP)->IsActive()) {
						(*it)->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_UP)->Disable();
						(*it)->GetChild(HIERARCHY_INDEX_MAIN_BUTTON_DOWN)->Enable();
					}
				}

				fillColor = (*it)->GetChildren()[HIERARCHY_INDEX_MAIN_ABILITY_FILL]->GetComponent<ComponentImage>();
				imageColor = (*it)->GetChildren()[HIERARCHY_INDEX_MAIN_ABILITY_IMAGE]->GetComponent<ComponentImage>();
				if (fillColor && imageColor) {

					if (cooldowns[skill] < 1) {
						fillColor->SetColor(float4(colorBlueForCD, 0.3f + cooldowns[skill]));
						imageColor->SetColor(colorWhite);
					} else {
						fillColor->SetColor(colorWhite);
						imageColor->SetColor(colorBlueBackground);
					}

					if (fillColor->IsFill()) {
						fillColor->SetFillValue(cooldowns[skill]);
					}
				}
			}
		} else {
			if ((*it)->GetChildren().size() > 0) {
				fillColor = (*it)->GetChildren()[HIERARCHY_INDEX_SECONDARY_ABILITY_FILL]->GetComponent<ComponentImage>();
				if (fillColor) {
					if (fillColor->IsFill()) {
						fillColor->SetFillValue(cooldowns[skill]);
					}
				}
			}
		}

		AbilityCoolDownEffectCheck(static_cast<Cooldowns>(skill), isMain ? canvas : nullptr);


		++skill;
	}

}

void HUDController::OnHealthLost(GameObject* targetCanvas, int health) {
	bool isFang = (targetCanvas->GetID() == fangHealthMainCanvas->GetID());

	int prevLivesToUse = isFang ? prevLivesFang : prevLivesOni;
	//std::vector<int>& remainingTimeActiveIndexesToUse = isFang ? remainingTimeActiveIndexesFang : remainingTimeActiveIndexesOni;
	float* remainingDurableHealthTimesToUse = isFang ? remainingDurableHealthTimesFang : remainingDurableHealthTimesOni;

	int healthLost = prevLivesToUse - health;
	int healthToUse = health;

	for (int i = 0; i < healthLost; i++) {

		GameObject* obj = targetCanvas->GetChild(healthToUse);

		//remainingTimeActiveIndexesToUse.push_back(healthToUse);
		remainingDurableHealthTimesToUse[healthToUse] = timeToFadeDurableHealthFeedback;

		if (!obj) continue;

		HealthLostInstantFeedback* fb = GET_SCRIPT(obj, HealthLostInstantFeedback);

		if (!fb) continue;

		fb->Play();
		healthToUse++;
	}
}

void HUDController::UpdateCanvasHP(GameObject* targetCanvas, int health, bool darkened) {
	bool isFang = (targetCanvas->GetID() == fangHealthMainCanvas->GetID());

	if (isFang) {
		if (prevLivesFang > health) {
			OnHealthLost(targetCanvas, health);
			prevLivesFang = health;
		}
	} else {
		if (prevLivesOni > health) {
			OnHealthLost(targetCanvas, health);
			prevLivesOni = health;
		}
	}
}