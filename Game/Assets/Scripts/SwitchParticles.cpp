#include "SwitchParticles.h"

#include "GameplaySystems.h"

EXPOSE_MEMBERS(SwitchParticles) {
	MEMBER(MemberType::GAME_OBJECT_UID, playerUID),
	MEMBER(MemberType::GAME_OBJECT_UID, mainParticuleUID)
};

GENERATE_BODY_IMPL(SwitchParticles);

void SwitchParticles::Start() {

	player = GameplaySystems::GetGameObject(playerUID);

	GameObject* effect = GameplaySystems::GetGameObject(mainParticuleUID);

	if (effect) {
		mainParticleSystem = effect->GetComponent<ComponentParticleSystem>();
	}

	
}

void SwitchParticles::Update() {
	if (!player || !mainParticleSystem) return;

	if (playing) {
		mainParticleSystem->PlayChildParticles();
		playing = false;
	}
	
}

void SwitchParticles::Play() {
	playing = true;
}
