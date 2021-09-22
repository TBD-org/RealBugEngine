#include "ComponentAudioSource.h"

#include "Globals.h"
#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleAudio.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleTime.h"
#include "Resources/ResourceAudioClip.h"
#include "Utils/ImGuiUtils.h"

#include "AL/al.h"
#include "debugdraw.h"
#include "Math/float3.h"

#include "Utils/Leaks.h"

#define JSON_TAG_PITCH "Pitch"
#define JSON_TAG_GAIN "Gain"
#define JSON_TAG_MUTE "Mute"
#define JSON_TAG_LOOPING "Looping"
#define JSON_TAG_PLAY_ON_AWAKE "PlayOnAwake"
#define JSON_TAG_AUDIO_CLIP_ID "AudioClipId"
#define JSON_TAG_SPATIAL_BLEND "SpatialBlend"
#define JSON_TAG_SOURCE_TYPE "SourceType"
#define JSON_TAG_INNER_ANGLE "InnerAngle"
#define JSON_TAG_OUTER_ANGLE "OuterAngle"
#define JSON_TAG_OUTER_GAIN "OuterGain"
#define JSON_TAG_ROLLOFF_FACTOR "RolloffFactor"
#define JSON_TAG_REFERENCE_DISTANCE "ReferenceDistance"
#define JSON_TAG_MAX_DISTANCE "MaxDistance"

ComponentAudioSource::~ComponentAudioSource() {
	Stop();

	App->resources->DecreaseReferenceCount(audioClipId);
}

void ComponentAudioSource::Init() {
	App->resources->IncreaseReferenceCount(audioClipId);

	isStarted = false;
	UpdateAudioSource();
}

void ComponentAudioSource::Update() {
	if (!isStarted && App->time->HasGameStarted() && playOnAwake) {
		Play();
		isStarted = true;
	}

	UpdateAudioSource();
	if (sourceId != 0 && IsStopped()) {
		Stop();
	}
}

void ComponentAudioSource::DrawGizmos() {
	if (IsActive() && drawGizmos) {
		if (spatialBlend && sourceType) {
			dd::cone(position, direction, dd::colors::White, 1.0f, 0.0f);
		} else {
			dd::sphere(position, dd::colors::White, 2.f);
		}
	}
}

void ComponentAudioSource::UpdateAudioSource() {
	ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
	float3 tempPos = transform->GetGlobalPosition();
	float3 tempDir = transform->GetGlobalRotation() * float3::unitZ;
	if (!tempPos.Equals(position) || !tempDir.Equals(direction)) {
		position = tempPos;
		direction = tempDir;

		if (!sourceId) return;

		if (spatialBlend) {
			alSourcefv(sourceId, AL_POSITION, position.ptr());
			if (sourceType) {
				alSourcefv(sourceId, AL_DIRECTION, direction.ptr());
			}
		}
	}
}

void ComponentAudioSource::OnEditorUpdate() {
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
	ImGui::Checkbox("Draw Gizmos", &drawGizmos);
	ImGui::Separator();
	ImGui::TextColored(App->editor->titleColor, "General Settings");

	ImGui::ResourceSlot<ResourceAudioClip>("AudioClip", &audioClipId, [this]() { Stop(); });

	if (ImGui::Checkbox("Mute", &mute)) {
		if (mute) {
			alSourcef(sourceId, AL_GAIN, 0.0f);
		} else {
			alSourcef(sourceId, AL_GAIN, gain);
		}
	}

	if (ImGui::Checkbox("Loop", &loop)) {
		alSourcef(sourceId, AL_LOOPING, loop);
	}
	if (ImGui::DragFloat("Gain", &gain, App->editor->dragSpeed3f, 0, 1)) {
		alSourcef(sourceId, AL_GAIN, gain);
	}
	if (ImGui::DragFloat("Pitch", &pitch, App->editor->dragSpeed3f, 0.5, 2)) {
		alSourcef(sourceId, AL_PITCH, pitch);
	}
	ImGui::Checkbox("Play On Awake", &playOnAwake);
	ImGui::Separator();

	ImGui::TextColored(App->editor->titleColor, "Position Settings");
	ImGui::Text("Spatial Blend");
	ImGui::SameLine();
	if (ImGui::RadioButton("2D", &spatialBlend, 0)) {
		alSourcei(sourceId, AL_SOURCE_RELATIVE, AL_TRUE);
		alSource3f(sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSource3f(sourceId, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("3D", &spatialBlend, 1)) {
		alSourcei(sourceId, AL_SOURCE_RELATIVE, AL_FALSE);
		alSourcefv(sourceId, AL_POSITION, position.ptr());
	}
	if (spatialBlend) { // 3D
		const char* sourceTypes[] = {"Omnidirectional", "Directional"};
		const char* sourceTypesCurrent = sourceTypes[sourceType];
		if (ImGui::BeginCombo("Source Type", sourceTypesCurrent)) {
			for (int n = 0; n < IM_ARRAYSIZE(sourceTypes); ++n) {
				bool isSelected = (sourceTypesCurrent == sourceTypes[n]);
				if (ImGui::Selectable(sourceTypes[n], isSelected)) {
					sourceType = n;
					if (sourceType) {
						alSourcefv(sourceId, AL_DIRECTION, direction.ptr());
					} else {
						alSourcef(sourceId, AL_CONE_INNER_ANGLE, 360);
						alSource3f(sourceId, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
					}
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (sourceType) { // Directional
			ImGui::Indent();
			if (ImGui::DragFloat("Inner Angle", &innerAngle, 1.f, 0.0f, outerAngle)) {
				alSourcef(sourceId, AL_CONE_INNER_ANGLE, innerAngle);
			}
			if (ImGui::DragFloat("Outer Angle", &outerAngle, 1.f, 0.0f, 360.f)) {
				alSourcef(sourceId, AL_CONE_OUTER_ANGLE, outerAngle);
			}
			if (ImGui::DragFloat("Outer Gain", &outerGain, App->editor->dragSpeed2f, 0.f, 1.f)) {
				alSourcef(sourceId, AL_CONE_OUTER_GAIN, outerGain);
			}
			ImGui::Unindent();
		}

		if (ImGui::DragFloat("Roll Off Factor", &rollOffFactor, 1.f, 0.0f, inf)) {
			alSourcef(sourceId, AL_ROLLOFF_FACTOR, rollOffFactor);
		}
		if (ImGui::DragFloat("Reference Distance", &referenceDistance, 1.f, 0.0f, inf)) {
			alSourcef(sourceId, AL_REFERENCE_DISTANCE, referenceDistance);
		}
		if (ImGui::DragFloat("Max Distance", &maxDistance, 1.f, 0.0f, inf)) {
			alSourcef(sourceId, AL_MAX_DISTANCE, maxDistance);
		}
	}

	ImGui::Separator();

	if (ImGui::Button("Play")) {
		Play();
	}
	ImGui::SameLine();
	if (ImGui::Button("Pause")) {
		Pause();
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop")) {
		Stop();
	}
}

void ComponentAudioSource::UpdateSourceParameters() {
	ResourceAudioClip* audioResource = App->resources->GetResource<ResourceAudioClip>(audioClipId);
	if (audioResource == nullptr) return;

	// EDIT HERE - check if resource exists, else get buffer

	alSourcef(sourceId, AL_PITCH, pitch);
	alSourcei(sourceId, AL_LOOPING, loop);
	alSourcei(sourceId, AL_BUFFER, audioResource->alBuffer);

	if (!spatialBlend) {
		alSourcei(sourceId, AL_SOURCE_RELATIVE, AL_TRUE);
		alSource3f(sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSource3f(sourceId, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
	} else {
		alSourcei(sourceId, AL_SOURCE_RELATIVE, AL_FALSE);
		alSourcefv(sourceId, AL_POSITION, position.ptr());

		if (sourceType) {
			alSourcefv(sourceId, AL_DIRECTION, direction.ptr());
			alSourcef(sourceId, AL_CONE_INNER_ANGLE, innerAngle);
			alSourcef(sourceId, AL_CONE_OUTER_ANGLE, outerAngle);
			alSourcef(sourceId, AL_CONE_OUTER_GAIN, outerGain);
		} else {
			alSourcef(sourceId, AL_CONE_INNER_ANGLE, 360);
			alSource3f(sourceId, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
		}
	}
	if (mute) {
		alSourcef(sourceId, AL_GAIN, 0.0f);
	} else {
		alSourcef(sourceId, AL_GAIN, gain);
	}
	alSourcef(sourceId, AL_ROLLOFF_FACTOR, rollOffFactor);
	alSourcef(sourceId, AL_REFERENCE_DISTANCE, referenceDistance);
	alSourcef(sourceId, AL_MAX_DISTANCE, maxDistance);
}

void ComponentAudioSource::Play() {
	if (IsActive()) {
		sourceId = App->audio->GetAvailableSource();
		if (sourceId) {
			UpdateSourceParameters();
			alSourcePlay(sourceId);
		}
	}
}

void ComponentAudioSource::Stop() {
	if (sourceId) {
		alSourceStop(sourceId);
		alSourcei(sourceId, AL_BUFFER, NULL);
		sourceId = 0;
	}
}

void ComponentAudioSource::Pause() const {
	if (IsPlaying()) {
		alSourcePause(sourceId);
	}
}

bool ComponentAudioSource::IsPlaying() const {
	ALint state;
	alGetSourcei(sourceId, AL_SOURCE_STATE, &state);
	return (state == AL_PLAYING);
}

bool ComponentAudioSource::IsPaused() const {
	ALint state;
	alGetSourcei(sourceId, AL_SOURCE_STATE, &state);
	return (state == AL_PAUSED);
}

bool ComponentAudioSource::IsStopped() const {
	ALint state;
	alGetSourcei(sourceId, AL_SOURCE_STATE, &state);
	return (state == AL_STOPPED || state == AL_INITIAL);
}

void ComponentAudioSource::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_PITCH] = pitch;
	jComponent[JSON_TAG_GAIN] = gain;
	jComponent[JSON_TAG_MUTE] = mute;
	jComponent[JSON_TAG_LOOPING] = loop;
	jComponent[JSON_TAG_PLAY_ON_AWAKE] = playOnAwake;
	jComponent[JSON_TAG_AUDIO_CLIP_ID] = audioClipId;
	jComponent[JSON_TAG_SPATIAL_BLEND] = spatialBlend;
	jComponent[JSON_TAG_SOURCE_TYPE] = sourceType;
	jComponent[JSON_TAG_INNER_ANGLE] = innerAngle;
	jComponent[JSON_TAG_OUTER_ANGLE] = outerAngle;
	jComponent[JSON_TAG_OUTER_GAIN] = outerGain;
	jComponent[JSON_TAG_ROLLOFF_FACTOR] = rollOffFactor;
	jComponent[JSON_TAG_REFERENCE_DISTANCE] = referenceDistance;
	jComponent[JSON_TAG_MAX_DISTANCE] = maxDistance;
}

void ComponentAudioSource::Load(JsonValue jComponent) {
	pitch = jComponent[JSON_TAG_PITCH];
	gain = jComponent[JSON_TAG_GAIN];
	mute = jComponent[JSON_TAG_MUTE];
	loop = jComponent[JSON_TAG_LOOPING];
	playOnAwake = jComponent[JSON_TAG_PLAY_ON_AWAKE];
	audioClipId = jComponent[JSON_TAG_AUDIO_CLIP_ID];
	spatialBlend = jComponent[JSON_TAG_SPATIAL_BLEND];
	sourceType = jComponent[JSON_TAG_SOURCE_TYPE];
	innerAngle = jComponent[JSON_TAG_INNER_ANGLE];
	outerAngle = jComponent[JSON_TAG_OUTER_ANGLE];
	outerGain = jComponent[JSON_TAG_OUTER_GAIN];
	rollOffFactor = jComponent[JSON_TAG_ROLLOFF_FACTOR];
	referenceDistance = jComponent[JSON_TAG_REFERENCE_DISTANCE];
	maxDistance = jComponent[JSON_TAG_MAX_DISTANCE];
}

// --- GETTERS ---

bool ComponentAudioSource::GetMute() const {
	return mute;
}

bool ComponentAudioSource::GetLoop() const {
	return loop;
}

float ComponentAudioSource::GetGain() const {
	return gain;
}

float ComponentAudioSource::GetPitch() const {
	return pitch;
}

bool ComponentAudioSource::GetPlayOnAwake() const {
	return playOnAwake;
}

int ComponentAudioSource::GetSpatialBlend() const {
	return spatialBlend;
}

int ComponentAudioSource::GetSourceType() const {
	return sourceType;
}

float ComponentAudioSource::GetInnerAngle() const {
	return innerAngle;
}

float ComponentAudioSource::GetOuterAngle() const {
	return outerAngle;
}

float ComponentAudioSource::GetOuterGain() const {
	return outerGain;
}

float ComponentAudioSource::GetRollOffFactor() const {
	return rollOffFactor;
}

float ComponentAudioSource::GetReferenceDistance() const {
	return referenceDistance;
}

float ComponentAudioSource::GetMaxDistance() const {
	return maxDistance;
}

// --- SETTERS ---

void ComponentAudioSource::SetMute(bool _mute) {
	mute = _mute;
}

void ComponentAudioSource::SetLoop(bool _loop) {
	loop = _loop;
}

void ComponentAudioSource::SetGain(float _gain) {
	gain = _gain;
}

void ComponentAudioSource::SetPitch(float _pitch) {
	pitch = _pitch;
}

void ComponentAudioSource::SetPlayOnAwake(bool _playOnAwake) {
	playOnAwake = _playOnAwake;
}

void ComponentAudioSource::SetSpatialBlend(int _spatialBlend) {
	spatialBlend = _spatialBlend;
}

void ComponentAudioSource::SetSourceType(int _sourceType) {
	sourceType = _sourceType;
}

void ComponentAudioSource::SetInnerAngle(float _innerAngle) {
	innerAngle = _innerAngle;
}

void ComponentAudioSource::SetOuterAngle(float _outerAngle) {
	outerAngle = _outerAngle;
}

void ComponentAudioSource::SetOuterGain(float _outerGain) {
	outerGain = _outerGain;
}

void ComponentAudioSource::SetRollOffFactor(float _rollOffFactor) {
	rollOffFactor = _rollOffFactor;
}

void ComponentAudioSource::SetReferenceDistance(float _referenceDistance) {
	referenceDistance = _referenceDistance;
}

void ComponentAudioSource::SetMaxDistance(float _maxDistance) {
	maxDistance = _maxDistance;
}
