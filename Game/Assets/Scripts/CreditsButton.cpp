#include "CreditsButton.h"

#include "GameplaySystems.h"

EXPOSE_MEMBERS(CreditsButton) {
	MEMBER(MemberType::SCENE_RESOURCE_UID, sceneUID)
};

GENERATE_BODY_IMPL(CreditsButton);

void CreditsButton::Start() {
}

void CreditsButton::Update() {
}

void CreditsButton::OnButtonClick() {
	if(sceneUID != 0) SceneManager::ChangeScene(sceneUID);
}