#pragma once

#include "Components/Component.h"
#include "UI/Interfaces/IMouseClickHandler.h"

#include "Math/float4.h"

enum class DirectionType {
	LEFT_TO_RIGHT,
	RIGHT_TO_LEFT
	// TODO
	// BOTTOM_TO_TOP,
	// TOP_TO_BOTTOM
};

class ComponentSlider : public Component
	, public IMouseClickHandler {
public:
	REGISTER_COMPONENT(ComponentSlider, ComponentType::SLIDER, false);

	void Init() override; // Component Initialization
	void Update() override;
	void OnEditorUpdate() override; // Input for class variables

	void OnClicked() override;
	void OnClickedInternal() override;

	TESSERACT_ENGINE_API void OnValueChanged(bool innerCall = false);
	void OnSliderDragged();

	void Save(JsonValue jComponent) const override; // Serializes the component
	void Load(JsonValue jComponent) override;		// Deserializes the component

	TESSERACT_ENGINE_API bool IsClicked() const; // Returns true if the button is clicked
	TESSERACT_ENGINE_API void SetClicked(bool clicked_); // Sets clicked to the value

	TESSERACT_ENGINE_API float4 GetTintColor() const; // Returns the correspondant color of the current state
	TESSERACT_ENGINE_API float4 GetClickColor() const; // Returns colorClicked
	TESSERACT_ENGINE_API float2 GetClickedPosition() const; // Returns mouse click position inside the slider

	UID GetHandleID() const;

	TESSERACT_ENGINE_API float GetCurrentValue() const;
	TESSERACT_ENGINE_API float GetMaxValue() const;
	TESSERACT_ENGINE_API float GetMinValue() const;
	TESSERACT_ENGINE_API float GetNormalizedValue() const;
	TESSERACT_ENGINE_API void ModifyValue(float multiplier);

	TESSERACT_ENGINE_API void ChangeNormalizedValue(float normalizedValue_); // IMPORTANT!!! Right now this method its only used in Gameplay

public:
	bool handleStopsOnEdge = false;
	bool beingHandled = false;

private:
	void SetNormalizedValue();
	void SearchForMissingGameObjectReferences();

private:
	GameObject* background = nullptr;
	GameObject* fill = nullptr;
	GameObject* handle = nullptr;

	float maxValue = 100.0f;
	float minValue = 0.0f;

	float currentValue = 50.0f;	  // Current value between maxValue and minValue
	float normalizedValue = 0.5f; // Normalized current value between 0 and 1

	float2 newPosition = float2(0, 0); // Click position inside the slider

	float sliderSensitivity = 100.0f;							// Speed at which the slider's value will increase via keyboard/game controller input
	bool clicked = false;										// Clicked state
	float4 colorClicked = float4(0.64f, 0.64f, 0.64f, 1.f);		// The color when the button is clicked
	float4 colorManualInput = float4(0.24f, 0.24f, 0.24f, 1.f); // The color when the button is clicked

	DirectionType direction = DirectionType::LEFT_TO_RIGHT; // Slider direction
};
