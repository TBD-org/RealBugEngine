#include "ParticleSystemManager.h"

#include "GameObject.h"

EXPOSE_MEMBERS(ParticleSystemManager) {
};

GENERATE_BODY_IMPL(ParticleSystemManager);

void ParticleSystemManager::Start() {
	
	GameObject* effect = &GetOwner();

	if (effect) {
		mainParticleSystem = effect->GetComponent<ComponentParticleSystem>();
	}
	
	if (mainParticleSystem) {
		mainParticleSystem->PlayChildParticles();
	}

}

void ParticleSystemManager::Update() {
	
}