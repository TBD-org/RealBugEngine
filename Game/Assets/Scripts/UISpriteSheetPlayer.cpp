#include "UISpriteSheetPlayer.h"
#include "GameObject.h"
#include "GameplaySystems.h"
EXPOSE_MEMBERS(UISpriteSheetPlayer) {
	MEMBER(MemberType::FLOAT, secondsPerFrame),
	MEMBER(MemberType::BOOL, loops),
	MEMBER(MemberType::BOOL, playOnAwake)
};

GENERATE_BODY_IMPL(UISpriteSheetPlayer);

void UISpriteSheetPlayer::Start() {

	FillFrameVector();

	if (frames.size() > 0) {
		currentFrameObj = frames[0];
		for (int i = 0; i < frames.size(); i++) {
			frames[i]->Disable();
		}
	}
	if (playOnAwake) {
		Play();
	}
}

void UISpriteSheetPlayer::Update() {
	if (frames.size() == 0) return;
	if (!playing)return;


	if (animationTimer >= secondsPerFrame) {
		currentFrame++;
		animationTimer = 0.0f;

		if (currentFrameObj)
			currentFrameObj->Disable();

		if (currentFrame >= frames.size()) {
			currentFrame = 0;
			if (!loops) {
				playing = false;
				return;
			}
		}

		currentFrameObj = frames[currentFrame];

		if (currentFrameObj)
			currentFrameObj->Enable();
	}

	animationTimer += Time::GetDeltaTime();

}

void UISpriteSheetPlayer::Play() {
	if (!(frames.size() > 0)) return;
	animationTimer = 0;
	playing = true;

	currentFrame = 0;

	currentFrameObj = frames[currentFrame];
	if (currentFrameObj)
		currentFrameObj->Enable();
}

void UISpriteSheetPlayer::Stop() {

	if (currentFrameObj) {
		currentFrameObj->Disable();
	}

	currentFrame = 0;

	playing = false;
}

bool UISpriteSheetPlayer::IsPlaying() const {
	return playing;
}

float UISpriteSheetPlayer::CalcDuration() {
	if (frames.size() == 0) FillFrameVector();
	
	return frames.size() * secondsPerFrame;

}

void UISpriteSheetPlayer::FillFrameVector() {
	frames = GetOwner().GetChildren();
}



