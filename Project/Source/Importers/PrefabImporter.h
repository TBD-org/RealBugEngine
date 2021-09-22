#pragma once

#include "Utils/JsonValue.h"

class GameObject;

namespace PrefabImporter {
	bool ImportPrefab(const char* filePath, JsonValue jMeta);

	bool SavePrefab(const char* filePath, GameObject* root);
} // namespace PrefabImporter