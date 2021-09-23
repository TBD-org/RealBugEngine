#include "FullHealthBarFeedback.h"

#include "GameObject.h"
#include "Components/UI/ComponentImage.h"
#include "GameplaySystems.h"

EXPOSE_MEMBERS(FullHealthBarFeedback) {
    MEMBER(MemberType::GAME_OBJECT_UID, fillUID),
        MEMBER(MemberType::GAME_OBJECT_UID, strokeUID),
        MEMBER(MemberType::FLOAT, growingTime),
        MEMBER(MemberType::FLOAT, shrinkingTime),
        MEMBER(MemberType::FLOAT, growingScale)
};

GENERATE_BODY_IMPL(FullHealthBarFeedback);

void FullHealthBarFeedback::Start() {
	fill = GameplaySystems::GetGameObject(fillUID);
	stroke = GameplaySystems::GetGameObject(strokeUID);

	if (fill) {
		fillTransform = fill->GetComponent<ComponentTransform2D>();
		if (fillTransform) originalSize = fillTransform->GetSize();
	}
}

void FullHealthBarFeedback::Update() {
	if (!fill || !stroke || !fillTransform) return;
	if (playing) {
		if (growTimer < growingTime) {
			float delta = growTimer / growingTime;
			float scale = Lerp(1, growingScale, delta);
			fillTransform->SetSize(float2(originalSize.x * scale, originalSize.y * scale));
			growTimer += Time::GetDeltaTime();
		}
		else if (shrinkTimer < shrinkingTime) {
			float delta = shrinkTimer / shrinkingTime;
			float scale = Lerp(growingScale, 1, delta);
			fillTransform->SetSize(float2(originalSize.x * scale, originalSize.y * scale));
			shrinkTimer += Time::GetDeltaTime();
		}
		else {
			playing = false;
			shrinkTimer = 0.0f;
			growTimer = 0.0f;
		}
	}
}

void FullHealthBarFeedback::Play() {
	if (!fill || !stroke || !fillTransform) return;
	stroke->Disable();
	playing = true;
}

void FullHealthBarFeedback::Stop() {
	if (!fill || !stroke || !fillTransform) return;

	fillTransform->SetSize(originalSize);

	shrinkTimer = 0.0f;
	growTimer = 0.0f;

	playing = false;
}

void FullHealthBarFeedback::Reset() {
	if (!fill || !stroke || !fillTransform) return;

	fillTransform->SetSize(originalSize);
	stroke->Enable();

	shrinkTimer = 0.0f;
	growTimer = 0.0f;

	playing = false;
}
