#include "AIMovement.h"

#include "GameObject.h"
#include "GameplaySystems.h"
#include "Components/ComponentTransform.h"

int AIMovement::maxAcceleration = 9999;

EXPOSE_MEMBERS(AIMovement) {
	MEMBER(MemberType::FLOAT, rotationSmoothness)
};

GENERATE_BODY_IMPL(AIMovement);

void AIMovement::Start() {
	ownerTransform = GetOwner().GetComponent<ComponentTransform>();
	agent = GetOwner().GetComponent<ComponentAgent>();
}

void AIMovement::Update() {

}

void AIMovement::Flee(AIState state, const float3& fromPosition, int speed, bool orientateToDir) {

	if (!ownerTransform || !agent) return;


	float3 position = ownerTransform->GetGlobalPosition();
	float3 direction = (position - fromPosition).Normalized();
	float3 newPosition = position + direction;
	agent->SetMoveTarget(newPosition, true);

	if (state != AIState::START && orientateToDir) {
		Quat newRotation = Quat::LookAt(float3(0, 0, 1), direction.Normalized(), float3(0, 1, 0), float3(0, 1, 0));
		ownerTransform->SetGlobalRotation(newRotation);
	}


}

void AIMovement::Stop() {
	if (!agent)return;

	if (agent->CanBeRemoved()) {
		agent->RemoveAgentFromCrowd();
		agent->AddAgentToCrowd();
	}

}

void AIMovement::Seek(AIState state, const float3& newPosition, int speed, bool orientateToDir) {
	if (!ownerTransform) return;

	float3 position = ownerTransform->GetGlobalPosition();
	float3 direction = newPosition - position;

	if (state == AIState::START) {
		velocity = direction.Normalized() * speed;
		position += velocity * Time::GetDeltaTime();
		ownerTransform->SetGlobalPosition(position);
	} else {
		if (agent) {
			agent->SetMoveTarget(newPosition, true);
			velocity = agent->GetVelocity();
		}
	}
	if (state != AIState::START && orientateToDir) {
		Orientate(velocity);
	}

}

void AIMovement::Orientate(const float3& direction) {
		targetRotation = Quat::LookAt(float3(0, 0, 1), direction.Normalized(), float3(0, 1, 0), float3(0, 1, 0));
		Quat rotation = Quat::Slerp(ownerTransform->GetGlobalRotation(), targetRotation, Min(Time::GetDeltaTime() / Max(rotationSmoothness, 0.000001f), 1.0f));
		ownerTransform->SetGlobalRotation(rotation);
}

bool AIMovement::CharacterInSight(const GameObject* character, const float searchRadius) {
	ComponentTransform* target = character->GetComponent<ComponentTransform>();
	if (target && ownerTransform) {
		float3 posTarget = target->GetGlobalPosition();
		return posTarget.Distance(ownerTransform->GetGlobalPosition()) < searchRadius;
	}

	return false;
}

bool AIMovement::CharacterInAttackRange(const GameObject* character, const float attackRange) {
	ComponentTransform* target = character->GetComponent<ComponentTransform>();
	if (target && ownerTransform) {
		float3 posTarget = target->GetGlobalPosition();
		return posTarget.Distance(ownerTransform->GetGlobalPosition()) < attackRange;
	}

	return false;
}

void AIMovement::SetClipSpeed(UID clipUID, float speed) {
	if (speed >= 0.f) {
		ResourceClip* clip = GameplaySystems::GetResource<ResourceClip>(clipUID);
		clip->speed = speed;
	}
}

GameObject* AIMovement::SearchReferenceInHierarchy(GameObject* root, std::string name) {
	
	if (root->name == name) {
		return root;
	}
	GameObject* reference = nullptr;
	for (GameObject* child : root->GetChildren())
	{
		reference = SearchReferenceInHierarchy(child, name);
		if (reference != nullptr) return reference;

	}
	return nullptr;
}


