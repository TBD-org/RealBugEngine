#include "AbilityRefeshFX.h"

#include "GameplaySystems.h"
#include "Components/UI/ComponentImage.h"
#include "Components/UI/ComponentTransform2D.h"
#include "Components/ComponentAudioSource.h"
#include "UISpriteSheetPlayer.h"
#include "HUDManager.h"

#define HIERARCHY_INDEX_GLOW_EFFECT 2

EXPOSE_MEMBERS(AbilityRefeshFX) {
	MEMBER(MemberType::FLOAT, totalEffectTime),
	MEMBER(MemberType::FLOAT, effectScale),
	MEMBER(MemberType::GAME_OBJECT_UID, pictoObjectUID),
	MEMBER(MemberType::GAME_OBJECT_UID, keyObjectUID),
	MEMBER(MemberType::BOOL, debugPlay)
};

GENERATE_BODY_IMPL(AbilityRefeshFX);

void AbilityRefeshFX::Start() {

	GameObject* effectMember1Obj = &GetOwner();

	if (effectMember1Obj) {
		effectMember1 = effectMember1Obj->GetComponent<ComponentImage>();
		if (effectMember1) {
			effectMember1->Disable();
		}
	}

	GameObject* pictoObj = GameplaySystems::GetGameObject(pictoObjectUID);
	GameObject* keyObj = GameplaySystems::GetGameObject(keyObjectUID);

	if (pictoObj && keyObj) {
		pictoTransform2D = pictoObj->GetComponent<ComponentTransform2D>();
		keyTransform2D = keyObj->GetComponent<ComponentTransform2D>();

		if (pictoTransform2D) {
			originalScaleVector = pictoTransform2D->GetScale();
		}
	}

	effectAudio = GetOwner().GetComponent<ComponentAudioSource>();
	children = GetOwner().GetChildren();

	if (children.size() > HIERARCHY_INDEX_GLOW_EFFECT) {
		glowImage = children[HIERARCHY_INDEX_GLOW_EFFECT]->GetComponent<ComponentImage>();
		if (glowImage) {
			glowImage->SetColor(float4(glowImage->GetColor().xyz(), 0));
		}
	}



}

void AbilityRefeshFX::Update() {

	if (debugPlay) {
		debugPlay = false;
		PlayEffect();
	}

	if (!isPlaying || !pictoTransform2D || !keyTransform2D || !effectMember1) return;

	effectTimer = Min(totalEffectTime, effectTimer + Time::GetDeltaTime());

	float deltaA = effectTimer / totalEffectTime;
	float deltaB = Min(1.0f, effectTimer * 1.2f / totalEffectTime);

	//DO EFFECT
	pictoTransform2D->SetScale(float3::Lerp(effectScaleVector, originalScaleVector, deltaA));
	keyTransform2D->SetScale(float3::Lerp(effectScaleVector, originalScaleVector, deltaA));


	effectMember1->SetColor(float4(effectMember1->GetColor().xyz(), 1 - deltaA));


	if (glowImage) {
		glowImage->SetColor(float4(glowImage->GetColor().xyz(), Clamp01(1 - deltaA * 2)));
	}



	if (effectTimer == totalEffectTime) {
		isPlaying = false;
		effectMember1->Disable();
	}

}

void AbilityRefeshFX::PlayEffect() {
	if (!pictoTransform2D || !keyTransform2D) return;
	if (effectMember1) {
		effectMember1->Enable();
		//RESET COLOR TO ORIGINAL
		effectMember1->SetColor(float4(effectMember1->GetColor().xyz(), 1.0f));
	}

	if (effectAudio) {
		effectAudio->Play();
	}


	for (size_t i = 0; i < 2; i++) {
		UISpriteSheetPlayer* player = GET_SCRIPT(children[i], UISpriteSheetPlayer);
		if (player) {
			player->Play();
		}
	}



	effectScaleVector = float3(effectScale);
	effectTimer = 0;
	isPlaying = true;
}

