#include "ComponentAudioListener.h"

#include "GameObject.h"
#include "Utils/Logging.h"

#include "AL/al.h"
#include "imgui.h"

#include "Utils/Leaks.h"

#define JSON_TAG_GAIN "Gain"

void ComponentAudioListener::Init() {
	alListenerf(AL_GAIN, gain);
	UpdateAudioListener();
}

void ComponentAudioListener::Update() {
	UpdateAudioListener();
}

void ComponentAudioListener::OnEditorUpdate() {
	if (ImGui::Checkbox("Active", &active)) {
		if (GetOwner().IsActive()) {
			if (active) {
				Enable();
			} else {
				Disable();
			}
		}
	}
	ImGui::Separator();
}

void ComponentAudioListener::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_GAIN] = gain;
}

void ComponentAudioListener::Load(JsonValue jComponent) {
	gain = jComponent[JSON_TAG_GAIN];
}

void ComponentAudioListener::OnEnable() {
	Init();
}

void ComponentAudioListener::OnDisable() {
	alListenerf(AL_GAIN, 0.f);
}

void ComponentAudioListener::UpdateAudioListener() {
	// For now, the AudioListener will follow the direction / position of the Camera attached to the GameObject
	ComponentCamera* camera = GetOwner().GetComponent<ComponentCamera>();
	if (camera == nullptr) {
		LOG("Warning: AudioListener has to be attached to a GameObject with a Camera Component");
		alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
		alListener3f(AL_ORIENTATION, 0.0f, 0.0f, 0.0f);
		return;
	}

	Frustum* frustum = camera->GetFrustum();
	float3 position = frustum->Pos();
	float3 front = frustum->Front();
	float3 up = frustum->Up();
	float orientation[6] {
		front[0],
		front[1],
		front[2],
		up[0],
		up[1],
		up[2],
	};
	alListenerfv(AL_POSITION, position.ptr());
	alListenerfv(AL_ORIENTATION, orientation);
}
