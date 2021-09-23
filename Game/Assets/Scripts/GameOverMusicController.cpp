#include "GameOverMusicController.h"

#include "GameplaySystems.h"

EXPOSE_MEMBERS(GameOverMusicController) {
	// Add members here to expose them to the engine. Example:
	// MEMBER(MemberType::FLOAT, example1),
};

GENERATE_BODY_IMPL(GameOverMusicController);

void GameOverMusicController::Start() {
	gameOverMusic = GetOwner().GetComponent<ComponentAudioSource>();
	levelMusic = GetOwner().GetParent()->GetComponent<ComponentAudioSource>();

	deathScreen = GameplaySystems::GetGameObject("DeathScreen");
}

void GameOverMusicController::Update() {
	if (hasEntered) return;

	if (deathScreen && deathScreen->IsActive()) {
		if (gameOverMusic) gameOverMusic->Play();
		if (levelMusic) levelMusic->Stop();
		hasEntered = true;
	}
}