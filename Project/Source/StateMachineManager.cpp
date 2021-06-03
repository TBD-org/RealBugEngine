#include "StateMachineManager.h"
#include <Application.h>
#include <Modules/ModuleResources.h>
#include "Modules/ModuleTime.h"
#include <Resources/ResourceStateMachine.h>
#include "Resources/ResourceClip.h"
#include "Transition.h"
#include <Utils/UID.h>
#include <Utils/Logging.h>
#include "GameObject.h"

void StateMachineManager::SendTrigger(const std::string& trigger, std::unordered_map<UID, float>& currentTimeStates, std::list<AnimationInterpolation>& animationInterpolations, const UID& stateMachineResourceUID, State& currentState, std::unordered_map<UID, float>& currentTimeStatesPrincipal) {
	ResourceStateMachine* resourceStateMachine = App->resources->GetResource<ResourceStateMachine>(stateMachineResourceUID);

	Transition* transition = resourceStateMachine->FindTransitionGivenName(trigger);
	if (transition != nullptr) {
		if (transition->source.id == currentState.id) {
			if (animationInterpolations.size() == 0) {
				animationInterpolations.push_front(AnimationInterpolation(&transition->source, currentTimeStates[currentState.id], 0, transition->interpolationDuration));
			}
			//Set the currentTime of AnimationInterpolation equals to currentTimeStatesPrincipal (instead of 0) to avoid the gap between the times between the interpolation of state secondary to state principal.
			// given to this : we are not allowed to have the states in the principal same as the secondary state machine
			animationInterpolations.push_front(AnimationInterpolation(&transition->target, currentTimeStatesPrincipal[transition->target.id], 0, transition->interpolationDuration));
			currentState = transition->target;
		} else {
			LOG("Warning: transition target from %s to %s, and current state is %s ", transition->source.name.c_str(), transition->target.name.c_str(), currentState.name.c_str());
		}
	}
}

bool StateMachineManager::UpdateAnimations(GameObject* gameObject, const GameObject& owner, std::unordered_map<UID, float>& currentTimeStatesPrincipal, std::list<AnimationInterpolation>& animationInterpolationsPrincipal, const UID& stateMachineResourceUIDPrincipal, State* currentStatePrincipal, std::unordered_map<UID, float>& currentTimeStatesSecondary, std::list<AnimationInterpolation>& animationInterpolationsSecondary, const UID& stateMachineResourceUIDSecondary, State* currentStateSecondary, float3& position, Quat& rotation, bool& resetSecondaryStatemachine) {
	bool result = true;
	StateMachineEnum stateMachineSelected = StateMachineEnum::PRINCIPAL;
	//The currentStateSecondary could be an empty State object with the id of zero in case for the second state machine
	if (currentStateSecondary && currentStateSecondary->id != 0) {
		ResourceStateMachine* resourceStateMachine = App->resources->GetResource<ResourceStateMachine>(stateMachineResourceUIDSecondary);
		//Looking for the bone if is it in the state machine in order to be applied
		if (resourceStateMachine && !resourceStateMachine->bones.empty()) {
			auto nameBone = resourceStateMachine->bones.find(gameObject->name);
			if (nameBone != resourceStateMachine->bones.end()) {
				stateMachineSelected = StateMachineEnum::SECONDARY;
			}
		}
	}

	//Selecting which state machine is going to be used
	switch (stateMachineSelected) {
	case StateMachineEnum::PRINCIPAL:
		result = StateMachineManager::CalculateAnimation(gameObject, owner, currentTimeStatesPrincipal, animationInterpolationsPrincipal, stateMachineResourceUIDPrincipal, currentStatePrincipal, position, rotation, resetSecondaryStatemachine, StateMachineEnum::PRINCIPAL);
		break;
	case StateMachineEnum::SECONDARY:
		result = StateMachineManager::CalculateAnimation(gameObject, owner, currentTimeStatesSecondary, animationInterpolationsSecondary, stateMachineResourceUIDSecondary, currentStateSecondary, position, rotation, resetSecondaryStatemachine, StateMachineEnum::SECONDARY, currentStatePrincipal->id == currentStateSecondary->id);
		break;
	}

	return result;
}

bool StateMachineManager::CalculateAnimation(GameObject* gameObject, const GameObject& owner, std::unordered_map<UID, float>& currentTimeStates, std::list<AnimationInterpolation>& animationInterpolations, const UID& stateMachineResourceUID, State* currentState, float3& position, Quat& rotation, bool& resetSecondaryStatemachine, StateMachineEnum stateMachineEnum, bool principalEqualSecondary) {
	bool result = false;

	ResourceStateMachine* resourceStateMachine = App->resources->GetResource<ResourceStateMachine>(stateMachineResourceUID);
	if (!resourceStateMachine) {
		return result;
	}

	//Checking for transition between states
	if (animationInterpolations.size() > 1) {
		result = AnimationController::InterpolateTransitions(animationInterpolations.begin(), animationInterpolations, *owner.GetRootBone(), *gameObject, position, rotation);

		//Updating times
		if (gameObject->name == (*resourceStateMachine->bones.begin())) { // Only udate currentTime for the rootBone
			bool finishedTransition = AnimationController::UpdateTransitions(animationInterpolations, currentTimeStates, App->time->GetDeltaTime());
			//Comparing the state principal with state secondary & set variable for setting secondary state as "empty" State
			if (finishedTransition && principalEqualSecondary) {
				resetSecondaryStatemachine = true;
			}
		}

	} else {
		if (currentState->id != 0) {
			ResourceClip* clip = App->resources->GetResource<ResourceClip>(currentState->clipUid);
			if (!clip) {
				return result;
			}
			result = AnimationController::GetTransform(*clip, currentTimeStates[currentState->id], gameObject->name.c_str(), position, rotation);

			if (gameObject->name == (*resourceStateMachine->bones.begin())) { //Only call this once
				if (!clip->loop) {
					int currentSample = AnimationController::GetCurrentSample(*clip, currentTimeStates[currentState->id]);
					// Checking if the current sample is the last keyframe in order to send the event
					if (currentSample == clip->endIndex) {
						for (ComponentScript& script : owner.GetComponents<ComponentScript>()) {
							if (script.IsActive()) {
								Script* scriptInstance = script.GetScriptInstance();
								if (scriptInstance != nullptr) {
									switch (stateMachineEnum) {
									case StateMachineEnum::PRINCIPAL:
										scriptInstance->OnAnimationFinished();
										break;
									case StateMachineEnum::SECONDARY:
										scriptInstance->OnAnimationSecondaryFinished();
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return result;
}
