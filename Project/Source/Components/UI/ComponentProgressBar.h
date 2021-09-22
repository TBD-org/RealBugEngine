#pragma once

#include "Components/Component.h"

#include "Math/float3.h"
#include "Math/float2.h"

enum class FillDirection {
	LEFT_TO_RIGHT,
	RIGHT_TO_LEFT,
	BOTTOM_TO_TOP,
	TOP_TO_BOTTOM
};

class ComponentTransform2D;

// Component that renders an Image on a Quad
class ComponentProgressBar : public Component {
public:
	REGISTER_COMPONENT(ComponentProgressBar, ComponentType::PROGRESS_BAR, false);

	void Init() override;
	void Update() override;							// Update
	void OnEditorUpdate() override;					// Input for parameters
	void Save(JsonValue jComponent) const override; // Serializes object
	void Load(JsonValue jComponent) override;		// Deserializes object

	TESSERACT_ENGINE_API void SetValue(float v);
	TESSERACT_ENGINE_API void SetFillPos(float fillPos);
	TESSERACT_ENGINE_API void SetMin(float m);
	TESSERACT_ENGINE_API void SetMax(float n);
	TESSERACT_ENGINE_API float GetValue();
	TESSERACT_ENGINE_API float GetFillPos();
	TESSERACT_ENGINE_API float GetMin();
	TESSERACT_ENGINE_API float GetMax();

public:
	float value = .5f;

private:
	GameObject* background = nullptr;
	GameObject* fill = nullptr;

	ComponentTransform2D* rectBack = nullptr;
	ComponentTransform2D* rectFill = nullptr;

	UID fillID = 0;							
	UID backgroundID = 0;	

	FillDirection dir = FillDirection::LEFT_TO_RIGHT;

	float3 backPos = float3::zero;
	float2 backSize = float2::zero;

	float fillXPos = 0.0f;
	float min = 0.0f;
	float max = 1.0f;

	int dirIndex = 0;

	inline static const char* fillDirections[] {"Left to right", "Right to Left", "Bottom to top", "Top to bottom"};
};
