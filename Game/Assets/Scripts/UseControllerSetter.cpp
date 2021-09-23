#include "UseControllerSetter.h"
#include "GameObject.h"
#include "GameplaySystems.h"
#include "PlayerController.h"

EXPOSE_MEMBERS(UseControllerSetter) {
	// Add members here to expose them to the engine. Example:
	// MEMBER(MemberType::BOOL, exampleMember1),
	// MEMBER(MemberType::PREFAB_RESOURCE_UID, exampleMember2),
	// MEMBER(MemberType::GAME_OBJECT_UID, exampleMember3)
};

GENERATE_BODY_IMPL(UseControllerSetter);

void UseControllerSetter::Start() {
	SearchForReferences();
}

void UseControllerSetter::Update() {

}

void UseControllerSetter::SearchForReferences() {
	GameObject* playerObj = GameplaySystems::GetGameObject("Player");
	if (playerObj) {
		playerController = GET_SCRIPT(playerObj, PlayerController);
	}
}

void UseControllerSetter::OnToggled(bool toggled_) {
	if (!playerController) SearchForReferences();
	if (!playerController) return;
	playerController->SetUseGamepad(playerController->useGamepad);
}