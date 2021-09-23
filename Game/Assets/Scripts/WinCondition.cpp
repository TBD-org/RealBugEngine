#include "WinCondition.h"

#include "GameObject.h"
#include "GameplaySystems.h"
#include "PlayerController.h"
#include "SceneTransition.h"

EXPOSE_MEMBERS(WinCondition) {
	MEMBER(MemberType::SCENE_RESOURCE_UID, sceneUID),
	MEMBER(MemberType::GAME_OBJECT_UID, transitionUID)

};

GENERATE_BODY_IMPL(WinCondition);

void WinCondition::Start() {
	gameObject = &GetOwner();
	GameObject* transitionObj = GameplaySystems::GetGameObject(transitionUID);
	if (transitionObj) {
		sceneTransition = GET_SCRIPT(transitionObj, SceneTransition);
	}
}

void WinCondition::Update() {}

void WinCondition::OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle) {
	ComponentBoxCollider* boxCollider = gameObject->GetComponent<ComponentBoxCollider>();
	if (boxCollider) boxCollider->Disable();

	if (sceneTransition) {
		sceneTransition->StartTransition();
	} else {
		if (sceneUID) SceneManager::ChangeScene(sceneUID);
	}

	PlayerController::currentLevel = 2;
}