#include "NoodleShop.h"

#include "FactoryDoors.h"
#include "GameplaySystems.h"
#include "GameObject.h"

EXPOSE_MEMBERS(NoodleShop) {
    MEMBER(MemberType::GAME_OBJECT_UID, doorUID),
    MEMBER(MemberType::GAME_OBJECT_UID, noodleShopObstacleUID)
};

GENERATE_BODY_IMPL(NoodleShop);

void NoodleShop::Start() {
    gameObject = &GetOwner();
	GameObject* door = GameplaySystems::GetGameObject(doorUID);
    if (door) {
        script = GET_SCRIPT(door, FactoryDoors);
    }
}

void NoodleShop::Update() {
	
}

void NoodleShop::OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle) {
    if (script) {
        script->Open();
        ComponentCapsuleCollider* capsuleCollider = gameObject->GetComponent<ComponentCapsuleCollider>();
	    if (capsuleCollider) capsuleCollider->Disable();
        GameObject* noodleShopObstacleGO = GameplaySystems::GetGameObject(noodleShopObstacleUID);
        if (noodleShopObstacleGO) {
            noodleShopObstacleGO->Disable();
        }
    }
}