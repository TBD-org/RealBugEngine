#pragma once

#include "Utils/UID.h"
#include "Resources/ResourceType.h"
#include "Utils/JsonValue.h"

#include <string>
#include <mutex>

class Resource {
public:
	Resource(ResourceType type, UID id, const char* name, const char* assetFilePath, const char* resourceFilePath);
	virtual ~Resource();

	ResourceType GetType() const;

	UID GetId() const;
	const std::string& GetName() const;
	const std::string& GetAssetFilePath() const;
	const std::string& GetResourceFilePath() const;

	void SetName(const char* name);

	bool IsLoaded();
	void SetLoaded(bool loaded);

	virtual void Load();
	virtual void FinishLoading();
	virtual void Unload();

	virtual void LoadResourceMeta(JsonValue jResourceMeta);
	virtual void SaveResourceMeta(JsonValue jResourceMeta);
	virtual void OnEditorUpdate();

private:
	ResourceType type = ResourceType::UNKNOWN;

	UID id = 0;
	std::string name = "";
	std::string assetFilePath = "";
	std::string resourceFilePath = "";

	bool isLoaded = false;
};
