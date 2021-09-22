#pragma once

#include "Utils/JsonValue.h"

class NavMesh;

namespace NavMeshImporter {
	bool ImportNavMesh(const char* filePath, JsonValue jMeta);
	bool ExportNavMesh(NavMesh& navMesh, const char* filePath);
} // namespace MaterialImporter