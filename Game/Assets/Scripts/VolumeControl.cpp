#include "VolumeControl.h"

#include "GameObject.h"
#include "GameplaySystems.h"
#include "Components/ComponentAudioListener.h"
#include "Components/UI/ComponentSlider.h"


EXPOSE_MEMBERS(VolumeControl) {
    // Add members here to expose them to the engine. Example:
	MEMBER(MemberType::GAME_OBJECT_UID, sliderUID),
	MEMBER(MemberType::GAME_OBJECT_UID, gameCameraUID),
    
};

GENERATE_BODY_IMPL(VolumeControl);

void VolumeControl::Start() {
	sliderGO = GameplaySystems::GetGameObject(sliderUID);
	gameCamera = GameplaySystems::GetGameObject(gameCameraUID);
	if (sliderGO && gameCamera) {
		ComponentSlider* comSlider = sliderGO->GetComponent<ComponentSlider>();
		ComponentAudioListener* comAudioLis = gameCamera->GetComponent<ComponentAudioListener>();
		if (comSlider && comAudioLis) {
			float volume = comSlider->GetNormalizedValue();
			comAudioLis->SetAudioVolume(volume);
		}
	}
}

void VolumeControl::Update() {
	
}

void VolumeControl::OnValueChanged() {
	if (!sliderGO || !gameCamera) return;
	ComponentSlider* comSlider = sliderGO->GetComponent<ComponentSlider>();
	ComponentAudioListener* comAudioLis = gameCamera->GetComponent<ComponentAudioListener>();
	if (!comSlider || !comAudioLis) return;

	float volume = comSlider->GetNormalizedValue();
	comAudioLis->SetAudioVolume(volume);
}