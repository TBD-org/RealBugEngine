#pragma once

#include "Scripting/Script.h"

class FactoryDoors;
class DialogueManager;

class AfterDialogCallback : public Script
{
	GENERATE_BODY(AfterDialogCallback);

public:
	void Start() override;
	void Update() override;
	void OpenFactoryDoors();

public:
	UID gameControllerUID = 0;
	UID winConditionUID = 0;

private:
	bool hasOpenedDialog = false;
	DialogueManager* dialogueManagerScript = nullptr;
	FactoryDoors* factoryDoorsScript = nullptr;
};

