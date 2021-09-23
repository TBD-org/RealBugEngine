#include "AfterDialogCallback.h"

#include "FactoryDoors.h"
#include "GameplaySystems.h"
#include "GameObject.h"
#include "DialogueManager.h"

EXPOSE_MEMBERS(AfterDialogCallback) {
	MEMBER(MemberType::GAME_OBJECT_UID, gameControllerUID),
	MEMBER(MemberType::GAME_OBJECT_UID, winConditionUID)
};

GENERATE_BODY_IMPL(AfterDialogCallback);

void AfterDialogCallback::Start() {
	GameObject* gameController = GameplaySystems::GetGameObject(gameControllerUID);
	if (gameController) dialogueManagerScript = GET_SCRIPT(gameController, DialogueManager);
	factoryDoorsScript = GET_SCRIPT(&GetOwner(), FactoryDoors);
}

void AfterDialogCallback::Update() {
	if (factoryDoorsScript && hasOpenedDialog && !dialogueManagerScript->HasActiveDialogue()) {
		factoryDoorsScript->Open();
		hasOpenedDialog = false;
		GameObject* winCondition = GameplaySystems::GetGameObject(winConditionUID);
		if (winCondition) {
			ComponentBoxCollider* boxCollider = winCondition->GetComponent<ComponentBoxCollider>();
			if (boxCollider) {
				boxCollider->Enable();
			}
		}
	}
}

void AfterDialogCallback::OpenFactoryDoors() {
	hasOpenedDialog = true;
}