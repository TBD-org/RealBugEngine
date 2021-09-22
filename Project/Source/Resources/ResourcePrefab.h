#pragma once

#include "Globals.h"
#include "Resources/Resource.h"

class GameObject;
class Scene;

class ResourcePrefab : public Resource {
public:
	REGISTER_RESOURCE(ResourcePrefab, ResourceType::PREFAB);

	void Load() override;
	void FinishLoading() override;
	void Unload() override;

	TESSERACT_ENGINE_API UID BuildPrefab(GameObject* parent);

private:
	rapidjson::Document* document = nullptr;
	Scene* prefabScene = nullptr;
};
