#include "FireTilesAnimationController.h"

#include "GameObject.h"

EXPOSE_MEMBERS(FireTilesAnimationController) {    
};

GENERATE_BODY_IMPL(FireTilesAnimationController);

void FireTilesAnimationController::Start() {
	animation = GetOwner().GetComponent<ComponentAnimation>();

	GameObject* owner = &GetOwner();

	ComponentBoxCollider* bCollider = owner->GetComponent<ComponentBoxCollider>();
	if (bCollider) {
		bCollider->Disable();
	}

}

void FireTilesAnimationController::Update() {
	
}

void FireTilesAnimationController::OnAnimationFinished() {
	if (animation->GetCurrentState()) {
		if (animation->GetCurrentState()->name == "Opening") {
			animation->SendTrigger("OpeningOpened");
		}
		else if (animation->GetCurrentState()->name == "Closing") {
			animation->SendTrigger("ClosingClosed");
		}
	}
}
