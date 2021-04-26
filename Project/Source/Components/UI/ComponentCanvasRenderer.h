#pragma once

#include "Components/Component.h"

class ComponentCanvasRenderer : public Component {
public:
	REGISTER_COMPONENT(ComponentCanvasRenderer, ComponentType::CANVASRENDERER, false);

	void Save(JsonValue jComponent) const override;
	void Load(JsonValue jComponent) override;
	void DuplicateComponent(GameObject& owner) override;

	void Render(const GameObject* gameObject) const; //Calls Draw on all images if one of its parents is a canvas

private:
	const GameObject* AnyParentHasCanvas(const GameObject* current) const; //Recursively iterates through parents to check if one of them contains a Canvas Component
};
