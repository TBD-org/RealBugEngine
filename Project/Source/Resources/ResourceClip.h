#pragma once
#include "Application.h"

#include "ResourceAnimation.h"
#include "Resources/ResourceType.h"
#include "Resources/Resource.h"
#include "Modules/ModuleResources.h"

#include "Utils/UID.h"
#include <string>

struct EventClip {
	bool sent = false;
	std::string name = "";
};

class ResourceClip : public Resource {
public:
	REGISTER_RESOURCE(ResourceClip, ResourceType::CLIP);

	void Load() override;
	void FinishLoading() override;
	void GetInfoJson();
	void Unload() override;
	void OnEditorUpdate() override;

	bool SaveToFile(const char* filePath);

	void Init(std::string& mName, UID mAnimationUID = 0, unsigned int mBeginIndex = 0, unsigned int mEndIndex = 0, bool mLoop = false, float mSpeed = 1.0f, UID mid = 0);

	void SetBeginIndex(unsigned int index);
	void SetEndIndex(unsigned int index);

public:
	std::string name = "";
	UID animationUID = 0;
	bool loop = false;
	unsigned int beginIndex = 0;
	unsigned int endIndex = 0;
	float speed = 1.0f;
	float duration = 1.0f;
	float frameRate = 1.0f;
	unsigned int keyFramesSize = 0;

	std::unordered_map<unsigned int, EventClip> keyEventClips;

private:
	int keyToDelete = -1;
	unsigned int newKeyFrameEditor = 0;
	std::string newNameEditor = "NewName";
};