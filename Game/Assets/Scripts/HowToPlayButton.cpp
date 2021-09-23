#include "HowToPlayButton.h"

#include "GameplaySystems.h"

EXPOSE_MEMBERS(HowToPlayButton) {
	MEMBER(MemberType::SCENE_RESOURCE_UID, sceneUID)
};

GENERATE_BODY_IMPL(HowToPlayButton);

void HowToPlayButton::Start() {
}

void HowToPlayButton::Update() {
}

void HowToPlayButton::OnButtonClick() {
	if(sceneUID != 0) SceneManager::ChangeScene(sceneUID);
}