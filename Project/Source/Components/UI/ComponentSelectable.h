#pragma once

#include "Components/Component.h"
#include "UI/Interfaces/IPointerEnterHandler.h"
#include "UI/Interfaces/IPointerExitHandler.h"

#include "Math/float4.h"

class ComponentEventSystem;

// TODO: DECIDE WETHER WE ARE GOING TO USE DIFFERENT TRANSITION_TYPES SUCH AS ANIMATIONS, AND IF SO, IMPLEMENT ANIMATIONS AS TRANSITION

class ComponentSelectable : public Component
	, IPointerEnterHandler
	, IPointerExitHandler {
public:
	enum class NavigationType {
		NONE,
		AUTOMATIC,
		MANUAL
	};

	enum class TransitionType {
		NONE,
		COLOR_CHANGE,
		ANIMATION,
	};

public:
	REGISTER_COMPONENT(ComponentSelectable, ComponentType::SELECTABLE, false);
	~ComponentSelectable();

	bool IsInteractable() const;
	void SetInteractable(bool interactable_);
	ComponentSelectable* FindSelectableOnDir(float2 dir);

	virtual void OnSelect();
	virtual void OnDeselect();

	void Init() override;
	void Update() override;
	void OnEditorUpdate() override;
	void OnEnable() override;
	void OnDisable() override;
	bool IsHovered() const;
	void SetHovered(bool hovered_);
	bool IsSelected() const;
	void Save(JsonValue jsonVal) const override;
	void Load(JsonValue jsonVal) override;

	// Inherited vía IPointerEnterHandler
	virtual void OnPointerEnter() override;
	virtual void OnPointerExit() override;
	bool CanBeRemoved() const override; //This override returns false if the GameObject holds a ComponentButton/ComponentToggle

	Component* GetSelectableComponent();
	void SetSelectableType(ComponentType type_); // Sets the enum that hints which other component (that would inherit from Selectable) is contained within the same GameObject
	const float4 GetDisabledColor() const;		 // Returns colorDisabled
	const float4 GetHoverColor() const;			 // Returns colorHovered
	const float4 GetSelectedColor() const;		 // Returns colorSelected
	TransitionType GetTransitionType() const;
	void TryToClickOn(bool internalCall) const; //internalCall means if the buttons is supposed to show it's been pressed (false) or if its supposed to actually do whatever it does when pressed (true)
	bool IsClicked() const;

public:
	UID onAxisUp = 0;
	UID onAxisDown = 0;
	UID onAxisLeft = 0;
	UID onAxisRight = 0;

	ComponentType selectableType = ComponentType::UNKNOWN;

protected:
	bool interactable = true;									  //Can this ComponentSelectable be interacted
	bool selected = false;										  //Is this ComponentSelectable selected
	bool hovered = false;										  //Is this ComponentSelectable hovered by mouse
	NavigationType navigationType = NavigationType::AUTOMATIC;	  //Navigation can be user-explicit, automatic based on 2D axis, or non-existing
	TransitionType transitionType = TransitionType::COLOR_CHANGE; //Transition for selected/hovered can be managed in different ways
	float4 colorDisabled = float4(0.73f, 0.73f, 0.73f, 1.f);	  // The color when the button is disabled
	float4 colorHovered = float4(0.84f, 0.84f, 0.84f, 1.f);		  // The color when the button is hovered
	float4 colorSelected = float4(0.5f, 0.5f, 0.5f, 1.f);		  // The color when the button is hovered
};
