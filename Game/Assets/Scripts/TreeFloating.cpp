#include "TreeFloating.h"

#include "GameObject.h"
#include "GameplaySystems.h"

#include <cmath>

EXPOSE_MEMBERS(TreeFloating) {
	MEMBER(MemberType::GAME_OBJECT_UID, treesUID)
};

GENERATE_BODY_IMPL(TreeFloating);

void TreeFloating::Start() {
	offset = rand() % 200;
	trees = GameplaySystems::GetGameObject(treesUID);
	if (trees) transform = trees->GetComponent<ComponentTransform>();
	if (transform) initialPosition = transform->GetPosition();
}

void TreeFloating::Update() {
	
	if (trees) transform = trees->GetComponent<ComponentTransform>();
	if (!transform) return;
	offset = offset + Time::GetDeltaTime();
	transform->SetPosition(initialPosition + float3(0.f, Sin(offset), 0.f));
}