#include "PauseController.h"

#include "GameObject.h"
#include "GameplaySystems.h"

EXPOSE_MEMBERS(PauseController) {
	// Add members here to expose them to the engine. Example:
	// MEMBER(MemberType::BOOL, exampleMember1),
	// MEMBER(MemberType::PREFAB_RESOURCE_UID, exampleMember2),
	// MEMBER(MemberType::GAME_OBJECT_UID, exampleMember3)
};

GENERATE_BODY_IMPL(PauseController);

void PauseController::Start() {

	//Get audio sources
	int i = 0;

	for (ComponentAudioSource& src : GetOwner().GetComponents<ComponentAudioSource>()) {
		if (i < static_cast<int>(AudioPause::TOTAL)) audios[i] = &src;
		i++;
	}

	levelMusic = GetOwner().GetParent()->GetComponent<ComponentAudioSource>();
}

void PauseController::Update() {
	if (!hasEntered && isPause) {
		hasEntered = true;
		if (audios[static_cast<int>(AudioPause::OPEN)]) {
			audios[static_cast<int>(AudioPause::OPEN)]->Play();
		}

		if (audios[static_cast<int>(AudioPause::MUSIC)]) {
			audios[static_cast<int>(AudioPause::MUSIC)]->Play();
		}

		if (levelMusic && levelMusic->IsPlaying()) {
			levelMusic->Pause();
		}
	}

	if (hasEntered && !isPause) {
		hasEntered = false;
		if (audios[static_cast<int>(AudioPause::CLOSE)]) {
			audios[static_cast<int>(AudioPause::CLOSE)]->Play();
		}

		if (audios[static_cast<int>(AudioPause::MUSIC)]) {
			audios[static_cast<int>(AudioPause::MUSIC)]->Stop();
		}

		if (levelMusic) {
			levelMusic->Play();
		}
	}
}

void PauseController::SetIsPause(bool _isPause)
{
	isPause = _isPause;
}
