#include "ActivateCollider.h"

#include "GameplaySystems.h"

#include "Components/Physics/ComponentSphereCollider.h"

EXPOSE_MEMBERS(ActivateCollider) {
};

GENERATE_BODY_IMPL(ActivateCollider);

void ActivateCollider::Start() {
	
	GameObject* go = &GetOwner();

	if (go) {
		ComponentSphereCollider* sphereCollider = go->GetComponent<ComponentSphereCollider>();
		if (sphereCollider) {
			sphereCollider->Disable();
			sphereCollider->Enable();
		}
	}
}

void ActivateCollider::Update() {
	
}