#include "HUDController.h"

#include "GameObject.h"
#include "Components/UI/ComponentImage.h"
#include "GameplaySystems.h"

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
};

GENERATE_BODY_IMPL(HUDController);

GameObject* HUDController::fangCanvas = nullptr;
GameObject* HUDController::onimaruCanvas = nullptr;

GameObject* HUDController::fang = nullptr;
GameObject* HUDController::onimaru = nullptr;

GameObject* HUDController::fangSkillsMainCanvas = nullptr;
GameObject* HUDController::onimaruSkillsMainCanvas = nullptr;
GameObject* HUDController::fangSkillsSecondCanvas = nullptr;
GameObject* HUDController::onimaruSkillsSecondCanvas = nullptr;

GameObject* HUDController::fangHealthMainCanvas = nullptr;
GameObject* HUDController::onimaruHealthMainCanvas = nullptr;
GameObject* HUDController::fangHealthSecondCanvas = nullptr;
GameObject* HUDController::onimaruHealthSecondCanvas = nullptr;
GameObject* HUDController::swapingSkillCanvas = nullptr;

std::array<float, Cooldowns::TOTAL> HUDController::cooldowns;


const float4 HUDController::colorMagenta = float4(236, 60, 137, 255) / 255;
const float4 HUDController::colorWhite = float4(255, 255, 255, 255) / 255;

const float4 HUDController::colorMagentaDarkened = float4(236, 60, 137, 128) / 255;
const float4 HUDController::colorWhiteDarkened = float4(255, 255, 255, 128) / 255;

void HUDController::Start() {

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
    
    if (onimaru && onimaruCanvas && fangCanvas && fang) {
        onimaru->Disable();
        onimaruCanvas->Disable();
        fangCanvas->Enable();
        fang->Enable();
    }


    cooldowns[Cooldowns::FANG_SKILL_1] = 0;
    cooldowns[Cooldowns::FANG_SKILL_2] = 0;
    cooldowns[Cooldowns::FANG_SKILL_3] = 0;
    cooldowns[Cooldowns::ONIMARU_SKILL_1] = 0;
    cooldowns[Cooldowns::ONIMARU_SKILL_2] = 0;
    cooldowns[Cooldowns::ONIMARU_SKILL_3] = 0;
    cooldowns[Cooldowns::SWITCH_SKILL] = 0;
}

void HUDController::Update() {
    if (!fangCanvas || !onimaruCanvas) return;
}

void HUDController::ChangePlayerHUD() {
    if (!fang || !onimaru) return;

    if (!fang->IsActive()) {
        fangCanvas->Disable();
        onimaruCanvas->Enable();
    }
    else {
        onimaruCanvas->Disable();
        fangCanvas->Enable();
    }
}

void HUDController::UpdateCooldowns(float onimaruCooldown1, float onimaruCooldown2, float onimaruCooldown3, float fangCooldown1, float fangCooldown2, float fangCooldown3, float switchCooldown) {
    
    cooldowns[Cooldowns::FANG_SKILL_1] = fangCooldown1;
    cooldowns[Cooldowns::FANG_SKILL_2] = fangCooldown2;
    cooldowns[Cooldowns::FANG_SKILL_3] = fangCooldown3;
    cooldowns[Cooldowns::ONIMARU_SKILL_1] = onimaruCooldown1;
    cooldowns[Cooldowns::ONIMARU_SKILL_2] = onimaruCooldown2;
    cooldowns[Cooldowns::ONIMARU_SKILL_3] = onimaruCooldown3;
    cooldowns[Cooldowns::SWITCH_SKILL] = switchCooldown;
    
    UpdateComponents();
}

void HUDController::UpdateHP(float currentHp, float altHp) {
    if (fang->IsActive()) {
        UpdateCanvasHP(fangHealthMainCanvas, currentHp, false);
        UpdateCanvasHP(onimaruHealthSecondCanvas, altHp, true);
    }
    else {
        UpdateCanvasHP(onimaruHealthMainCanvas, altHp, false);
        UpdateCanvasHP(fangHealthSecondCanvas, currentHp, true);
    }
}

void HUDController::UpdateComponents() {

    if (!fang || !onimaru) return;
    if (!fangSkillsMainCanvas || !onimaruSkillsMainCanvas || !fangSkillsSecondCanvas || !onimaruSkillsSecondCanvas) return;
    if (!fangHealthMainCanvas || !onimaruHealthMainCanvas || !fangHealthSecondCanvas || !onimaruHealthSecondCanvas) return;
    if (!swapingSkillCanvas) return;
    // Update all cooldowns
    if (fang->IsActive()) {
        UpdateFangCooldowns(fangSkillsMainCanvas);
        UpdateOnimaruCooldowns(onimaruSkillsSecondCanvas);
        UpdateCommonSkill();
    }
    else if(onimaru->IsActive()) {
        UpdateOnimaruCooldowns(onimaruSkillsMainCanvas);
        UpdateFangCooldowns(fangSkillsSecondCanvas);
        UpdateCommonSkill();
    }
}

void HUDController::UpdateCommonSkill() {
    std::vector<GameObject*> children = swapingSkillCanvas->GetChildren();
    for (std::vector<GameObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        ComponentImage* image = (*it)->GetComponent<ComponentImage>();
        if (image) {
            if (image->IsFill()) {
                image->SetFillValue(cooldowns[Cooldowns::SWITCH_SKILL]);
            }
        }
    }
}

void HUDController::UpdateFangCooldowns(GameObject* fangSkillCanvas) {
    std::vector<GameObject*> children = fangSkillCanvas->GetChildren();
    int skill = FANG_SKILL_1;
    for (std::vector<GameObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        std::vector<GameObject*> skills = (*it)->GetChildren();
        for (std::vector<GameObject*>::iterator itSkills = skills.begin(); itSkills != skills.end(); ++itSkills) {
            ComponentImage* image = (*itSkills)->GetComponent<ComponentImage>();
            if (image) {
                if (image->IsFill()) {
                    switch (skill) {
                        case Cooldowns::FANG_SKILL_1:
                            image->SetFillValue(cooldowns[Cooldowns::FANG_SKILL_1]);
                            break;
                        case Cooldowns::FANG_SKILL_2:
                            image->SetFillValue(cooldowns[Cooldowns::FANG_SKILL_2]);
                            break;
                        case Cooldowns::FANG_SKILL_3:
                            image->SetFillValue(cooldowns[Cooldowns::FANG_SKILL_3]);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        ++skill;
    }
}

void HUDController::UpdateOnimaruCooldowns(GameObject* onimaruSkillCanvas) {
    std::vector<GameObject*> children = onimaruSkillCanvas->GetChildren();
    int skill = ONIMARU_SKILL_1;
    for (std::vector<GameObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        std::vector<GameObject*> skills = (*it)->GetChildren();
        for (std::vector<GameObject*>::iterator itSkills = skills.begin(); itSkills != skills.end(); ++itSkills) {
            ComponentImage* image = (*itSkills)->GetComponent<ComponentImage>();
            if (image) {
                if (image->IsFill()) {
                    switch (skill) {
                        case Cooldowns::ONIMARU_SKILL_1:
                            image->SetFillValue(cooldowns[Cooldowns::ONIMARU_SKILL_1]);
                            break;
                        case Cooldowns::ONIMARU_SKILL_2:
                            image->SetFillValue(cooldowns[Cooldowns::ONIMARU_SKILL_2]);
                            break;
                        case Cooldowns::ONIMARU_SKILL_3:
                            image->SetFillValue(cooldowns[Cooldowns::ONIMARU_SKILL_3]);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        ++skill;
    }
}

void HUDController::UpdateCanvasHP(GameObject* targetCanvas, int health, bool darkened)
{
    float4 magentaToSet = darkened ? colorMagentaDarkened : colorMagenta;
    float4 whiteToSet = darkened ? colorWhiteDarkened : colorWhite;

    int i = 0;
    for (GameObject* hpGameObject : targetCanvas->GetChildren()) {
        ComponentImage* hpComponent = hpGameObject->GetComponent<ComponentImage>();
        if (i < health) {
            hpComponent->SetColor(magentaToSet);
        }
        else {
            hpComponent->SetColor(whiteToSet);
        }
        i++;
    }
}
