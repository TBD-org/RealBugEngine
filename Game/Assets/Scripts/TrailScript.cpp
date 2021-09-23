#include "TrailScript.h"

#include "GameObject.h"
#include "GameplaySystems.h"
EXPOSE_MEMBERS(TrailScript) {
	MEMBER(MemberType::INT, speed),
	MEMBER(MemberType::FLOAT, life),
	MEMBER(MemberType::FLOAT, offsetPosition)
};

GENERATE_BODY_IMPL(TrailScript);

void TrailScript::Start() {
}

void TrailScript::Update() {

	if (life >= 0) {
		life -= Time::GetDeltaTime();
		ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
		float3 aux = transform->GetGlobalRotation() * float3(0.0f, 1.0f, 0.0f);
		aux *= speed * Time::GetDeltaTime();
		float3 newPosition = transform->GetGlobalPosition();
		newPosition += aux;
		transform->SetGlobalPosition(newPosition);
	}
	else {
		GameplaySystems::DestroyGameObject(&GetOwner());
	}
}

void TrailScript::OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle) {
	if (collidedWith.name == "Fang" || collidedWith.name == "Onimaru" || collidedWith.name == "FangUltimate") return;
	GameplaySystems::DestroyGameObject(&GetOwner());
}