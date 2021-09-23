#include "TriggerDialogue.h"

#include "DialogueManager.h"
#include "GameplaySystems.h"
#include "GameObject.h"
#include "AfterDialogCallback.h"

EXPOSE_MEMBERS(TriggerDialogue) {
    MEMBER(MemberType::GAME_OBJECT_UID, gameControllerUID),
    MEMBER(MemberType::GAME_OBJECT_UID, afterDialogCallbackUID),
    MEMBER(MemberType::INT, dialogueID)
};

GENERATE_BODY_IMPL(TriggerDialogue);

void TriggerDialogue::Start() {
    gameController = GameplaySystems::GetGameObject(gameControllerUID);
    if (gameController) dialogueManagerScript = GET_SCRIPT(gameController, DialogueManager);
    GameObject* afterDialogCallback = GameplaySystems::GetGameObject(afterDialogCallbackUID);
    if (afterDialogCallback) afterDialogCallbackScript = GET_SCRIPT(afterDialogCallback, AfterDialogCallback);
}

void TriggerDialogue::Update() {}
    
void TriggerDialogue::OnCollision(GameObject& /*collidedWith*/, float3 /*collisionNormal*/, float3 /*penetrationDistance*/, void* /*particle*/) {

    if (dialogueManagerScript) {
        if (dialogueID < sizeof(dialogueManagerScript->dialoguesArray) / sizeof(dialogueManagerScript->dialoguesArray[0])
            && &dialogueManagerScript->dialoguesArray[dialogueID] != nullptr) {
            dialogueManagerScript->PlayOpeningAudio();
            dialogueManagerScript->SetActiveDialogue(&dialogueManagerScript->dialoguesArray[dialogueID]);
            if (afterDialogCallbackScript) afterDialogCallbackScript->OpenFactoryDoors();
        }
    }
    GetOwner().Disable();
}