#include "TriggerDialoguePowerUp.h"

#include "GameplaySystems.h"
#include "GameObject.h"

#include "DialogueManager.h"
#include "PlayerController.h"

EXPOSE_MEMBERS(TriggerDialoguePowerUp) {
    MEMBER(MemberType::GAME_OBJECT_UID, gameControllerUID),
    MEMBER(MemberType::GAME_OBJECT_UID, playerUID)
};

GENERATE_BODY_IMPL(TriggerDialoguePowerUp);

void TriggerDialoguePowerUp::Start() {
    gameController = GameplaySystems::GetGameObject(gameControllerUID);
    if (gameController) dialogueManagerScript = GET_SCRIPT(gameController, DialogueManager);
    player = GameplaySystems::GetGameObject(playerUID);
    if (player) playerControllerScript = GET_SCRIPT(player, PlayerController);
}

void TriggerDialoguePowerUp::Update() {

}

void TriggerDialoguePowerUp::OnCollision(GameObject& /*collidedWith*/, float3 /*collisionNormal*/, float3 /*penetrationDistance*/, void* /*particle*/) {

    if (playerControllerScript) playerControllerScript->ObtainUpgradeCell();
    if (dialogueManagerScript) {
        dialogueManagerScript->PlayOpeningAudio();
        dialogueManagerScript->ActivatePowerUpDialogue();
    }
    GetOwner().Disable();
}