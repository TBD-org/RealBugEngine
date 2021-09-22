#pragma once

#include "Utils/JsonValue.h"

namespace ScriptImporter {
	bool ImportScript(const char* filePath, JsonValue jMeta);
} // namespace ScriptImporter