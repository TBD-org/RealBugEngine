#pragma once

#include "Utils/JsonValue.h"

#include <unordered_map>
#include <string>

class GameObject;

namespace ModelImporter {
	bool ImportModel(const char* filePath, JsonValue jMeta);

	void SaveBones(GameObject* node, std::unordered_map<std::string, GameObject*>& goBones);
	void CacheBones(GameObject* node, std::unordered_map<std::string, GameObject*>& goBones);
} // namespace ModelImporter