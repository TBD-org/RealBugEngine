#pragma once

#include "Components/Component.h"

#include <vector>

class GameObject;
class ComponentSelectable;

class ComponentEventSystem : public Component {
public:
	REGISTER_COMPONENT(ComponentEventSystem, ComponentType::EVENT_SYSTEM, false);
	~ComponentEventSystem();

	void Start() override;
	void Update() override;
	void OnEditorUpdate() override;
	void Save(JsonValue jComponent) const override;
	void Load(JsonValue jComponent) override;
	void OnEnable() override;
	void OnDisable() override;

	void SetSelected(UID newSelectableComponentId);
	void SetSelected(ComponentSelectable* newSelectable);
	void EnteredPointerOnSelectable(ComponentSelectable* newHoveredComponent);	//Interface implementation
	void ExitedPointerOnSelectable(ComponentSelectable* newUnHoveredComponent); //Interface implementation
	ComponentSelectable* GetCurrentSelected() const;							//Returns currently selected ComponentSelectable
	TESSERACT_ENGINE_API ComponentSelectable* GetCurrentlyHovered() const;		//Returns last Selectable that was hovered over with mouse
	void SetClickedGameObject(GameObject* clickedObj_);
	GameObject* GetClickedGameObject();

private:
	UID selectedId = 0; //Currently selected SelectableComponent*
	GameObject* clickedObj = nullptr;
	UID hoveredSelectableID = 0;
	bool started = false;

	float navigationTimer = 0.0f;
	float timeBetweenNavigations = 0.2f;

public:
	UID firstSelectedId = 0; //Reference to the "first selected selectableComponent", this is not used directly but Unity implements it so that users can access it
};
