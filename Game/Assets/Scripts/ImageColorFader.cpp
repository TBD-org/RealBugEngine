#include "ImageColorFader.h"

#include "GameplaySystems.h"
#include "Components/UI/ComponentImage.h"

EXPOSE_MEMBERS(ImageColorFader) {
	MEMBER(MemberType::FLOAT3, originalColor),
	MEMBER(MemberType::FLOAT, originalAlpha),
	MEMBER(MemberType::FLOAT3, targetColor),
	MEMBER(MemberType::FLOAT, targetAlpha),
	MEMBER(MemberType::FLOAT, totalTransitionTime),
	MEMBER(MemberType::BOOL, disableOnFinish),
	MEMBER(MemberType::GAME_OBJECT_UID, imageHoldingObjectUID),
	MEMBER(MemberType::BOOL, debugPlay)
};

GENERATE_BODY_IMPL(ImageColorFader);

void ImageColorFader::Start() {
	GameObject* imageHoldingObject = GameplaySystems::GetGameObject(imageHoldingObjectUID);
	if (imageHoldingObject) {
		targetImage = imageHoldingObject->GetComponent<ComponentImage>();
	}

	if (startOnAwake) {
		Play();
	}

}

void ImageColorFader::Update() {
	if (!targetImage)return;

	if (debugPlay) {
		Play();
		debugPlay = false;
	}

	if (!running)return;
	if (transitionTime < totalTransitionTime) {
		UpdateImageColor();
		transitionTime = Clamp(transitionTime + Time::GetDeltaTime(), 0.0f, totalTransitionTime);
	} else {
		Stop();
	}
}

void ImageColorFader::Play() {
	if (!targetImage) return;
	targetImage->GetOwner().Enable();
	transitionTime = 0;
	float4 imageColor = targetImage->GetColor();

	targetColor.x >= 0 ? targetColorToUse.x = targetColor.x : imageColor.x;
	targetColor.y >= 0 ? targetColorToUse.y = targetColor.y : imageColor.y;
	targetColor.z >= 0 ? targetColorToUse.z = targetColor.z : imageColor.z;
	targetAlpha >= 0 ? targetColorToUse.w = targetAlpha : imageColor.w;

	originalColor.x >= 0 ? originalColorToUse.x = originalColor.x : imageColor.x;
	originalColor.y >= 0 ? originalColorToUse.y = originalColor.y : imageColor.y;
	originalColor.z >= 0 ? originalColorToUse.z = originalColor.z : imageColor.z;
	originalAlpha >= 0 ? originalColorToUse.w = originalAlpha : imageColor.w;
	running = true;
}

void ImageColorFader::Stop() {
	if (targetImage) {
		targetImage->SetColor(targetColorToUse);
		if (disableOnFinish) {
			targetImage->GetOwner().Disable();
		}
	}
	running = false;
}

//This method assumes targetImage is not null
void ImageColorFader::UpdateImageColor() {
	float delta = transitionTime / totalTransitionTime;
	targetImage->SetColor(float4::Lerp(originalColorToUse, targetColorToUse, delta));
}
