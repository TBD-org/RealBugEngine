#include "ComponentAnimation.h"

#include "Application.h"
#include "Transition.h"
#include "GameObject.h"
#include "AnimationInterpolation.h"
#include "AnimationController.h"
#include "Resources/ResourceAnimation.h"
#include "Resources/ResourceClip.h"
#include "Components/ComponentType.h"
#include "Components/ComponentTransform.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleInput.h"
#include "Modules/ModuleEvents.h"
#include "Utils/UID.h"
#include "Utils/Logging.h"
#include "Utils/ImGuiUtils.h"

#include <algorithm> // std::find

#include "Utils/Leaks.h"

#define JSON_TAG_LOOP "Controller"
#define JSON_TAG_ANIMATION_ID "AnimationId"
#define JSON_TAG_STATE_MACHINE_ID "StateMachineId"
#define JSON_TAG_INITAL_STATE_ID "InitalState"
#define JSON_TAG_CLIP "Clip"

void ComponentAnimation::Update() {
	if (!initialState) { //Checking if there is no state machine
		LoadResourceStateMachine();
	}

	OnUpdate();
}

void ComponentAnimation::OnEditorUpdate() {
	if (ImGui::Checkbox("Active", &active)) {
		if (GetOwner().IsActive()) {
			if (active) {
				Enable();
			} else {
				Disable();
			}
		}
	}
	ImGui::Separator();

	ImGui::TextColored(App->editor->titleColor, "Animation");

	UID oldStateMachineUID = stateMachineResourceUID;
	ImGui::ResourceSlot<ResourceStateMachine>("State Machine", &stateMachineResourceUID);
	if (oldStateMachineUID != stateMachineResourceUID) {
		ResourceStateMachine* resourceStateMachine = App->resources->GetResource<ResourceStateMachine>(stateMachineResourceUID);
		if (resourceStateMachine) {
			currentState = &resourceStateMachine->states.front();
			initialState = &resourceStateMachine->states.front();
		} else {
			currentState = nullptr;
			initialState = nullptr;
		}
	}
}

void ComponentAnimation::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_STATE_MACHINE_ID] = stateMachineResourceUID;
	jComponent[JSON_TAG_INITAL_STATE_ID] = initialState ? initialState->id : 0;
}

void ComponentAnimation::Load(JsonValue jComponent) {
	stateMachineResourceUID = jComponent[JSON_TAG_STATE_MACHINE_ID];
	if (stateMachineResourceUID != 0) App->resources->IncreaseReferenceCount(stateMachineResourceUID);

	initalStateUid = jComponent[JSON_TAG_INITAL_STATE_ID];
	LoadResourceStateMachine();
}

void ComponentAnimation::OnUpdate() {
	// Update gameobjects matrix
	GameObject* rootBone = GetOwner().GetRootBone();

	UpdateAnimations(rootBone);

	if (currentState) {
		ResourceClip* currentClip = App->resources->GetResource<ResourceClip>(currentState->clipUid);
		currentState->currentTime += App->time->GetDeltaTime() * currentClip->speed;
	}
}

void ComponentAnimation::SendTrigger(const std::string& trigger) {
	ResourceStateMachine* resourceStateMachine = App->resources->GetResource<ResourceStateMachine>(stateMachineResourceUID);

	Transition* transition = resourceStateMachine->FindTransitionGivenName(trigger);
	if (transition != nullptr) {
		if (animationInterpolations.size() == 0) {
			animationInterpolations.push_front(AnimationInterpolation(&transition->source, currentState->currentTime, 0, transition->interpolationDuration));
		}
		animationInterpolations.push_front(AnimationInterpolation(&transition->target, 0, 0, transition->interpolationDuration));
	}
}

void ComponentAnimation::UpdateAnimations(GameObject* gameObject) {
	if (gameObject == nullptr) {
		return;
	}

	//find gameobject in hash
	float3 position = float3::zero;
	Quat rotation = Quat::identity;

	bool result = true;
	ResourceStateMachine* resourceStateMachine = App->resources->GetResource<ResourceStateMachine>(stateMachineResourceUID);
	if (!resourceStateMachine) {
		return;
	}

	if (animationInterpolations.size() > 1) {
		result = AnimationController::InterpolateTransitions(animationInterpolations.begin(), animationInterpolations, *GetOwner().GetRootBone(), *gameObject, position, rotation);

		//Updating times
		if (gameObject == GetOwner().GetRootBone()) { // Only udate currentTime for the rootBone
			State* newState = AnimationController::UpdateTransitions(animationInterpolations, App->time->GetDeltaTime());
			if (newState) {
				currentState = newState;
			}
		}

	} else {
		if (currentState) {
			ResourceClip* clip = App->resources->GetResource<ResourceClip>(currentState->clipUid);
			result = AnimationController::GetTransform(*clip, currentState->currentTime, gameObject->name.c_str(), position, rotation);
			
			if (gameObject == GetOwner().GetRootBone()) {
				if (!clip->loop) {
					int currentSample = AnimationController::GetCurrentSample(*clip, currentState->currentTime);
					if (currentSample == clip->endIndex) {
						TesseractEvent animationFinishedEvent = TesseractEvent(TesseractEventType::ANIMATION_FINISHED);
						App->events->AddEvent(animationFinishedEvent);
					}
				}
			}
		}
	}

	ComponentTransform* componentTransform = gameObject->GetComponent<ComponentTransform>();

	if (componentTransform && result) {
		componentTransform->SetPosition(position);
		componentTransform->SetRotation(rotation);
	}

	for (GameObject* child : gameObject->GetChildren()) {
		UpdateAnimations(child);
	}
}

void ComponentAnimation::LoadResourceStateMachine() {
	ResourceStateMachine* resourceStateMachine = App->resources->GetResource<ResourceStateMachine>(stateMachineResourceUID);

	if (resourceStateMachine) {
		for (auto& state : resourceStateMachine->states) {
			if (initalStateUid == state.id) {
				initialState = &state;
				currentState = &state;
				break;
			}
		}
	}
}
