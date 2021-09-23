#include "AbilityRefreshEffectProgressBar.h"

#include "GameplaySystems.h"
#include "Components/UI/ComponentProgressBar.h"
#include "Components/UI/ComponentImage.h"

#include <math.h> 

EXPOSE_MEMBERS(AbilityRefreshEffectProgressBar) {
	MEMBER(MemberType::FLOAT, durationLoadUp),
	MEMBER(MemberType::BOOL, debugPlay),
	MEMBER(MemberType::GAME_OBJECT_UID, progressBarUID)
};

GENERATE_BODY_IMPL(AbilityRefreshEffectProgressBar);

void AbilityRefreshEffectProgressBar::Start() {
	progressBarGO = GameplaySystems::GetGameObject(progressBarUID);
	if (progressBarGO != nullptr) {
		progressBar = progressBarGO->GetComponent<ComponentProgressBar>();
		if (progressBar) {
			progressBar->SetValue(1);
		}
	}
}

void AbilityRefreshEffectProgressBar::Update() {
	if (!progressBar) {
		return;
	}

	if (debugPlay) {
		debugPlay = false;
		Play();
	}

	// Progress bar loadup
	if (!animationLoadUpFinished) {
		if (currentTime <= durationLoadUp) {
			progressBar->SetValue(currentTime / durationLoadUp);
		}
		else{
			animationLoadUpFinished = true;
			progressBar->SetValue(1);
		}
	}

	currentTime += Time::GetDeltaTime();
}

void AbilityRefreshEffectProgressBar::Play() {
	if (!progressBar) {
		return;
	}

	currentTime = 0;
	progressBar->SetValue(0);
	debugPlay = false;
	animationLoadUpFinished = false;
}

void AbilityRefreshEffectProgressBar::ResetBar()
{
	if (!progressBar) {
		return;
	}
	progressBar->SetValue(0);
}