#include "HotfixFangUltimate.h"
#include "GameplaySystems.h"

EXPOSE_MEMBERS(HotfixFangUltimate) {
    // Add members here to expose them to the engine. Example:
    // MEMBER(MemberType::BOOL, exampleMember1),
    // MEMBER(MemberType::PREFAB_RESOURCE_UID, exampleMember2),
    // MEMBER(MemberType::GAME_OBJECT_UID, exampleMember3)
    MEMBER(MemberType::FLOAT, lifeTime),
};

GENERATE_BODY_IMPL(HotfixFangUltimate);

void HotfixFangUltimate::Start() {
	
}

void HotfixFangUltimate::Update() {
    if (lifeTime > 0) {
        lifeTime -= Time::GetDeltaTime();
    }
    else {
        GameplaySystems::DestroyGameObject(&GetOwner());
    }
}