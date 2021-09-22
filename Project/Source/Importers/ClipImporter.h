#pragma once

#include "Utils/JsonValue.h"

namespace ClipImporter {
	bool ImportClip(const char* filePath, JsonValue jMeta);
} // namespace ClipImporter