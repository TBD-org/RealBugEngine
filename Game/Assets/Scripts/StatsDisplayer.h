#pragma once

#include "Scripting/Script.h"

class GameObject;
class ComponentText;

class StatsDisplayer : public Script
{
	GENERATE_BODY(StatsDisplayer);

public:

	void Start() override;
	void Update() override;
	void SetPanelActive(bool value);

public:
	UID fangUID = 0;
	UID canvasUID = 0;
	UID canvasTextsUID = 0;

private:
	GameObject* canvas = nullptr;
	GameObject* canvasTexts = nullptr;
	GameObject* player = nullptr;

	ComponentText* fpsText = nullptr;
	ComponentText* trigText = nullptr;
	ComponentText* positionText = nullptr;
	ComponentText* cameraText = nullptr;

	bool panelActive = false;
};

