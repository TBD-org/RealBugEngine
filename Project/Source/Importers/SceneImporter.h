#pragma once

#include "Utils/JsonValue.h"

class Scene;

namespace SceneImporter {
	bool ImportScene(const char* filePath, JsonValue jMeta);

	Scene* LoadScene(const char* filePath);
	bool SaveScene(Scene* scene, const char* filePath);
} // namespace SceneImporter