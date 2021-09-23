#include "GenericGodModeToggle.h"
#include "GodModeController.h"

#include "GameplaySystems.h"
#include "GameObject.h"

#include "Components/UI/ComponentToggle.h"

EXPOSE_MEMBERS(GenericGodModeToggle) {
	MEMBER(MemberType::GAME_OBJECT_UID, godControllerUID),
	MEMBER(MemberType::UINT, index)
};

GENERATE_BODY_IMPL(GenericGodModeToggle);

void GenericGodModeToggle::Start() {
	gameObject = &GetOwner();
	if (gameObject->HasComponent<ComponentToggle>()) {
		toggle = gameObject->GetComponent<ComponentToggle>();
	}

	godController = GameplaySystems::GetGameObject(godControllerUID);
}

void GenericGodModeToggle::Update() {}

void GenericGodModeToggle::OnToggled(bool toggled_) {
	if (godController) {
		GodModeController* gmControllerScript = GET_SCRIPT(godController, GodModeController);
		gmControllerScript->OnChildToggle(index, toggle->IsChecked());
	}
}