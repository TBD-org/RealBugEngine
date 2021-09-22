#pragma once
#include "Component.h"

#include "ComponentParticleSystem.h"

#include "Math/float3.h"
#include "Math/float4x4.h"

class ImGradient;
struct ImGradientMark;

class ComponentBillboard : public Component {
public:
	REGISTER_COMPONENT(ComponentBillboard, ComponentType::BILLBOARD, false);
	~ComponentBillboard();

	void Init() override;
	void Update() override;

	void OnEditorUpdate() override;
	void Load(JsonValue jComponent) override;
	void Save(JsonValue jComponent) const override;
	void Draw();

	void ResetColor();

private:
	UID textureID = 0; // ID of the image

	BillboardType billboardType = BillboardType::NORMAL;

	float4x4 modelStretch = float4x4::identity;
	float3 initPos = float3::zero;
	float3 previousPos = float3::zero;
	float3 direction = float3::zero;

	float currentFrame = 0.0f;
	float colorFrame = 0.0f;

	// General Options
	bool isRandomFrame = false;

	// Texture Sheet Animation
	unsigned Xtiles = 1;
	unsigned Ytiles = 1;
	float animationSpeed = 0.0f;

	// Color over Lifetime
	bool colorOverLifetime = false;
	float colorLifetime = 10.0f;
	ImGradient* gradient = nullptr;
	ImGradientMark* draggingGradient = nullptr;
	ImGradientMark* selectedGradient = nullptr;

	// Texture Options
	bool flipTexture[2] = {false, false};
};
