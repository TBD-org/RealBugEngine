#include "OnimaruBullet.h"

#include "GameObject.h"
#include "GameplaySystems.h"
EXPOSE_MEMBERS(OnimaruBullet) {
	MEMBER(MemberType::INT, speed),
		MEMBER(MemberType::FLOAT, life)
};

GENERATE_BODY_IMPL(OnimaruBullet);

void OnimaruBullet::Start() {
}

void OnimaruBullet::Update() {
	if (life >= 0) {
		life -= Time::GetDeltaTime();
		ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
		float3 aux = onimaruDirection * float3(0.0f, 0.0f, 1.0f);
		aux *= speed * Time::GetDeltaTime();
		float3 newPosition = transform->GetGlobalPosition();
		newPosition += aux;
		transform->SetGlobalPosition(newPosition);
	}
	else {
		GameplaySystems::DestroyGameObject(&GetOwner());
	}
}

void OnimaruBullet::SetOnimaruDirection(Quat direction) {
	onimaruDirection = direction;
}

void OnimaruBullet::OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle) {
	if (collidedWith.name == "Fang" || collidedWith.name == "Onimaru" || collidedWith.name == "OnimaruBullet" || collidedWith.name == "Shield") return;
	GameplaySystems::DestroyGameObject(&GetOwner());
}