#include "PlayMusicSript.h"

#include "GameObject.h"
#include "GameplaySystems.h"

EXPOSE_MEMBERS(PlayMusicSript) {
	// Add members here to expose them to the engine. Example:
	// MEMBER(MemberType::BOOL, exampleMember1),
	// MEMBER(MemberType::PREFAB_RESOURCE_UID, exampleMember2),
	// MEMBER(MemberType::GAME_OBJECT_UID, exampleMember3)

	MEMBER(MemberType::GAME_OBJECT_UID, skullUID),
};

GENERATE_BODY_IMPL(PlayMusicSript);

void PlayMusicSript::Start() {
	skull = GameplaySystems::GetGameObject(skullUID);
}

void PlayMusicSript::Update() {
	ComponentAudioSource* audioSource = skull->GetComponent<ComponentAudioSource>();
	ComponentParticleSystem* particle = skull->GetComponent<ComponentParticleSystem>();
	if (!audioSource) return;
	if (Input::GetMouseButtonDown(0)) {
		audioSource->Play();
		particle->Play();
		Debug::Log("Play");
	}
}