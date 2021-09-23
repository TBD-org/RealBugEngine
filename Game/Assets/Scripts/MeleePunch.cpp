#include "MeleePunch.h"
#include "GameplaySystems.h"
#include "GameObject.h"

EXPOSE_MEMBERS(MeleePunch) {
};

GENERATE_BODY_IMPL(MeleePunch);

void MeleePunch::Start() {
	
}

void MeleePunch::Update() {
}

void MeleePunch::OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle) {
	if (collidedWith.name == "Shield" || collidedWith.name == "Onimaru" || collidedWith.name == "Fang" ) {
		GameplaySystems::DestroyGameObject(&GetOwner());
	}
}