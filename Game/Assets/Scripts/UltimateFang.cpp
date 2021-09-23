#include "UltimateFang.h"

#include "GameObject.h"
#include "GameplaySystems.h"
#include "Modules/ModulePhysics.h"
#include "Components/ComponentBoundingBox.h"

EXPOSE_MEMBERS(UltimateFang) {
	MEMBER(MemberType::FLOAT, radius),
	MEMBER(MemberType::PREFAB_RESOURCE_UID, fangBulletUID)
};

GENERATE_BODY_IMPL(UltimateFang);

void UltimateFang::Start() {

	bullet = GameplaySystems::GetResource<ResourcePrefab>(fangBulletUID);

	GetOwner().Disable();
}

void UltimateFang::Update() {
	tickCurrent -= Time::GetDeltaTime();
	if (tickCurrent <= 0) {
		tickOn = true;
		tickCurrent = tickDuration;
	}
	else {
		tickOn = false;
		if (collisionedGameObject.size() > 0) {
			int i = rand() % collisionedGameObject.size(); // select random enemi from the list
			ComponentTransform* transformOwner = GetOwner().GetComponent<ComponentTransform>();
			ComponentTransform* transformTarget = collisionedGameObject[i].GetComponent<ComponentTransform>();

			GameObject* fang = GetOwner().GetParent();

			ComponentCapsuleCollider* ccOwner = fang->GetComponent<ComponentCapsuleCollider>();
			float midHeight = 0.0f;
			if(ccOwner){
				midHeight = ccOwner->height;//bbOwner->GetLocalMaxPointAABB().y / 2;
			}

			float3 posFang = transformOwner->GetGlobalPosition();
			float3 posTarget = transformTarget->GetGlobalPosition();
			float3 dir = (posTarget - posFang).Normalized();
			
			GameObject* auxBullet =  GameplaySystems::Instantiate(bullet, transformOwner->GetGlobalPosition() + float3(0.0f, midHeight, 0.0f), (DirectionToQuat(dir) * float3x3::FromEulerXYZ(pi / 2, 0.0f, 0.0f)).ToQuat());
			if (auxBullet->GetComponent<ComponentParticleSystem>()) {
				auxBullet->GetComponent<ComponentParticleSystem>()->Play();
			}
		}
	}
	collisionedGameObject.clear();
}

Quat UltimateFang::DirectionToQuat(float3 dir) {
	float angle = atan2(dir.x, dir.z);
	float qx = 0;
	float qy = 1 * sin(angle / 2);
	float qz = 0;
	float qw = cos(angle / 2);
	return Quat(qx, qy, qz, qw);
}

void UltimateFang::OnCollision(GameObject& collidedWith, float3 collisionNormal, float3 penetrationDistance, void* particle) {
	if (tickOn) {
		if (collidedWith.name == "MeleeGrunt" || collidedWith.name == "RangedGrunt") {
			collisionedGameObject.push_back(collidedWith);
		}
	}
}

void UltimateFang::StartUltimate()
{
	ComponentSphereCollider* sphereCollider = GetOwner().GetComponent<ComponentSphereCollider>();
	sphereCollider->radius = radius;

	GetOwner().Enable();
	tickCurrent = tickDuration;
}

void UltimateFang::EndUltimate()
{
	GetOwner().Disable();
}
