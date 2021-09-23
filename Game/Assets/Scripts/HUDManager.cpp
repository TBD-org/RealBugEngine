#include "HUDManager.h"
#include "PlayerController.h";
#include "GameController.h"
#include "Components/UI/ComponentTransform2D.h"
#include "Components/UI/ComponentImage.h"
#include "ImageColorFader.h"
#include "AbilityRefeshFX.h"

#define HIERARCHY_INDEX_ABILITY_FILL 1
#define HIERARCHY_INDEX_ABILITY_DURATION_FILL 2
#define HIERARCHY_INDEX_ABILITY_EFFECT 3
#define HIERARCHY_INDEX_ABILITY_PICTO_SHADE 5
#define HIERARCHY_INDEX_ABILITY_KEY_FILL 6


#define HIERARCHY_INDEX_OTHER_ULTIMATE_ABILITY_OTHER_ULT_FILL 4
#define HIERARCHY_INDEX_OTHER_ULTIMATE_ABILITY_FILL 5
#define HIERARCHY_INDEX_ULTIMATE_ABILITY_PICTO_SHADE 6
#define HIERARCHY_INDEX_ULTIMATE_ABILITY_KEY_FILL 7


#define HIERARCHY_INDEX_ULTIMATE_ABILITY_DECOR_FILL 8

#define HIERARCHY_INDEX_SWITCH_ABILITY_GREEN_EFFECT 0
#define HIERARCHY_INDEX_SWITCH_ABILITY_FILL 2
#define HIERARCHY_INDEX_SWITCH_ABILITY_DURATION_FILL 3
#define HIERARCHY_INDEX_SWITCH_ABILITY_IN_USE_WHITE 4
#define HIERARCHY_INDEX_SWITCH_ABILITY_IN_USE_GLOW 5
#define HIERARCHY_INDEX_SWITCH_ABILITY_EFFECT 6
#define HIERARCHY_INDEX_SWITCH_ABILITY_PICTO_SHADE 8
#define HIERARCHY_INDEX_SWITCH_ABILITY_KEY_FILL 9

#define SWITCH_SKILL_HIERARCHY_NUM_CHILDREN 10

#define HIERARCHY_INDEX_HEALTH_BACKGROUND 0
#define HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK 1
#define HIERARCHY_INDEX_HEALTH_FILL 2
#define HIERARCHY_INDEX_HEALTH_OVERLAY 3

#define HIERARCHY_INDEX_SWITCH_HEALTH_STROKE 0
#define HIERARCHY_INDEX_SWITCH_HEALTH_FILL 1

#define HIERARCHY_INDEX_HUD_LEFT_SIDE 0
#define HIERARCHY_INDEX_HUD_RIGHT_SIDE 1

#define HEALTH_HIERARCHY_NUM_CHILDREN 4
#define HUD_HIT_FEEDBACK_SIDES 2
#define SWITCH_HEALTH_HIERARCHY_NUM_CHILDREN 2

EXPOSE_MEMBERS(HUDManager) {
	MEMBER(MemberType::GAME_OBJECT_UID, playerObjectUID),
	MEMBER_SEPARATOR("HUD Abilities"),
	MEMBER(MemberType::GAME_OBJECT_UID, fangSkillParentUID),
	MEMBER(MemberType::GAME_OBJECT_UID, onimaruSkillParentUID),
	MEMBER(MemberType::GAME_OBJECT_UID, switchSkillParentUID),
	MEMBER_SEPARATOR("HUD Health"),
	MEMBER(MemberType::GAME_OBJECT_UID, fangHealthParentUID),
	MEMBER(MemberType::GAME_OBJECT_UID, onimaruHealthParentUID),
	MEMBER(MemberType::GAME_OBJECT_UID, switchHealthParentUID),
	MEMBER(MemberType::FLOAT, lostHealthFeedbackAlpha),
	MEMBER_SEPARATOR("HUD Sides"),
	MEMBER(MemberType::GAME_OBJECT_UID, sidesHUDParentUID),
	MEMBER(MemberType::FLOAT, criticalHealthPercentage),
	MEMBER(MemberType::STRING, shieldObjName)
};

GENERATE_BODY_IMPL(HUDManager);

void HUDManager::Start() {
	GameObject* playerControllerObj = GameplaySystems::GetGameObject(playerObjectUID);
	if (playerControllerObj) {
		playerController = GET_SCRIPT(playerControllerObj, PlayerController);

		onimaruObj = playerControllerObj->GetChild("Onimaru");
		fangObj = playerControllerObj->GetChild("Fang");
	}
	fangSkillParent = GameplaySystems::GetGameObject(fangSkillParentUID);
	onimaruSkillParent = GameplaySystems::GetGameObject(onimaruSkillParentUID);
	switchSkillParent = GameplaySystems::GetGameObject(switchSkillParentUID);
	switchSkillActivated = false;

	if (fangSkillParent && onimaruSkillParent && switchSkillParent) {
		skillsFang = fangSkillParent->GetChildren();
		skillsOni = onimaruSkillParent->GetChildren();
		switchSkillParent->Disable();

		//Vector used later to avoid a flicker on first swtich
		std::vector<ComponentTransform2D*>oniTransforms;

		for (int i = 0; i < static_cast<int>(Cooldowns::TOTAL); i++) {
			ComponentTransform2D* transform2D = nullptr;
			if (i < static_cast<int>(Cooldowns::ONIMARU_SKILL_1)) {
				//Fang skill
				transform2D = skillsFang[i]->GetComponent<ComponentTransform2D>();

			} else if (i != static_cast<int>(Cooldowns::SWITCH_SKILL)) {
				//Onimaru skill
				transform2D = skillsOni[i - 3]->GetComponent<ComponentTransform2D>();
				if (transform2D) {
					oniTransforms.push_back(transform2D);
				}
			} else {
				transform2D = switchSkillParent->GetComponent<ComponentTransform2D>();
			}

			if (transform2D) {
				cooldownTransformOriginalPositions[i] = transform2D->GetPosition();
			}
		}

		//Set onimaru abilities position to the collapsed position so as to avoid a flicker on first swtich
		for (int i = 0; i < oniTransforms.size(); i++) {
			oniTransforms[i]->SetPosition(cooldownTransformOriginalPositions[static_cast<int>(Cooldowns::SWITCH_SKILL)]);
		}

		GameObject* pictoShadeObj = switchSkillParent->GetChild("PictoShade");

		if (pictoShadeObj) {
			switchShadeTransform = pictoShadeObj->GetComponent<ComponentTransform2D>();
		}


		if (switchSkillParent) {
			std::vector<GameObject*> switchChildren = switchSkillParent->GetChildren();

			if (switchChildren.size() != SWITCH_SKILL_HIERARCHY_NUM_CHILDREN) return;

			switchChildren[HIERARCHY_INDEX_SWITCH_ABILITY_IN_USE_WHITE]->Disable();
			switchChildren[HIERARCHY_INDEX_SWITCH_ABILITY_IN_USE_GLOW]->Enable();
			switchGlowImage = switchChildren[HIERARCHY_INDEX_SWITCH_ABILITY_IN_USE_GLOW]->GetComponent<ComponentImage>();
			if (switchGlowImage) {
				switchGlowImage->SetColor(float4(1.0f, 1.0f, 1.0f, 0.0f));
			}
			switchChildren[HIERARCHY_INDEX_SWITCH_ABILITY_GREEN_EFFECT]->Disable();

		}

		onimaruSkillParent->Disable();

		int i = 0;
		for (ComponentAudioSource& src : GetOwner().GetParent()->GetComponents<ComponentAudioSource>()) {
			if (i < static_cast<int>(HUDManagerAudio::TOTAL)) audios[i] = &src;
			++i;
		}

	}

	fangHealthParent = GameplaySystems::GetGameObject(fangHealthParentUID);
	onimaruHealthParent = GameplaySystems::GetGameObject(onimaruHealthParentUID);
	switchHealthParent = GameplaySystems::GetGameObject(switchHealthParentUID);

	if (fangHealthParent && onimaruHealthParent) {
		ComponentTransform2D* pos = nullptr;
		pos = fangHealthParent->GetComponent<ComponentTransform2D>();
		if (pos) originalFangHealthPosition = pos->GetPosition();
		pos = nullptr;
		pos = onimaruHealthParent->GetComponent<ComponentTransform2D>();
		if (pos) originalOnimaruHealthPosition = pos->GetPosition();
		fangHealthChildren = fangHealthParent->GetChildren();
		onimaruHealthChildren = onimaruHealthParent->GetChildren();

		GetAllHealthColors();
		InitializeHealth();
	}

	if (switchHealthParent) {
		switchHealthChildren = switchHealthParent->GetChildren();

		if (switchHealthChildren.size() == SWITCH_HEALTH_HIERARCHY_NUM_CHILDREN) {
			ComponentTransform2D* transform = switchHealthChildren[HIERARCHY_INDEX_SWITCH_HEALTH_STROKE]->GetComponent<ComponentTransform2D>();
			if (transform) originalStrokeSize = transform->GetSize();
			transform = switchHealthChildren[HIERARCHY_INDEX_SWITCH_HEALTH_FILL]->GetComponent<ComponentTransform2D>();
			if (transform) originalFillPos = transform->GetPosition();
		}
	}

	sidesHUDParent = GameplaySystems::GetGameObject(sidesHUDParentUID);

	if (sidesHUDParent) {
		sidesHUDChildren = sidesHUDParent->GetChildren();
		InitializeHUDSides();
	}
}

void HUDManager::Update() {
	// This checks for when the Switch tutorial is reached.When this happens, switch is activated and Player will be able to switch from then on.
	if (GameController::IsSwitchTutorialReached() && !switchSkillActivated) {
		if (switchSkillParent) {
			if (!switchSkillParent->IsActive()) {
				switchSkillParent->Enable();
				switchSkillActivated = true;
			}
		}
	}

	ManageSwitch();
	if (playingLostHealthFeedback) PlayLostHealthFeedback();
	if (playingHitEffect) PlayHitEffect();
}

void HUDManager::UpdateCooldowns(float onimaruCooldown1, float onimaruCooldown2, float onimaruCooldown3, float fangCooldown1, float fangCooldown2, float fangCooldown3, float switchCooldown, float fangUltimateRemainingNormalizedValue, float oniUltimateRemainingNormalizedValue) {
	cooldowns[static_cast<int>(Cooldowns::FANG_SKILL_1)] = fangCooldown1;
	cooldowns[static_cast<int>(Cooldowns::FANG_SKILL_2)] = fangCooldown2;
	cooldowns[static_cast<int>(Cooldowns::FANG_SKILL_3)] = fangUltimateRemainingNormalizedValue == 0 ? fangCooldown3 : 0.99f;
	cooldowns[static_cast<int>(Cooldowns::ONIMARU_SKILL_1)] = onimaruCooldown1;
	cooldowns[static_cast<int>(Cooldowns::ONIMARU_SKILL_2)] = onimaruCooldown2;
	cooldowns[static_cast<int>(Cooldowns::ONIMARU_SKILL_3)] = oniUltimateRemainingNormalizedValue == 0 ? onimaruCooldown3 : 0.99f;
	cooldowns[static_cast<int>(Cooldowns::SWITCH_SKILL)] = switchCooldown;

	if (onimaruObj && fangObj && fangSkillParent && onimaruSkillParent) {

		if (fangObj->IsActive()) {
			UpdateVisualCooldowns(fangSkillParent, static_cast<int>(Cooldowns::FANG_SKILL_1));
			SetRemainingDurationNormalizedValue(fangSkillParent, static_cast<int>(Cooldowns::FANG_SKILL_3), fangUltimateRemainingNormalizedValue);
		} else {
			UpdateVisualCooldowns(onimaruSkillParent, static_cast<int>(Cooldowns::ONIMARU_SKILL_1));
			SetRemainingDurationNormalizedValue(onimaruSkillParent, static_cast<int>(Cooldowns::ONIMARU_SKILL_3) - 3, oniUltimateRemainingNormalizedValue);
		}
	}
	UpdateCommonSkillVisualCooldown();
}

void HUDManager::UpdateHealth(float fangHealth, float onimaruHealth) {
	if (!fangObj || !onimaruObj || !playerController) return;
	if (fangHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN || onimaruHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN) return;

	if (switchState == SwitchState::IDLE) StartLostHealthFeedback(); // Temporary hack

	float health = fangObj->IsActive() ? fangHealth : onimaruHealth;
	float maxHealth = fangObj->IsActive() ? playerController->GetFangMaxHealth() : playerController->GetOnimaruMaxHealth();

	ComponentImage* healthFill = nullptr;
	healthFill = fangObj->IsActive() ? fangHealthChildren[HIERARCHY_INDEX_HEALTH_FILL]->GetComponent<ComponentImage>() : onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_FILL]->GetComponent<ComponentImage>();
	if (healthFill) {
		if (healthFill->IsFill()) {
			healthFill->SetFillValue(health / maxHealth);
		}
	}

	if (fangObj->IsActive()) fangPreviousHealth = fangHealth;
	else onimaruPreviousHealth = onimaruHealth;

	if (switchState != SwitchState::IDLE) ResetLostHealthFeedback();

	if (!criticalHealthWarning && health <= maxHealth * (criticalHealthPercentage / 100.f)) ShowCriticalHealthWarning();

	playingHitEffect = true;
	hitEffectTimer = 0.0f;
}

void HUDManager::HealthRegeneration(float health) {
	if (!fangObj || !onimaruObj || !playerController) return;
	if (fangHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN || onimaruHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN) return;

	float maxHealth = fangObj->IsActive() ? playerController->GetOnimaruMaxHealth() : playerController->GetFangMaxHealth();

	// Set current health
	ComponentImage* healthFill = nullptr;
	healthFill = fangObj->IsActive() ? onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_FILL]->GetComponent<ComponentImage>() : fangHealthChildren[HIERARCHY_INDEX_HEALTH_FILL]->GetComponent<ComponentImage>();
	if (healthFill) {
		if (healthFill->IsFill()) {
			healthFill->SetFillValue(health / maxHealth);
		}
	}

	// Set health lost fill bar to current health
	ComponentImage* healthLost = nullptr;
	healthLost = fangObj->IsActive() ? onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>() : fangHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>();
	if (healthLost) {
		if (healthLost->IsFill()) {
			healthLost->SetFillValue(health / maxHealth);
		}
	}

	if (fangObj->IsActive()) onimaruPreviousHealth = health;
	else fangPreviousHealth = health;
}

void HUDManager::StartCharacterSwitch() {
	if (!fangObj || !playerController) return;
	switchTimer = 0;
	switchState = SwitchState::PRE_COLLAPSE;
	if (switchSkillParent) {
		std::vector<GameObject*> switchChildren = switchSkillParent->GetChildren();
		if (switchChildren.size() > HIERARCHY_INDEX_SWITCH_ABILITY_KEY_FILL - 1) {
			if (switchGlowImage) {
				if (playerController && playerController->AreBothCharactersAlive()) {
					switchGlowImage->SetColor(float4(1.0f, 1.0f, 1.0f, 1.0f));
					switchChildren[HIERARCHY_INDEX_SWITCH_ABILITY_IN_USE_WHITE]->Enable();
					switchChildren[HIERARCHY_INDEX_SWITCH_ABILITY_IN_USE_GLOW]->Enable();
				}
			}

			switchChildren[HIERARCHY_INDEX_SWITCH_ABILITY_GREEN_EFFECT]->Enable();
		}
	}
	if (playingLostHealthFeedback) StopLostHealthFeedback();

	// Check if the new character needs the health warning

	float health = fangObj->IsActive() ? onimaruPreviousHealth : fangPreviousHealth;
	float maxHealth = fangObj->IsActive() ? playerController->GetOnimaruMaxHealth() : playerController->GetFangMaxHealth();

	if (health > maxHealth * (criticalHealthPercentage / 100.f)) {
		if (criticalHealthWarning) HideCriticalHealthWarning();
	} else if (!criticalHealthWarning) ShowCriticalHealthWarning();
}

void HUDManager::SetCooldownRetreival(Cooldowns cooldown) {
	abilityCoolDownsRetreived[static_cast<int>(cooldown)] = false;
	if (cooldown != Cooldowns::SWITCH_SKILL)
		SetPictoState(cooldown, PictoState::UNAVAILABLE);
}

void HUDManager::StartUsingSkill(Cooldowns cooldown) {
	SetPictoState(cooldown, PictoState::IN_USE);
}

void HUDManager::StopUsingSkill(Cooldowns cooldown) {
	if (cooldown != Cooldowns::SWITCH_SKILL)
		SetPictoState(cooldown, cooldowns[static_cast<int>(cooldown)] < 1.0f ? PictoState::UNAVAILABLE : PictoState::AVAILABLE);
}

void HUDManager::OnCharacterDeath() {
	abilityCoolDownsRetreived[static_cast<int>(Cooldowns::SWITCH_SKILL)] = false;
}

void HUDManager::OnCharacterResurrect() {

}

void HUDManager::UpdateVisualCooldowns(GameObject* canvas, int startingIt) {

	std::vector<GameObject*> skills = canvas->GetChildren();
	//Skills size is supposed to be 3, for each character has 3 skills
	if (skills.size() == 3) {
		int ultCount = 0;
		int skill = startingIt;
		for (std::vector<GameObject*>::iterator it = skills.begin(); it != skills.end(); ++it) {

			ComponentImage* fillImage = nullptr;
			ComponentImage* pictogramImage = nullptr;
			ComponentText* text = nullptr;
			ComponentImage* textFill = nullptr;

			std::vector<GameObject*> children = (*it)->GetChildren();

			if (children.size() < HIERARCHY_INDEX_ABILITY_KEY_FILL) return;

			fillImage = children[HIERARCHY_INDEX_ABILITY_FILL]->GetComponent<ComponentImage>();

			if (ultCount != 2) {
				pictogramImage = (*it)->GetChildren()[HIERARCHY_INDEX_ABILITY_PICTO_SHADE]->GetComponent<ComponentImage>();
				textFill = children[HIERARCHY_INDEX_ABILITY_KEY_FILL]->GetComponent<ComponentImage>();
			} else {

				if (children.size() < HIERARCHY_INDEX_ULTIMATE_ABILITY_DECOR_FILL) return;

				pictogramImage = (*it)->GetChildren()[HIERARCHY_INDEX_ULTIMATE_ABILITY_PICTO_SHADE]->GetComponent<ComponentImage>();
				textFill = children[HIERARCHY_INDEX_ULTIMATE_ABILITY_KEY_FILL]->GetComponent<ComponentImage>();
			}

			if (fillImage && pictogramImage) {

				if (cooldowns[skill] < 1) {
					//On Cooldown
					fillImage->SetColor(float4(skillColorNotAvailable.xyz(), 0.3f + cooldowns[skill]));
				} else {
					//Available
					fillImage->SetColor(skillColorAvailable);
				}

				if (fillImage->IsFill()) {
					fillImage->SetFillValue(cooldowns[skill]);
				}
			}

			if (textFill) {
				textFill->SetColor(cooldowns[skill] < 1 ? buttonColorNotAvailable : buttonColorAvailable);
				text = textFill->GetOwner().GetChild(1)->GetComponent<ComponentText>();
				if (text) {
					text->SetFontColor(cooldowns[skill] < 1 ? buttonTextColorNotAvailable : buttonTextColorAvailable);
				}
			}

			AbilityCoolDownEffectCheck(static_cast<Cooldowns>(skill), canvas);

			++skill;
			++ultCount;
		}

		if (skills[2]) {
			if (cooldowns[startingIt + 2] < 1.0f) {
				GameObject* decorFill = skills[2]->GetChildren()[HIERARCHY_INDEX_ULTIMATE_ABILITY_DECOR_FILL];
				if (decorFill) {
					decorFill->Disable();
				}
			}
		}

		GameObject* otherUltFillObj = skills[2]->GetChildren()[HIERARCHY_INDEX_OTHER_ULTIMATE_ABILITY_OTHER_ULT_FILL];

		if (otherUltFillObj) {
			ComponentImage* otherUltFill = otherUltFillObj->GetComponent<ComponentImage>();
			if (otherUltFill) {
				if (startingIt == 0) {
					//Fang must fill with onimaru cd value
					otherUltFill->SetFillValue(cooldowns[static_cast<int>(Cooldowns::ONIMARU_SKILL_3)]);
				} else {
					//Onimaru must fill with fang cd value
					otherUltFill->SetFillValue(cooldowns[static_cast<int>(Cooldowns::FANG_SKILL_3)]);
				}
			}
		}

	}
}


void HUDManager::SetRemainingDurationNormalizedValue(GameObject* canvas, int index, float normalizedValue) {
	if (!canvas)return;
	std::vector<GameObject*> children = canvas->GetChildren();
	if (children.size() <= index) return;

	GameObject* fillHolder = children[index]->GetChild(HIERARCHY_INDEX_ABILITY_DURATION_FILL);
	if (fillHolder) {
		ComponentImage* fill = fillHolder->GetComponent<ComponentImage>();
		if (fill) {
			fill->SetFillValue(normalizedValue);
		}
	}
}


//Hierarchy sensitive method
void HUDManager::AbilityCoolDownEffectCheck(Cooldowns cooldown, GameObject* canvas) {

	if (!abilityCoolDownsRetreived[static_cast<int>(cooldown)]) {
		if (cooldowns[static_cast<int>(cooldown)] == 1.0f) {
			if (canvas) {
				AbilityRefeshFX* ef = nullptr;
				std::vector<GameObject*> skills = canvas->GetChildren();

				if (skills.size() < 2) return;

				if (cooldown < Cooldowns::ONIMARU_SKILL_1) {
					//Fang skill

					std::vector<GameObject*>children = skills[static_cast<int>(cooldown)]->GetChildren();

					if (children.size() < HIERARCHY_INDEX_ABILITY_EFFECT) return;

					ef = GET_SCRIPT(children[HIERARCHY_INDEX_ABILITY_EFFECT], AbilityRefeshFX);

				} else if (cooldown < Cooldowns::SWITCH_SKILL) {
					//Onimaru skill
					std::vector<GameObject*>children = skills[static_cast<int>(cooldown) - 3]->GetChildren();

					if (children.size() < HIERARCHY_INDEX_ABILITY_EFFECT) return;

					if (cooldown == Cooldowns::ONIMARU_SKILL_1) {
						if (onimaruObj) {
							GameObject* shieldObj = onimaruObj->GetChild(shieldObjName.c_str());
							if (shieldObj) {
								if (!shieldObj->IsActive()) {
									ef = GET_SCRIPT(children[HIERARCHY_INDEX_ABILITY_EFFECT], AbilityRefeshFX);
								}
							}
						}
					} else {
						ef = GET_SCRIPT(children[HIERARCHY_INDEX_ABILITY_EFFECT], AbilityRefeshFX);
					}
				} else {
					//Switch skill

					if (playerController) {
						if (playerController->AreBothCharactersAlive()) {
							ef = GET_SCRIPT(skills[HIERARCHY_INDEX_SWITCH_ABILITY_EFFECT], AbilityRefeshFX);
						}
					}
				}

				if (ef) {
					PlayCoolDownEffect(ef, cooldown);

					if (cooldown == Cooldowns::FANG_SKILL_3 || cooldown == Cooldowns::ONIMARU_SKILL_3) {

						GameObject* ultiDecoFill = nullptr;

						if (cooldown == Cooldowns::FANG_SKILL_3) {
							std::vector<GameObject*>children = skills[static_cast<int>(cooldown)]->GetChildren();
							if (children.size() < HIERARCHY_INDEX_ULTIMATE_ABILITY_DECOR_FILL)return;
							ultiDecoFill = children[HIERARCHY_INDEX_ULTIMATE_ABILITY_DECOR_FILL];
						} else {
							std::vector<GameObject*>children = skills[static_cast<int>(cooldown) - 3]->GetChildren();
							if (children.size() < HIERARCHY_INDEX_ULTIMATE_ABILITY_DECOR_FILL)return;
							ultiDecoFill = children[HIERARCHY_INDEX_ULTIMATE_ABILITY_DECOR_FILL];
						}

						if (ultiDecoFill) {
							ImageColorFader* colorFader = GET_SCRIPT(ultiDecoFill, ImageColorFader);
							if (colorFader) {
								colorFader->Play();
							}
						}
					}

					abilityCoolDownsRetreived[static_cast<int>(cooldown)] = true;
					SetPictoState(cooldown, PictoState::AVAILABLE);
				}
			} else {
				//Canvas was passed as nullptr, which means that no effect must be played, only that the bool must be set
				abilityCoolDownsRetreived[static_cast<int>(cooldown)] = true;
				SetPictoState(cooldown, PictoState::AVAILABLE);
			}
		}
	}
}

void HUDManager::UpdateCommonSkillVisualCooldown() {
	if (!switchSkillParent) return;


	std::vector<GameObject*> children = switchSkillParent->GetChildren();

	if (children.size() != SWITCH_SKILL_HIERARCHY_NUM_CHILDREN) return;

	ComponentImage* fillColor = children[HIERARCHY_INDEX_SWITCH_ABILITY_FILL]->GetComponent<ComponentImage>();
	ComponentImage* image = children[HIERARCHY_INDEX_SWITCH_ABILITY_PICTO_SHADE]->GetComponent<ComponentImage>();

	GameObject* textParent = children[HIERARCHY_INDEX_SWITCH_ABILITY_KEY_FILL];
	ComponentText* text = nullptr;

	if (textParent) {
		if (textParent->GetChildren().size() > 1)
			text = textParent->GetChild(1)->GetComponent<ComponentText>();
	}

	ComponentImage* textFill = children[HIERARCHY_INDEX_SWITCH_ABILITY_KEY_FILL]->GetComponent<ComponentImage>();

	if (fillColor && image) {
		fillColor->SetFillValue(cooldowns[static_cast<int>(Cooldowns::SWITCH_SKILL)]);
		if (playerController) {
			if (playerController->AreBothCharactersAlive()) {
				fillColor->SetColor(cooldowns[static_cast<int>(Cooldowns::SWITCH_SKILL)] < 1 ? float4(switchSkillColorNotAvailable.xyz(), 0.3f + cooldowns[static_cast<int>(Cooldowns::SWITCH_SKILL)]) : switchSkillColorAvailable);
			} else {
				fillColor->SetColor(switchSkillColorDeadCharacter);
			}
		}
	}

	AbilityCoolDownEffectCheck(Cooldowns::SWITCH_SKILL, switchSkillParent);

	if (text) {
		text->SetFontColor(cooldowns[static_cast<int>(Cooldowns::SWITCH_SKILL)] < 1 ? buttonTextColorNotAvailable : buttonTextColorAvailable);
	}

	if (textFill) {
		textFill->SetColor(cooldowns[static_cast<int>(Cooldowns::SWITCH_SKILL)] < 1 ? buttonColorNotAvailable : buttonColorAvailable);
	}

}

void HUDManager::ManageSwitch() {
	if (!fangSkillParent || !onimaruSkillParent || !switchSkillParent || !fangHealthParent || !onimaruHealthParent || !switchHealthParent || !fangObj || !onimaruObj) return;
	if (skillsFang.size() != 3 || skillsOni.size() != 3) return;
	if (fangHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN || onimaruHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN) return;
	if (switchHealthChildren.size() != SWITCH_HEALTH_HIERARCHY_NUM_CHILDREN) return;

	ComponentTransform2D* transform2D = nullptr;
	ComponentTransform2D* health = nullptr;
	ComponentImage* backgroundImage = nullptr;
	ComponentImage* fillImage = nullptr;
	ComponentImage* overlayImage = nullptr;
	ComponentText* healthText = nullptr;
	ComponentImage* switchBarFillImage = nullptr;
	ComponentImage* switchHealthStroke = nullptr;
	ComponentTransform2D* switchHealthStrokeTransform2D = nullptr;
	ComponentTransform2D* switchHealthFillTransform2D = nullptr;

	if (switchShadeTransform) {
		if ((switchState != SwitchState::IDLE || cooldowns[static_cast<int>(Cooldowns::SWITCH_SKILL)] >= 1.0f) && playerController && playerController->AreBothCharactersAlive()) {
			Quat rotToAdd;
			rotToAdd.SetFromAxisAngle(float4(0, 0, 1, 1), Time::GetDeltaTime() * rotationSpeed);
			switchShadeTransform->SetRotation(rotToAdd * switchShadeTransform->GetGlobalRotation());
		}
	}

	switch (switchState) {
	case SwitchState::IDLE:
		//This code handles the color grading progressively increasing and decreasing alpha
		if (switchSkillParent) {
			std::vector<GameObject*> children = switchSkillParent->GetChildren();
			ComponentImage* fillImage = children[HIERARCHY_INDEX_SWITCH_ABILITY_FILL]->GetComponent<ComponentImage>();
			if (fillImage) {

				float delta = switchColorTimer / switchColorTotalTime;

				if (switchColorIncreasing) {
					fillImage->SetColor(float4(fillImage->GetColor().xyz(), Lerp(0.3f, 0.7f, delta)));
				} else {
					fillImage->SetColor(float4(fillImage->GetColor().xyz(), Lerp(0.7f, 0.3f, delta)));
				}
				switchColorTimer += Time::GetDeltaTime();
			}
		}

		//Reset color timer and invert the toggle for increasing/decreasing
		if (switchColorTimer >= switchColorTotalTime) {
			switchColorTimer = 0;
			switchColorIncreasing = !switchColorIncreasing;
		}

		break;
	case SwitchState::PRE_COLLAPSE:

		if (switchTimer > switchPreCollapseMovementTime) {
			switchTimer = switchPreCollapseMovementTime;
		}

		//Glow should only update if both characters are alive
		if (switchGlowImage) {
			if (playerController && playerController->AreBothCharactersAlive()) {
				switchGlowImage->SetColor(float4(1.0f, 1.0f, 1.0f, 1 - Clamp(switchTimer / switchPreCollapseMovementTime, 0.0f, 0.7f)));
			}
		}

		ManageSwitchPreCollapseState(fangSkillParent->IsActive() ? fangSkillParent : onimaruSkillParent, fangSkillParent->IsActive() ? skillsFang : skillsOni);
		ManageSwitchGreenEffect(true, switchPreCollapseMovementTime);



		if (switchTimer == switchPreCollapseMovementTime) {

			//Glow should only update if both characters are alive
			if (switchGlowImage) {
				if (playerController && playerController->AreBothCharactersAlive()) {
					switchGlowImage->SetColor(float4(1.0f, 1.0f, 1.0f, 0));
				}
			}
			switchState = SwitchState::COLLAPSE;
			switchTimer = 0;
		}

		break;
	case SwitchState::COLLAPSE:
		SetCooldownRetreival(Cooldowns::SWITCH_SKILL);

		if (switchTimer > switchCollapseMovementTime) {
			switchTimer = switchCollapseMovementTime;
		}

		//Glow should only update if both characters are alive
		if (switchGlowImage) {
			if (playerController && playerController->AreBothCharactersAlive()) {
				switchGlowImage->SetColor(float4(1.0f, 1.0f, 1.0f, 1 - Clamp01(0.7f + switchTimer / switchPreCollapseMovementTime)));
			}
		}

		ManageSwitchCollapseState(fangSkillParent->IsActive() ? fangSkillParent : onimaruSkillParent, fangSkillParent->IsActive() ? skillsFang : skillsOni);
		ManageSwitchGreenEffect(false, switchCollapseMovementTime);

		//Health handling

		if (fangObj->IsActive()) {
			health = onimaruHealthParent->GetComponent<ComponentTransform2D>();
			backgroundImage = onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_BACKGROUND]->GetComponent<ComponentImage>();
			fillImage = onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_FILL]->GetComponent<ComponentImage>();
			overlayImage = onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_OVERLAY]->GetComponent<ComponentImage>();

			if (health) {
				health->SetPosition(float3::Lerp(originalOnimaruHealthPosition + float3(healthOffset, 0, 0), originalOnimaruHealthPosition, switchTimer / switchCollapseMovementTime));
			}
		} else {
			health = fangHealthParent->GetComponent<ComponentTransform2D>();
			backgroundImage = fangHealthChildren[HIERARCHY_INDEX_HEALTH_BACKGROUND]->GetComponent<ComponentImage>();
			fillImage = fangHealthChildren[HIERARCHY_INDEX_HEALTH_FILL]->GetComponent<ComponentImage>();
			overlayImage = fangHealthChildren[HIERARCHY_INDEX_HEALTH_OVERLAY]->GetComponent<ComponentImage>();

			if (health) {
				health->SetPosition(float3::Lerp(originalFangHealthPosition, originalFangHealthPosition - float3(healthOffset, 0, 0), switchTimer / switchCollapseMovementTime));
			}
		}

		switchHealthStroke = switchHealthChildren[HIERARCHY_INDEX_SWITCH_HEALTH_STROKE]->GetComponent<ComponentImage>();


		if (backgroundImage) {
			backgroundImage->SetColor(float4::Lerp(healthBarBackgroundColor, healthBarBackgroundColorInBackground, switchTimer / switchCollapseMovementTime));
		}

		if (fillImage) {
			fillImage->SetColor(float4::Lerp(healthFillBarColor, healthFillBarColorInBackground, switchTimer / switchCollapseMovementTime));
		}

		if (overlayImage) {
			overlayImage->SetColor(float4::Lerp(healthOverlayColor, healthOverlayColorInBackground, switchTimer / switchCollapseMovementTime));
		}

		if (switchHealthStroke) {
			switchHealthStroke->SetColor(float4::Lerp(healthSwitchStrokeInitialColor, healthSwitchStrokeChangingColor, switchTimer / switchCollapseMovementTime));
		}

		switchHealthFillTransform2D = switchHealthChildren[HIERARCHY_INDEX_SWITCH_HEALTH_FILL]->GetComponent<ComponentTransform2D>();
		if (switchHealthFillTransform2D) {
			switchHealthFillTransform2D->SetScale(float3::Lerp(float3(1, 1, 1), float3(0, 1, 1), switchTimer / switchCollapseMovementTime));
		}

		if (switchTimer == switchCollapseMovementTime) {
			switchState = SwitchState::PRE_DEPLOY;

			if (fangSkillParent->IsActive()) {
				fangSkillParent->Disable();
				onimaruSkillParent->Enable();
			} else {
				fangSkillParent->Enable();
				onimaruSkillParent->Disable();
			}
			if (switchHealthFillTransform2D) {
				if (!fangObj->IsActive()) switchHealthFillTransform2D->SetPosition(float3(originalFillPos.x, originalFillPos.y - originalStrokeSize.y, originalFillPos.z));
				else switchHealthFillTransform2D->SetPosition(originalFillPos);
			}
			switchTimer = 0;
		}

		break;
	case SwitchState::PRE_DEPLOY:
		// TODO: Refactor with pivot

		if (switchTimer > switchBarGrowShrinkTime) {
			switchTimer = switchBarGrowShrinkTime;
		}
		if (switchHealthStrokeGrowing) {
			float delta = switchTimer / switchBarGrowShrinkTime;
			float2 size = float2::Lerp(originalStrokeSize, float2(originalStrokeSize.x, originalStrokeSize.y * 2), delta);
			switchHealthStrokeTransform2D = switchHealthChildren[HIERARCHY_INDEX_SWITCH_HEALTH_STROKE]->GetComponent<ComponentTransform2D>();
			if (switchHealthStrokeTransform2D) {
				switchHealthStrokeTransform2D->SetSize(size);
			}
		} else if (switchHealthStrokeShrinking) {
			float delta = switchTimer / switchBarGrowShrinkTime;
			float2 size = float2::Lerp(float2(originalStrokeSize.x, originalStrokeSize.y * 2), originalStrokeSize, delta);
			switchHealthStrokeTransform2D = switchHealthChildren[HIERARCHY_INDEX_SWITCH_HEALTH_STROKE]->GetComponent<ComponentTransform2D>();
			if (switchHealthStrokeTransform2D) {
				switchHealthStrokeTransform2D->SetSize(size);
			}
		}

		if (switchTimer == switchBarGrowShrinkTime) {
			if (switchHealthStrokeGrowing) {
				switchHealthStrokeGrowing = false;
				switchHealthStrokeShrinking = true;

			} else if (switchHealthStrokeShrinking) {
				switchHealthStrokeGrowing = true;
				switchHealthStrokeShrinking = false;

				switchState = SwitchState::DEPLOY;
			}

			if (fangObj->IsActive()) switchHealthStrokeTransform2D->SetPivot(float2(1, 1));
			else switchHealthStrokeTransform2D->SetPivot(float2(0, 0));

			switchTimer = 0;
		}
		break;
	case SwitchState::DEPLOY:

		if (switchTimer > switchDeployMovementTime) {
			switchTimer = switchDeployMovementTime;
		}

		if (fangSkillParent->IsActive()) {


			if (switchTimer < switchDeployMovementTime / 3) {
				float delta = (switchTimer / (switchDeployMovementTime / 3));
				for (int i = 0; i < skillsFang.size(); ++i) {
					transform2D = skillsFang[i]->GetComponent<ComponentTransform2D>();
					if (transform2D) {
						transform2D->SetScale(float3::Lerp(float3(0, 0, 0), float3(1, 1, 1), delta));
					}
				}
			} else {
				for (int i = 0; i < skillsFang.size(); ++i) {
					transform2D = skillsFang[i]->GetComponent<ComponentTransform2D>();
					if (transform2D) {
						transform2D->SetScale(float3(1, 1, 1));
					}
				}
			}

			for (int i = 0; i < skillsFang.size(); ++i) {
				transform2D = skillsFang[i]->GetComponent<ComponentTransform2D>();

				if (transform2D) {
					transform2D->SetPosition(float3::Lerp(cooldownTransformOriginalPositions[static_cast<int>(Cooldowns::SWITCH_SKILL)], cooldownTransformOriginalPositions[i] + float3(switchExtraOffset + i * 10.0f, 0, 0), switchTimer / switchDeployMovementTime));
				}

			}
		} else {

			if (switchTimer < switchDeployMovementTime / 3) {
				float delta = (switchTimer / (switchDeployMovementTime / 3));
				for (int i = 0; i < skillsOni.size(); ++i) {
					transform2D = skillsOni[i]->GetComponent<ComponentTransform2D>();
					if (transform2D) {
						transform2D->SetScale(float3::Lerp(float3(0, 0, 0), float3(1, 1, 1), delta));
					}
				}
			} else {
				for (int i = 0; i < skillsOni.size(); ++i) {
					transform2D = skillsOni[i]->GetComponent<ComponentTransform2D>();
					if (transform2D) {
						transform2D->SetScale(float3(1, 1, 1));
					}
				}
			}

			for (int i = 0; i < skillsOni.size(); ++i) {
				transform2D = skillsOni[i]->GetComponent<ComponentTransform2D>();
				if (transform2D) {
					transform2D->SetPosition(float3::Lerp(cooldownTransformOriginalPositions[static_cast<int>(Cooldowns::SWITCH_SKILL)], cooldownTransformOriginalPositions[i + 3] + float3(switchExtraOffset + i * 10.0f, 0, 0), switchTimer / switchDeployMovementTime));
				}
			}
		}

		ManageSwitchGreenEffect(true, switchDeployMovementTime);

		if (fangObj->IsActive()) {
			health = fangHealthParent->GetComponent<ComponentTransform2D>();
			backgroundImage = fangHealthChildren[HIERARCHY_INDEX_HEALTH_BACKGROUND]->GetComponent<ComponentImage>();
			fillImage = fangHealthChildren[HIERARCHY_INDEX_HEALTH_FILL]->GetComponent<ComponentImage>();
			overlayImage = fangHealthChildren[HIERARCHY_INDEX_HEALTH_OVERLAY]->GetComponent<ComponentImage>();

			if (health) {
				health->SetPosition(float3::Lerp(originalFangHealthPosition - float3(healthOffset, 0, 0), originalFangHealthPosition, switchTimer / switchCollapseMovementTime));
			}
		} else {
			health = onimaruHealthParent->GetComponent<ComponentTransform2D>();
			backgroundImage = onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_BACKGROUND]->GetComponent<ComponentImage>();
			fillImage = onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_FILL]->GetComponent<ComponentImage>();
			overlayImage = onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_OVERLAY]->GetComponent<ComponentImage>();

			if (health) {
				health->SetPosition(float3::Lerp(originalOnimaruHealthPosition, originalOnimaruHealthPosition + float3(healthOffset, 0, 0), switchTimer / switchCollapseMovementTime));
			}
		}

		switchHealthStroke = switchHealthChildren[HIERARCHY_INDEX_SWITCH_HEALTH_STROKE]->GetComponent<ComponentImage>();

		if (backgroundImage) {
			backgroundImage->SetColor(float4::Lerp(healthBarBackgroundColorInBackground, healthBarBackgroundColor, switchTimer / switchCollapseMovementTime));
		}

		if (fillImage) {
			fillImage->SetColor(float4::Lerp(healthFillBarColorInBackground, healthFillBarColor, switchTimer / switchCollapseMovementTime));
		}

		if (overlayImage) {
			overlayImage->SetColor(float4::Lerp(healthOverlayColorInBackground, healthOverlayColor, switchTimer / switchCollapseMovementTime));
		}

		if (switchHealthStroke) {
			switchHealthStroke->SetColor(float4::Lerp(healthSwitchStrokeChangingColor, healthSwitchStrokeInitialColor, switchTimer / switchCollapseMovementTime));
		}

		switchHealthFillTransform2D = switchHealthChildren[HIERARCHY_INDEX_SWITCH_HEALTH_FILL]->GetComponent<ComponentTransform2D>();

		if (switchHealthFillTransform2D) {
			switchHealthFillTransform2D->SetScale(float3::Lerp(float3(0, 1, 1), float3(1, 1, 1), switchTimer / switchCollapseMovementTime));
		}

		if (switchTimer == switchDeployMovementTime) {
			switchState = SwitchState::POST_DEPLOY;
			switchTimer = 0;
		}

		break;
	case SwitchState::POST_DEPLOY:

		if (switchTimer > switchPostDeployMovementTime) {
			switchTimer = switchPostDeployMovementTime;
		}

		if (fangSkillParent->IsActive()) {
			for (int i = 0; i < skillsFang.size(); ++i) {
				transform2D = skillsFang[i]->GetComponent<ComponentTransform2D>();
				if (transform2D) {
					transform2D->SetPosition(float3::Lerp(cooldownTransformOriginalPositions[i] + float3(switchExtraOffset + i * 10.0f, 0, 0), cooldownTransformOriginalPositions[i], switchTimer / switchPreCollapseMovementTime));
				}

			}
		} else {
			for (int i = 0; i < skillsOni.size(); ++i) {
				transform2D = skillsOni[i]->GetComponent<ComponentTransform2D>();
				if (transform2D) {
					transform2D->SetPosition(float3::Lerp(cooldownTransformOriginalPositions[i + 3] + float3(switchExtraOffset + i * 10.0f, 0, 0), cooldownTransformOriginalPositions[i + 3], switchTimer / switchPreCollapseMovementTime));
				}
			}
		}

		ManageSwitchGreenEffect(false, switchPreCollapseMovementTime);

		if (switchTimer == switchPostDeployMovementTime) {
			switchState = SwitchState::IDLE;

			std::vector<GameObject*> switchChildren = switchSkillParent->GetChildren();
			if (switchChildren.size() > HIERARCHY_INDEX_SWITCH_ABILITY_KEY_FILL - 1) {
				switchChildren[HIERARCHY_INDEX_SWITCH_ABILITY_IN_USE_WHITE]->Disable();
				switchChildren[HIERARCHY_INDEX_SWITCH_ABILITY_GREEN_EFFECT]->Disable();
			}

			//IN_USE pictoState is black colored, which, in case of having a dead character, works perfectly
			if (playerController && playerController->AreBothCharactersAlive()) {
				SetPictoState(Cooldowns::SWITCH_SKILL, PictoState::UNAVAILABLE);
			} else {
				SetPictoState(Cooldowns::SWITCH_SKILL, PictoState::IN_USE);
			}

			switchTimer = 0;
		}

		//Reset glow just in case a character was dead and glowEffect was interrupted
		if (switchGlowImage) {
			switchGlowImage->SetColor(float4(1.0f, 1.0f, 1.0f, 0.0f));
		}

		break;

	}
	switchTimer += Time::GetDeltaTime();
}

void HUDManager::PlayCoolDownEffect(AbilityRefeshFX* effect, Cooldowns cooldown) {
	if (effect != nullptr) {
		effect->PlayEffect();
	}

	if (audios[static_cast<int>(HUDManagerAudio::COOLDOWN_RECOVER)]) {
		audios[static_cast<int>(HUDManagerAudio::COOLDOWN_RECOVER)]->Play();
	}
}

void HUDManager::PlayHitEffect() {
	if (sidesHUDChildren.size() != HUD_HIT_FEEDBACK_SIDES) return;

	if (hitEffectTimer > hitEffectTotalTime) {
		hitEffectTimer = hitEffectTotalTime;
	}

	float4 initialHitColor = criticalHealthWarning ? float4(sideHitColor.x, sideHitColor.y, sideHitColor.z, sideHitColor.w * 2) : sideHitColor;
	float4 finalHitColor = criticalHealthWarning ? sideHitColor : sideNormalColor;

	for (GameObject* side : sidesHUDChildren) {
		ComponentImage* sideImage = side->GetComponent<ComponentImage>();
		if (sideImage) sideImage->SetColor(float4::Lerp(initialHitColor, finalHitColor, hitEffectTimer / hitEffectTotalTime));
	}

	if (hitEffectTimer >= hitEffectTotalTime) {
		hitEffectTimer = 0.f;
		playingHitEffect = false;

		for (GameObject* side : sidesHUDChildren) {
			ComponentImage* sideImage = side->GetComponent<ComponentImage>();
			if (sideImage) sideImage->SetColor(float4::Lerp(initialHitColor, finalHitColor, 1.0f));
		}

	} else {
		hitEffectTimer += Time::GetDeltaTime();
	}

}

void HUDManager::ShowCriticalHealthWarning() {
	if (sidesHUDChildren.size() != HUD_HIT_FEEDBACK_SIDES) return;

	for (GameObject* side : sidesHUDChildren) {
		ComponentImage* sideImage = side->GetComponent<ComponentImage>();
		if (sideImage) sideImage->SetColor(float4(sideHitColor.x, sideHitColor.y, sideHitColor.z, sideHitColor.w));
	}

	if (audios[static_cast<int>(HUDManagerAudio::CRITICAL_HEALTH)]) {
		audios[static_cast<int>(HUDManagerAudio::CRITICAL_HEALTH)]->Play();
	}
	criticalHealthWarning = true;
}

void HUDManager::HideCriticalHealthWarning() {
	if (sidesHUDChildren.size() != HUD_HIT_FEEDBACK_SIDES) return;

	for (GameObject* side : sidesHUDChildren) {
		ComponentImage* sideImage = side->GetComponent<ComponentImage>();
		if (sideImage) sideImage->SetColor(sideNormalColor);
	}


	if (audios[static_cast<int>(HUDManagerAudio::CRITICAL_HEALTH)]) {
		audios[static_cast<int>(HUDManagerAudio::CRITICAL_HEALTH)]->Stop();
	}

	criticalHealthWarning = false;
}

void HUDManager::PlayLostHealthFeedback() {
	if (!fangObj || !onimaruObj || !fangHealthParent || !onimaruHealthParent) return;
	if (fangHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN || onimaruHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN) return;

	if (lostHealthTimer > lostHealthFeedbackTotalTime) {
		lostHealthTimer = lostHealthFeedbackTotalTime;
	}

	ComponentImage* lostHealth = nullptr;
	lostHealth = fangObj->IsActive() ? fangHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>() : onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>();
	if (lostHealth) {
		lostHealth->SetColor(float4::Lerp(healthLostFeedbackFillBarInitialColor, healthLostFeedbackFillBarFinalColor, lostHealthTimer / lostHealthFeedbackTotalTime));
	}

	if (lostHealthTimer == lostHealthFeedbackTotalTime) {
		lostHealthTimer = 0.f;
		playingLostHealthFeedback = false;
		ResetLostHealthFeedback();
	} else {
		lostHealthTimer += Time::GetDeltaTime();
	}
}

void HUDManager::StartLostHealthFeedback() {
	if (!fangObj || !onimaruObj || !fangHealthParent || !onimaruHealthParent) return;
	if (fangHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN || onimaruHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN) return;

	lostHealthTimer = 0.f;

	if (!playingLostHealthFeedback) {
		playingLostHealthFeedback = true;
		ComponentImage* lostHealth = nullptr;
		lostHealth = fangObj->IsActive() ? fangHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>() : onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>();
		if (lostHealth) {
			lostHealth->SetColor(healthLostFeedbackFillBarInitialColor);
		}
	} else {
		ResetLostHealthFeedback();
	}
}

void HUDManager::StopLostHealthFeedback() {
	if (!fangObj || !onimaruObj || !fangHealthParent || !onimaruHealthParent || !playerController) return;
	if (fangHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN || onimaruHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN) return;

	playingLostHealthFeedback = false;

	ResetLostHealthFeedback();
}

void HUDManager::ResetLostHealthFeedback() {
	// We don't need to check for null because it's called from a function that already checks them but just in case it's called from anywhere else
	if (!fangObj || !onimaruObj || !fangHealthParent || !onimaruHealthParent || !playerController) return;
	if (fangHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN || onimaruHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN) return;

	float maxHealth = fangObj->IsActive() ? playerController->GetFangMaxHealth() : playerController->GetOnimaruMaxHealth();
	ComponentImage* lostHealth = nullptr;
	lostHealth = fangObj->IsActive() ? fangHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>() : onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>();
	if (lostHealth) {
		float feedbackHealth = fangObj->IsActive() ? fangPreviousHealth : onimaruPreviousHealth;
		if (lostHealth->IsFill()) {
			lostHealth->SetFillValue(feedbackHealth / maxHealth);
		}
	}

	// If we have switched, we need to reset the other health lost fill bar
	if (switchState != SwitchState::IDLE) {
		// If X character is active, we have already swapped so we get the other one feedback bar
		ComponentImage* lostHealth = nullptr;
		lostHealth = fangObj->IsActive() ? fangHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>() : onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>();
		float maxHealth = fangObj->IsActive() ? playerController->GetFangMaxHealth() : playerController->GetOnimaruMaxHealth();
		float feedbackHealth = fangObj->IsActive() ? fangPreviousHealth : onimaruPreviousHealth;

		if (lostHealth) {
			lostHealth->SetColor(healthLostFeedbackFillBarFinalColor);
			if (lostHealth->IsFill()) {
				lostHealth->SetFillValue(feedbackHealth / maxHealth);
			}
		}
	}
}

void HUDManager::SetPictoState(Cooldowns cooldown, PictoState newState) {
	if (pictoStates[static_cast<int>(cooldown)] == newState)return;

	float4 colorToUse = float4(0, 0, 0, 0);
	switch (newState) {
	case PictoState::AVAILABLE:
		colorToUse = cooldown == Cooldowns::SWITCH_SKILL ? switchPictoColorNotInUse : skillPictoColorAvailable;
		break;
	case PictoState::UNAVAILABLE:
		colorToUse = cooldown == Cooldowns::SWITCH_SKILL ? switchPictoColorNotInUse : skillPictoColorNotAvailable;
		break;
	case PictoState::IN_USE:
		colorToUse = cooldown == Cooldowns::SWITCH_SKILL ? switchPictoColorInUse : skillPictoColorInUse;
		break;
	}

	std::vector<GameObject*>children;

	if (static_cast<int>(cooldown) < static_cast<int>(Cooldowns::ONIMARU_SKILL_1)) {
		children = fangSkillParent->GetChildren();
	} else 	if (static_cast<int>(cooldown) < static_cast<int>(Cooldowns::SWITCH_SKILL)) {
		children = onimaruSkillParent->GetChildren();
	} else {
		children = switchSkillParent->GetChildren();
	}

	if (cooldown != Cooldowns::SWITCH_SKILL) {
		//Character-specific ability picto state
		if (children[static_cast<int>(cooldown) % 3]->GetChildren().size() > HIERARCHY_INDEX_ABILITY_PICTO_SHADE - 1) {
			GameObject* pictoShade = children[(static_cast<int>(cooldown)) % 3]->GetChild(HIERARCHY_INDEX_ABILITY_PICTO_SHADE);

			if (pictoShade->HasChildren()) {
				GameObject* pictoFillObj = pictoShade->GetChild(static_cast<unsigned int>(0));
				if (pictoFillObj) {
					ComponentImage* pictoFill = pictoFillObj->GetComponent<ComponentImage>();
					if (pictoFill) {
						pictoFill->SetColor(colorToUse);
					}
				}
			}

		}
	} else {
		//Switch ability picto state
		if (children.size() < HIERARCHY_INDEX_SWITCH_ABILITY_PICTO_SHADE) return;

		GameObject* pictoShade = children[HIERARCHY_INDEX_SWITCH_ABILITY_PICTO_SHADE];
		if (pictoShade) {
			if (pictoShade->HasChildren()) {
				GameObject* pictoFill = pictoShade->GetChild(static_cast<unsigned int>(0));
				if (pictoFill) {
					ComponentImage* pictoFillImage = pictoFill->GetComponent<ComponentImage>();
					if (pictoFillImage) {
						pictoFillImage->SetColor(colorToUse);
					}
				}
			}
		}
	}


}

void HUDManager::GetAllHealthColors() {
	if (!fangHealthParent || !onimaruHealthParent) return;
	if (fangHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN || onimaruHealthChildren.size() != HEALTH_HIERARCHY_NUM_CHILDREN) return;

	// Get background color in background
	ComponentImage* image = onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_BACKGROUND]->GetComponent<ComponentImage>();
	if (image) healthBarBackgroundColorInBackground = image->GetColor();

	// Get fill color in background
	image = onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_FILL]->GetComponent<ComponentImage>();
	if (image) healthFillBarColorInBackground = image->GetColor();

	// Get overlay color in background
	image = onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_OVERLAY]->GetComponent<ComponentImage>();
	if (image) healthOverlayColorInBackground = image->GetColor();

	// Get main background color
	image = fangHealthChildren[HIERARCHY_INDEX_HEALTH_BACKGROUND]->GetComponent<ComponentImage>();
	if (image) healthBarBackgroundColor = image->GetColor();

	// Get main fill color
	image = fangHealthChildren[HIERARCHY_INDEX_HEALTH_FILL]->GetComponent<ComponentImage>();
	if (image) healthFillBarColor = image->GetColor();

	// Get main overlay color
	image = fangHealthChildren[HIERARCHY_INDEX_HEALTH_OVERLAY]->GetComponent<ComponentImage>();
	if (image) healthOverlayColor = image->GetColor();

	// Get lost feedback colors
	image = fangHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>();
	if (image) {
		healthLostFeedbackFillBarFinalColor = image->GetColor();
	}

	healthLostFeedbackFillBarInitialColor = float4(healthLostFeedbackFillBarFinalColor.xyz(), lostHealthFeedbackAlpha);
}

void HUDManager::InitializeHealth() {
	if (!playerController) return;

	// Set initial health values
	ComponentImage* health = fangHealthChildren[HIERARCHY_INDEX_HEALTH_FILL]->GetComponent<ComponentImage>();
	float healthValue = 1.f;
	if (health) {
		if (health->IsFill()) {
			healthValue = playerController->GetFangMaxHealth();
			health->SetFillValue(healthValue / healthValue);
		}
	}

	fangPreviousHealth = healthValue;

	health = onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_FILL]->GetComponent<ComponentImage>();
	healthValue = 1.f;
	if (health) {
		if (health->IsFill()) {
			healthValue = playerController->GetOnimaruMaxHealth();
			health->SetFillValue(healthValue / healthValue);
		}
	}

	onimaruPreviousHealth = healthValue;

	// Set initial lost health bar
	ComponentImage* healthLost = fangHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>();
	healthValue = 1.f;
	if (healthLost) {
		if (healthLost->IsFill()) {
			healthValue = playerController->GetFangMaxHealth();
			healthLost->SetFillValue(healthValue / healthValue);
		}
		healthLost->SetColor(healthLostFeedbackFillBarFinalColor);
	}

	healthLost = onimaruHealthChildren[HIERARCHY_INDEX_HEALTH_LOST_FEEDBACK]->GetComponent<ComponentImage>();
	healthValue = 1.f;
	if (healthLost) {
		if (healthLost->IsFill()) {
			healthValue = playerController->GetOnimaruMaxHealth();
			healthLost->SetFillValue(healthValue / healthValue);
		}
		healthLost->SetColor(healthLostFeedbackFillBarFinalColor);
	}
}

void HUDManager::InitializeHUDSides() {
	if (sidesHUDChildren.size() == HUD_HIT_FEEDBACK_SIDES) {
		// Get normal color 
		GameObject* leftSide = sidesHUDChildren[HIERARCHY_INDEX_HUD_LEFT_SIDE];
		ComponentImage* sideImage = nullptr;
		if (leftSide) {
			sideImage = leftSide->GetComponent<ComponentImage>();
			if (sideImage) sideNormalColor = sideImage->GetColor();
		}

		// Get hit color and set it to normal
		GameObject* rightSide = sidesHUDChildren[HIERARCHY_INDEX_HUD_RIGHT_SIDE];
		if (rightSide) {
			sideImage = rightSide->GetComponent<ComponentImage>();
			if (sideImage) sideHitColor = sideImage->GetColor();
			sideImage->SetColor(sideNormalColor);
		}
	}
}

void HUDManager::ManageSwitchPreCollapseState(GameObject* activeParent, const std::vector<GameObject*>& skills) {
	for (int i = 0; i < skills.size(); ++i) {
		ComponentTransform2D* transform2D = skills[i]->GetComponent<ComponentTransform2D>();
		if (transform2D) {
			transform2D->SetPosition(float3::Lerp(cooldownTransformOriginalPositions[i], cooldownTransformOriginalPositions[i] + float3(switchExtraOffset + i * 10.0f, 0, 0), switchTimer / switchPreCollapseMovementTime));
		}
	}
}

void HUDManager::ManageSwitchCollapseState(GameObject* skillsParent, const std::vector<GameObject*>& skills) {
	ComponentTransform2D* transform2D = nullptr;

	if (switchTimer > switchCollapseMovementTime / 1.5f) {
		float delta = (switchTimer - (switchCollapseMovementTime / 1.5f)) / (switchCollapseMovementTime / 1.5f);
		for (int i = 0; i < skills.size(); ++i) {
			transform2D = skills[i]->GetComponent<ComponentTransform2D>();
			if (transform2D) {
				transform2D->SetScale(float3::Lerp(float3(1, 1, 1), float3(0, 0, 0), delta));
			}
		}
	}

	for (int i = 0; i < skills.size(); ++i) {
		transform2D = skills[i]->GetComponent<ComponentTransform2D>();
		if (transform2D) {
			transform2D->SetPosition(float3::Lerp(cooldownTransformOriginalPositions[i] + float3(switchExtraOffset + i * 10.0f, 0, 0), cooldownTransformOriginalPositions[static_cast<int>(Cooldowns::SWITCH_SKILL)], switchTimer / switchCollapseMovementTime));
		}

	}
}

void HUDManager::ManageSwitchGreenEffect(bool growing, float timer) {
	std::vector<GameObject*> switchSkillChildren = switchSkillParent->GetChildren();

	if (switchSkillChildren.size() == SWITCH_SKILL_HIERARCHY_NUM_CHILDREN) {
		float xInitialScale = growing ? 0.5f : 1;
		float xFinalScale = growing ? 1 : 0.5f;
		ComponentTransform2D* transform2D = switchSkillChildren[HIERARCHY_INDEX_SWITCH_ABILITY_GREEN_EFFECT]->GetComponent<ComponentTransform2D>();
		if (transform2D) {
			transform2D->SetScale(float3::Lerp(float3(xInitialScale, 1.f, 1.f), float3(xFinalScale, 1, 1), switchTimer / timer));
		}
	}
}
