#pragma once

#include "Scripting/Script.h"

class GameObject;
class DialogueManager;
class AfterDialogCallback;
class TriggerDialogue : public Script {
	GENERATE_BODY(TriggerDialogue);

public:
	void Start() override;
	void Update() override;
	void OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle = nullptr) override;

public:
	UID gameControllerUID = 0;
	UID afterDialogCallbackUID = 0;
	GameObject* gameController = nullptr;
	DialogueManager* dialogueManagerScript = nullptr;
	int dialogueID = 0;

private:
	AfterDialogCallback* afterDialogCallbackScript = nullptr;
};
