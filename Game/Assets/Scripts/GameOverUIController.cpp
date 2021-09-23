#include "GameOverUIController.h"

#include "UISpriteSheetPlayer.h"
#include "GameplaySystems.h"
#include "Components/UI/ComponentImage.h"
#include "Components/UI/ComponentText.h"
#include "Components/ComponentAudioSource.h"

//TODO MAKE BUTTONS DO WHAT THEY SAY THEY DO
//TODO MAKE BACKGROUND ANIMATED
//TODO ADJUST UI SIZES

EXPOSE_MEMBERS(GameOverUIController) {
	MEMBER(MemberType::GAME_OBJECT_UID, inOutPlayerUID),
	MEMBER(MemberType::GAME_OBJECT_UID, loopPlayerUID),
	MEMBER(MemberType::GAME_OBJECT_UID, vibrationPlayerUID),
	MEMBER(MemberType::FLOAT, minVibrationTime),
	MEMBER(MemberType::FLOAT, maxVibrationTime),
	MEMBER(MemberType::FLOAT, fadeInTime),
	MEMBER(MemberType::GAME_OBJECT_UID, playAgainButtonUID),
	MEMBER(MemberType::GAME_OBJECT_UID, mainMenuButtonUID),
	MEMBER(MemberType::GAME_OBJECT_UID, backgroundUID),
	MEMBER(MemberType::GAME_OBJECT_UID, scrollingBackgroundObjUID),
	MEMBER(MemberType::FLOAT, scrollDuration)
};

GENERATE_BODY_IMPL(GameOverUIController);

void GameOverUIController::Start() {

	GameObject* inOutPlayerGO = GameplaySystems::GetGameObject(inOutPlayerUID);
	GameObject* loopPlayerGO = GameplaySystems::GetGameObject(loopPlayerUID);
	GameObject* vibrationPlayerGO = GameplaySystems::GetGameObject(vibrationPlayerUID);

	if (inOutPlayerGO) inOutPlayer = GET_SCRIPT(inOutPlayerGO, UISpriteSheetPlayer);
	if (loopPlayerGO) loopPlayer = GET_SCRIPT(loopPlayerGO, UISpriteSheetPlayer);
	if (vibrationPlayerGO) vibrationPlayer = GET_SCRIPT(vibrationPlayerGO, UISpriteSheetPlayer);

	GameObject* playAgainGO = GameplaySystems::GetGameObject(playAgainButtonUID);
	GameObject* mainMenuGO = GameplaySystems::GetGameObject(mainMenuButtonUID);
	GameObject* backgroundGO = GameplaySystems::GetGameObject(backgroundUID);
	GameObject* scrollingBackgroundObj = GameplaySystems::GetGameObject(scrollingBackgroundObjUID);

	if (playAgainGO) {
		playAgainButtonImage = playAgainGO->GetComponent<ComponentImage>();
		playAgainButtonText = playAgainGO->GetChildren()[0]->GetComponent < ComponentText>();
	}
	if (mainMenuGO) {
		mainMenuButtonImage = mainMenuGO->GetComponent<ComponentImage>();
		mainMenuButtonText = mainMenuGO->GetChildren()[0]->GetComponent < ComponentText>();
	}

	if (backgroundGO) {
		backgroundImage = backgroundGO->GetComponent<ComponentImage>();

	}

	if (scrollingBackgroundObj) {
		scrollingBackgroundImage = scrollingBackgroundObj->GetComponent<ComponentImage>();
	}

	std::vector<GameObject*> children = GetOwner().GetChildren();

	for (std::vector<GameObject*>::const_iterator it = children.begin(); it != children.end(); ++it) {
		(*it)->Disable();
	}

}

void GameOverUIController::Update() {
	if (!inOutPlayer || !loopPlayer || !vibrationPlayer) return;

	if (scrollingBackgroundImage) {
		if (scrollingTimer < scrollDuration) {
			scrollingTimer += Time::GetDeltaTime();
			scrollingBackgroundImage->SetTextureOffset(float2(-scrollingTimer * 0.1f, scrollingTimer * 0.1f));
		} else {
			scrollingTimer = 0.0f;
		}
	}
	switch (state) {
	case GameOverState::OFFLINE:
		break;
	case GameOverState::FADE_IN: {
		fadeInTimer += Time::GetDeltaTime();
		float delta = Clamp01(fadeInTimer / fadeInTime);
		SetColors(delta);

		if (delta >= 1.0f) {
			state = GameOverState::START;
		}
		if (delta >= 0.5f && !inOutPlayer->IsPlaying()) {
			inOutPlayer->Play();
		}

		break;
	}
	case GameOverState::START:
		if (!inOutPlayer->IsPlaying()) {
			EnterIdleState();
		}
		break;
	case GameOverState::IDLE:

		vibrationTimer -= Time::GetDeltaTime();

		if (vibrationTimer <= 0) {
			state = GameOverState::VIBRATION;
			vibrationTimer = 0.0f;
			loopPlayer->Stop();
			vibrationPlayer->Play();
		}

		break;
	case GameOverState::VIBRATION:
		if (!vibrationPlayer->IsPlaying()) {
			EnterIdleState();
		}
		break;
	}
}

void GameOverUIController::GameOver() {
	GetOwner().Enable();
	if (inOutPlayer && state == GameOverState::OFFLINE) {
		fadeInTimer = 0.0f;
		state = GameOverState::FADE_IN;

		std::vector<GameObject*> children = GetOwner().GetChildren();

		for (std::vector<GameObject*>::const_iterator it = children.begin(); it != children.end(); ++it) {
			(*it)->Enable();
		}

		if (backgroundImage && playAgainButtonImage && mainMenuButtonImage && scrollingBackgroundImage) {
			backgroundOriginalColor = backgroundImage->GetColor();
			playAgainButtonImageOriginalColor = playAgainButtonImage->GetColor();
			mainMenuButtonImageOriginalColor = mainMenuButtonImage->GetColor();
			scrollingBackgroundImageOriginalColor = scrollingBackgroundImage->GetColor();
			SetColors(0);
		}
	}
	ComponentAudioSource* audio = GetOwner().GetParent()->GetComponent<ComponentAudioSource>();
	if (audio && audio->IsPlaying()) {
		audio->Stop();
	}
}

void GameOverUIController::EnterIdleState() {
	state = GameOverState::IDLE;
	loopPlayer->Play();
	vibrationTimer = minVibrationTime + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (maxVibrationTime - minVibrationTime)));
}

void GameOverUIController::SetColors(float delta) {
	if (!playAgainButtonImage || !playAgainButtonText || !mainMenuButtonImage || !mainMenuButtonText || !backgroundImage)return;

	playAgainButtonImage->SetColor(float4(playAgainButtonImageOriginalColor.xyz(), delta));
	playAgainButtonText->SetFontColor(float4(1, 1, 1, delta));
	mainMenuButtonImage->SetColor(float4(mainMenuButtonImageOriginalColor.xyz(), delta));
	mainMenuButtonText->SetFontColor(float4(1, 1, 1, delta));
	backgroundImage->SetColor(float4(backgroundOriginalColor.xyz(), delta * backgroundOriginalColor.w));

	scrollingBackgroundImage->SetColor(float4(scrollingBackgroundImageOriginalColor.xyz(), delta * scrollingBackgroundImageOriginalColor.w));
}
