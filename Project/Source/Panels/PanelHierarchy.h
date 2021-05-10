#pragma once

#include "Panel.h"

class GameObject;

class PanelHierarchy : public Panel {
public:
	PanelHierarchy();

	void Update() override;

private:
	void UpdateHierarchyNode(GameObject* gameObject);
	GameObject* CreateEmptyGameObject(GameObject* gameObject);
	GameObject* CreateEventSystem(GameObject* gameObject);
	GameObject* CreateUICanvas(GameObject* gameObject);
	GameObject* CreateUIImage(GameObject* gameObject);
	GameObject* CreateUIText(GameObject* gameObject);
	GameObject* CreateUIButton(GameObject* gameObject);
	GameObject* CreateUISlider(GameObject* gameObject);
	GameObject* CreateUIToggle(GameObject* gameObject);
	GameObject* CreatePartycleSystemObject(GameObject* gameObject);

private:
	int windowWidth = 0;
	int windowHeight = 0;
};
