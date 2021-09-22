#pragma once

#include "Resource.h"
#include "Utils/UID.h"

#include <vector>
#include <sndfile.h>

class ResourceAudioClip : public Resource {
public:
	REGISTER_RESOURCE(ResourceAudioClip, ResourceType::AUDIO);

	void Load() override;
	void FinishLoading() override;
	void Unload() override;

public:
	unsigned int alBuffer = 0;

private:
	bool validAudio = false;

	SF_INFO sfInfo;
	int format = 0;
	short* audioData = nullptr;
	unsigned size = 0;
};