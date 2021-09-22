#pragma once

#include "Utils/JsonValue.h"

namespace ShaderImporter {
	bool ImportShader(const char* filePath, JsonValue jMeta);
} // namespace ShaderImporter