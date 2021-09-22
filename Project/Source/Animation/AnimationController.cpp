#include "AnimationController.h"

#include "Application.h"
#include "GameObject.h"
#include "State.h"
#include "Modules/ModuleTime.h"
#include "Resources/ResourceAnimation.h"
#include "AnimationInterpolation.h"
#include "Resources/ResourceClip.h"
#include "Components/ComponentAnimation.h"

#include "Math/float3.h"

#include "Utils/Leaks.h"

int AnimationController::GetCurrentSample(const ResourceClip& clip, float& currentTime) {
	unsigned currentSample = clip.beginIndex + static_cast<unsigned>((currentTime * (clip.keyFramesSize)) / clip.duration);
	return Clamp(currentSample, clip.beginIndex, clip.endIndex);
}

bool AnimationController::GetTransform(ResourceClip& clip, float& currentTime, const char* name, float3& pos, Quat& quat, ComponentAnimation& componentAnimation) {
	if (clip.animationUID == 0) return false;

	ResourceAnimation* resourceAnimation = App->resources->GetResource<ResourceAnimation>(clip.animationUID);
	if (resourceAnimation == nullptr || resourceAnimation->keyFrames.size() == 0) return false;

	// Resetting the events since it has been a loop only for one bone
	if (currentTime >= clip.duration) {
		for (auto& element : componentAnimation.listClipsKeyEvents[clip.GetId()]) {
			element.second.sent = false;
		}
		componentAnimation.listClipsCurrentEventKeyFrames[clip.GetId()] = clip.beginIndex;
	}

	if (clip.loop) {
		while (currentTime >= clip.duration) {
			currentTime -= clip.duration;
		}
	} else {
		currentTime = currentTime >= clip.duration ? clip.duration : currentTime;
	}

	float currentSample = (currentTime * (clip.keyFramesSize)) / clip.duration;
	currentSample += clip.beginIndex;
	int intPart = (int) currentSample;
	float decimal = currentSample - intPart;

	//find in hash by name
	std::unordered_map<std::string, ResourceAnimation::Channel>::const_iterator channel = resourceAnimation->keyFrames[intPart].channels.find(name);
	unsigned int idNext = intPart == (clip.endIndex) ? clip.beginIndex : intPart + 1;
	std::unordered_map<std::string, ResourceAnimation::Channel>::const_iterator channelNext = resourceAnimation->keyFrames[idNext].channels.find(name);

	if (channel == resourceAnimation->keyFrames[intPart].channels.end() && channelNext == resourceAnimation->keyFrames[idNext].channels.end()) {
		return false;
	}

	pos = float3::Lerp(channel->second.translation, channelNext->second.translation, decimal);
	quat = AnimationController::Interpolate(channel->second.rotation, channelNext->second.rotation, decimal);

	return true;
}

bool AnimationController::InterpolateTransitions(const std::list<AnimationInterpolation>::iterator& it, const std::list<AnimationInterpolation>& animationInterpolations, const GameObject& rootBone, const GameObject& gameObject, float3& pos, Quat& quat, ComponentAnimation& componentAnimation) {
	ResourceClip* clip = App->resources->GetResource<ResourceClip>((*it).state->clipUid);
	if (!clip) {
		return false;
	}
	bool result = GetTransform(*clip, (*it).currentTime, gameObject.name.c_str(), pos, quat, componentAnimation);
	bool resultInner = true;
	if (&(*it) != &(*std::prev(animationInterpolations.end())) && result) {
		float3 position;
		Quat rotation;
		resultInner = AnimationController::InterpolateTransitions(std::next(it), animationInterpolations, rootBone, gameObject, position, rotation, componentAnimation);
		if (resultInner) {
			float weight = (*it).fadeTime / (*it).transitionTime;
			pos = float3::Lerp(position, pos, weight);
			quat = AnimationController::Interpolate(rotation, quat, weight);
		}
	}

	return result && resultInner;
}

struct CheckFinishInterpolation {
	bool operator()(AnimationInterpolation& animationInterpolation) {
		return animationInterpolation.fadeTime >= animationInterpolation.transitionTime;
	}
};

bool AnimationController::UpdateTransitions(std::list<AnimationInterpolation>& animationInterpolations, std::unordered_map<UID, float>& currentTimeStates, const float time) {
	bool finished = false;
	for (auto& interpolation = animationInterpolations.begin(); interpolation != animationInterpolations.end(); ++interpolation) {
		ResourceClip* clip = App->resources->GetResource<ResourceClip>((*interpolation).state->clipUid);
		if (!clip) {
			return false;
		}
		(*interpolation).currentTime += time * clip->speed;

		bool isLastOne = interpolation == std::prev(animationInterpolations.end()); //ignore given it is the last one

		if (!isLastOne) {
			(*interpolation).fadeTime += time;
		}

		if (finished) {
			(*interpolation).fadeTime = (*interpolation).transitionTime;
		}

		if ((*interpolation).fadeTime >= (*interpolation).transitionTime) {
			finished = true;
			currentTimeStates[(*interpolation).state->id] = (*interpolation).currentTime;
		}
	}

	CheckFinishInterpolation checkFinishInterpolation;
	animationInterpolations.remove_if(checkFinishInterpolation);
	if (animationInterpolations.size() <= 1) {
		animationInterpolations.clear();
	}

	return finished;
}

Quat AnimationController::Interpolate(const Quat& first, const Quat& second, float lambda) {
	if (first.Dot(second) >= 0.0f) { // is minimum arc ?
		return Quat::Lerp(first, second, lambda).Normalized();
	} else {
		return Quat::Lerp(first, second.Neg(), lambda).Normalized();
	}
}