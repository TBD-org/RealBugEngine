#include "AbilityRefreshEffect.h"

#include "GameplaySystems.h"
#include "Components/UI/ComponentImage.h"
#include "Components/UI/ComponentTransform2D.h"
#include "HUDController.h"

// clang-format off
EXPOSE_MEMBERS(AbilityRefreshEffect) {
	MEMBER(MemberType::FLOAT, totalEffectTime),
	MEMBER(MemberType::FLOAT, effectScale),
	MEMBER(MemberType::GAME_OBJECT_UID, effectMember1UID),
	MEMBER(MemberType::GAME_OBJECT_UID, skillParentObjectUID),
	MEMBER(MemberType::BOOL, debugPlay)
};// clang-format on


GENERATE_BODY_IMPL(AbilityRefreshEffect);

void AbilityRefreshEffect::Start() {
	GameObject* effectMember1Obj = GameplaySystems::GetGameObject(effectMember1UID);
	GameObject* skillObject = GameplaySystems::GetGameObject(skillParentObjectUID);

	if (effectMember1Obj) {
		effectMember1 = effectMember1Obj->GetComponent<ComponentImage>();
		effectTransform2D = effectMember1Obj->GetComponent<ComponentTransform2D>();
		if (effectMember1) {
			originalAlpha = effectMember1->GetColor().z;
			originalColor = effectMember1->GetColor().xyz();
			effectMember1->Disable();
		}
	}

	if (skillObject) {
		skillObjTransform2D = skillObject->GetComponent<ComponentTransform2D>();
	}
}



void AbilityRefreshEffect::Update() {

	if (debugPlay) {
		debugPlay = false;
		Play();
	}

	if (!isPlaying || !skillObjTransform2D) return;

	effectTimer = Min(totalEffectTime, effectTimer + Time::GetDeltaTime());

	float deltaA = effectTimer / totalEffectTime;
	float deltaB = Min(1.0f, effectTimer * 1.2f / totalEffectTime);

	//DO EFFECT
	skillObjTransform2D->SetScale(float3::Lerp(effectScaleVector, originalScaleVector, deltaA));
	effectTransform2D->SetScale(float3::Lerp(effectScaleVector, originalEffectScaleVector, deltaB));

	if (deltaA > 0.5) {
		deltaA = HUDController::MapValue01(deltaA, 0.5f, 1.0f);
		effectMember1->SetColor(float4(originalColor, Lerp(originalAlpha, 0, deltaA)));
	}


	if (effectTimer == totalEffectTime) {
		isPlaying = false;
		effectMember1->Disable();
	}
}

void AbilityRefreshEffect::Play() {
	if (!skillObjTransform2D) return;
	if (effectMember1) {
		effectMember1->Enable();
		effectMember1->SetColor(float4(originalColor, originalAlpha));
	}
	originalScaleVector = skillObjTransform2D->GetScale();
	originalEffectScaleVector = effectTransform2D->GetScale();
	effectScaleVector = float3(effectScale);
	effectTimer = 0;
	isPlaying = true;
}