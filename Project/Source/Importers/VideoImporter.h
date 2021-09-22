#pragma once

#include "Utils/JsonValue.h"

namespace VideoImporter {

	bool ImportVideo(const char* filePath, JsonValue jMeta);

}; // namespace VideoImporter
