#pragma once

#include "Resource.h"

#include "Math/float3.h"
#include "Math/Quat.h"
#include <vector>
#include <unordered_map>

class ResourceAnimation : public Resource {
public:
	struct Channel {
		float3 translation = float3::zero;
		Quat rotation = Quat::identity;
	};

	struct KeyFrameChannels {
		std::unordered_map<std::string, Channel> channels;
	};

public:
	REGISTER_RESOURCE(ResourceAnimation, ResourceType::ANIMATION);

	void Load() override;
	void Unload() override;

public:
	std::vector<KeyFrameChannels> keyFrames;

	float duration = 0.0f;
};
