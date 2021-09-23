#include "GlitchyTitleController.h"

#include "GameplaySystems.h"
#include "UISpriteSheetPlayer.h"
#include "Components/UI/ComponentImage.h"
#include "CanvasFader.h"
#include "Components/ComponentAudioSource.h"
#include "StartTitleGlitchOnPlay.h"

EXPOSE_MEMBERS(GlitchyTitleController) {
	MEMBER(MemberType::FLOAT, minTimeForTimeGlitches),
	MEMBER(MemberType::FLOAT, maxTimeForTimeBetweenGlitches),
	MEMBER(MemberType::FLOAT, glitchDuration),
	MEMBER(MemberType::GAME_OBJECT_UID, startUpTitleObjUID),
	MEMBER(MemberType::GAME_OBJECT_UID, loopingTitleObjUID),
	MEMBER(MemberType::GAME_OBJECT_UID, glitchTitleObjUID),
	MEMBER(MemberType::GAME_OBJECT_UID, startPlayingTitleObjUID),
	MEMBER(MemberType::GAME_OBJECT_UID, blackImageObjUID)
};

GENERATE_BODY_IMPL(GlitchyTitleController);

void GlitchyTitleController::Start() {
	GameObject* startUpTitleObj = GameplaySystems::GetGameObject(startUpTitleObjUID);
	GameObject* loopingTitleObj = GameplaySystems::GetGameObject(loopingTitleObjUID);
	GameObject* glitchTitleObj = GameplaySystems::GetGameObject(glitchTitleObjUID);
	GameObject* startPlayingTitleObj = GameplaySystems::GetGameObject(startPlayingTitleObjUID);
	GameObject* blackImageObj = GameplaySystems::GetGameObject(blackImageObjUID);

	if (blackImageObjUID) {
		canvasFader = GET_SCRIPT(blackImageObj, CanvasFader);
		if (canvasFader) {
			//blackImage->SetColor(alphaBlack);
			canvasFader->FadeIn();
		}
	}

	if (startUpTitleObj) {
		startUpTitleSpriteSheetPlayer = GET_SCRIPT(startUpTitleObj, UISpriteSheetPlayer);
	}

	if (loopingTitleObj) {
		loopingTitleSpriteSheetPlayer = GET_SCRIPT(loopingTitleObj, UISpriteSheetPlayer);
	}

	if (glitchTitleObj) {
		glitchTitleSpriteSheetPlayer = GET_SCRIPT(glitchTitleObj, UISpriteSheetPlayer);
		glitchDuration = glitchTitleSpriteSheetPlayer->CalcDuration();
	}

	if (startPlayingTitleObj) {
		startPlayingTitleSpriteSheetPlayer = GET_SCRIPT(startPlayingTitleObj, UISpriteSheetPlayer);
	}

	int i = 0;
	for (ComponentAudioSource& src : GetOwner().GetComponents<ComponentAudioSource>()) {
		if (i < static_cast<int>(GlitchTitleAudio::TOTAL)) audios[i] = &src;
		++i;
	}

}

void GlitchyTitleController::Update() {

	if (!startUpTitleSpriteSheetPlayer || !loopingTitleSpriteSheetPlayer || !glitchTitleSpriteSheetPlayer || !startPlayingTitleSpriteSheetPlayer)return;
	if (startPlayingTitleSpriteSheetPlayer->IsPlaying())return;

	switch (glitchState) {
	case GlitchState::FADE_IN:
		if (canvasFader) {

			if (!canvasFader->IsPlaying()) {
				glitchState = GlitchState::WAIT_START;
			}

		} else {
			//Error prevention
			glitchState = GlitchState::WAIT_START;
		}
		break;
	case GlitchState::WAIT_START:
		stateTimer += Time::GetDeltaTime();
		if (stateTimer >= fadeInTotalTime) {
			glitchState = GlitchState::WAIT_START_SPRITESHEET;
			startUpTitleSpriteSheetPlayer->Play();
			PlayAudio(GlitchTitleAudio::FADE_IN);
			stateTimer = 0;
		}
		break;
	case GlitchState::WAIT_START_SPRITESHEET:
		stateTimer += Time::GetDeltaTime();
		if (stateTimer >= startUpTitleSpriteSheetPlayer->CalcDuration()) {
			glitchState = GlitchState::START;
		}
		break;
	case GlitchState::START:
		StartStateIdle();
		break;
	case GlitchState::IDLE:
		if (timerBetweenGlitches <= 0) {
			loopingTitleSpriteSheetPlayer->Stop();
			glitchTitleSpriteSheetPlayer->Play();
			PlayAudio(GlitchTitleAudio::GLITCH);
			glitchState = GlitchState::GLITCH;
		}
		timerBetweenGlitches -= Time::GetDeltaTime();
		break;
	case GlitchState::GLITCH:
		//if (glitchTitleSpriteSheetPlayer->IsPlaying())return;

		if (glitchTimer > glitchDuration) {
			glitchTitleSpriteSheetPlayer->Stop();
			StartStateIdle();
			glitchTimer = 0.0f;
		} else {
			glitchTimer += Time::GetDeltaTime();
		}
		break;


		//When button gets pressed
	case GlitchState::PLAY:

		stateTimer = 0.0f;

		loopingTitleSpriteSheetPlayer->Stop();
		glitchTitleSpriteSheetPlayer->Stop();
		startPlayingTitleSpriteSheetPlayer->Stop();

		startPlayingTitleSpriteSheetPlayer->Play();

		glitchState = GlitchState::WAIT_FADE_OUT;
		break;
	case GlitchState::WAIT_FADE_OUT:
		stateTimer += Time::GetDeltaTime();
		if (stateTimer >= fadeInTotalTime) {
			canvasFader->FadeOut();
			glitchState = GlitchState::FADE_OUT;
		}
		break;
	case GlitchState::FADE_OUT:
		if (!canvasFader->IsPlaying()) {
			if (pressedButton) {
				pressedButton->DoTransition();
			}
		}
		break;
	}
}

void GlitchyTitleController::PressedPlay(StartTitleGlitchOnPlay* pressedButton_) {
	pressedButton = pressedButton_;
	glitchState = GlitchState::PLAY;

}

bool GlitchyTitleController::ReadyForTransition() const {
	return glitchState == GlitchState::FADE_OUT;
}

void GlitchyTitleController::StartStateIdle() {
	glitchState = GlitchState::IDLE;
	timerBetweenGlitches = minTimeForTimeGlitches + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (maxTimeForTimeBetweenGlitches - minTimeForTimeGlitches)));
	loopingTitleSpriteSheetPlayer->Stop();
	loopingTitleSpriteSheetPlayer->Play();
}

void GlitchyTitleController::PlayAudio(GlitchTitleAudio type) {
	if (audios[static_cast<int>(type)]) audios[static_cast<int>(type)]->Play();
}
