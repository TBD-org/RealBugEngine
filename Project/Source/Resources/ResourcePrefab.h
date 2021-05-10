#pragma once

#include "Globals.h"
#include "Resources/Resource.h"

class GameObject;

class ResourcePrefab : public Resource {
public:
	REGISTER_RESOURCE(ResourcePrefab, ResourceType::PREFAB);

	TESSERACT_ENGINE_API UID BuildPrefab(GameObject* parent);
};
