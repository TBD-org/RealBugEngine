#pragma once

#include "Component.h"
#include "Utils/UID.h"

class ComponentSkyBox : public Component {
public:
	REGISTER_COMPONENT(ComponentSkyBox, ComponentType::SKYBOX, false);
	~ComponentSkyBox();

	void Init() override;
	void OnEditorUpdate() override;
	void Load(JsonValue jComponent) override;
	void Save(JsonValue jComponent) const override;
	void Draw();

	UID GetSkyboxResourceID();

public:
	float strength = 1.0f;

private:
	UID shaderId = 0;
	UID skyboxId = 0;
};
