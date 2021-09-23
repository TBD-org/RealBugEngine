#include "HealthLostInstantFeedback.h"
#include "GameplaySystems.h"

#include "Components/UI/ComponentImage.h"
#include "Components/UI/ComponentTransform2D.h"

EXPOSE_MEMBERS(HealthLostInstantFeedback) {
	MEMBER(MemberType::FLOAT, growthDuration),
		MEMBER(MemberType::FLOAT, growthMaxScale),
		MEMBER(MemberType::FLOAT, fadeOutDuration),
		MEMBER(MemberType::GAME_OBJECT_UID, imageUID)

};

GENERATE_BODY_IMPL(HealthLostInstantFeedback);

void HealthLostInstantFeedback::Play() {
	if (image) {
		image->Enable();
		image->SetColor(float4(1, 1, 1, 1));
	}
	if (transform) transform->SetSize(float2(originalSize.x, originalSize.y));
	playing = true;
	growthTimer = 0;
	fadeOutTimer = 0;
}

void HealthLostInstantFeedback::Stop() {
	if (image) {
		image->Disable();
		image->SetColor(float4(1, 1, 1, 1));
	}
	if (transform) transform->SetSize(float2(originalSize.x, originalSize.y));
	playing = false;
	growthTimer = 0;
	fadeOutTimer = 0;
}

void HealthLostInstantFeedback::Start() {
	GameObject* obj = GameplaySystems::GetGameObject(imageUID);

	if (obj) {
		image = obj->GetComponent<ComponentImage>();
		transform = obj->GetComponent <ComponentTransform2D>();
		originalSize = transform->GetSize();
	}

	if (image) {
		image->Disable();
	}

	std::vector<GameObject*>ownerChildren = GetOwner().GetChildren();
	if (ownerChildren.size() > 0) {
		ComponentImage* fillImage = ownerChildren[1]->GetComponent<ComponentImage>();
		if (fillImage) {
			originalColor = fillImage->GetColor();
		}
	}
}

void HealthLostInstantFeedback::Update() {

	if (!transform || !image)return;

	if (playing) {
		if (growthTimer < growthDuration) {
			growthTimer = Min(growthTimer + Time::GetDeltaTime(), growthDuration);

			float delta = growthTimer / growthDuration;

			float scaleValue = Lerp(1, growthMaxScale, delta);
			transform->SetSize(float2(originalSize.x * scaleValue, originalSize.y * scaleValue));
			image->SetColor(float4(originalColor.xyz(), 1));

			//Grow
		} else if (fadeOutTimer < fadeOutDuration) {
			fadeOutTimer = Min(fadeOutTimer + Time::GetDeltaTime(), fadeOutDuration);

			float delta = fadeOutTimer / fadeOutDuration;
			float fadeValue = Lerp(1, 0, delta);

			image->SetColor(float4(originalColor.xyz(), fadeValue));
			//FadeOut
		} else {
			playing = false;
			image->Disable();
			//Disable
		}
	}
}