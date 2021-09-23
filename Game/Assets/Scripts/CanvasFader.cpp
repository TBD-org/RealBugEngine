#include "CanvasFader.h"

#include "GameplaySystems.h"
#include "GameObject.h"
#include "Components/UI/ComponentImage.h"

EXPOSE_MEMBERS(CanvasFader) {
	MEMBER(MemberType::GAME_OBJECT_UID, blackImageObjUID),
	MEMBER(MemberType::BOOL, fadeInOnStart)
};

GENERATE_BODY_IMPL(CanvasFader);

void CanvasFader::Start() {
	GameObject* blackImageObj = GameplaySystems::GetGameObject(blackImageObjUID);
	if (blackImageObj) {
		blackImage = blackImageObj->GetComponent<ComponentImage>();
	}

	if (fadeInOnStart) {
		FadeIn();
	}
}

void CanvasFader::Update() {
	UpdateBlackImage();
}

void CanvasFader::FadeIn() {
	if (!blackImage)return;
	blackImage->SetColor(float4(blackImage->GetColor().xyz(), 1.0f));
	Play();
	fadeState = FadeState::FADE_IN;
}

void CanvasFader::FadeOut() {
	if (!blackImage)return;
	blackImage->SetColor(float4(blackImage->GetColor().xyz(), 0.0f));
	Play();
	fadeState = FadeState::FADE_OUT;
}

void CanvasFader::Stop() {
	fadeState = FadeState::IDLE;
	playing = false;
}

bool CanvasFader::IsPlaying() const {
	return playing && blackImage;
}

void CanvasFader::UpdateBlackImage() {
	switch (fadeState) {
	case FadeState::IDLE:
		break;
	case FadeState::FADE_IN:
		if (fadeTimer > fadeDuration) {
			blackImage->SetColor(noAlphaBlack);
			Stop();
		} else {
			blackImage->SetColor(float4::Lerp(alphaBlack, noAlphaBlack, Clamp01(fadeTimer / fadeDuration)));
		}
		fadeTimer += Time::GetDeltaTime();
		break;
	case FadeState::FADE_OUT:
		if (fadeTimer > fadeDuration) {
			blackImage->SetColor(alphaBlack);
			Stop();
		} else {
			blackImage->SetColor(float4::Lerp(noAlphaBlack, alphaBlack, Clamp01(fadeTimer / fadeDuration)));
		}
		fadeTimer += Time::GetDeltaTime();
		break;
	}
}

void CanvasFader::Play() {
	fadeTimer = 0.0f;
	playing = true;
}