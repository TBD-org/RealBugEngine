#pragma once

#include "Utils/JsonValue.h"

namespace FontImporter {
	bool ImportFont(const char* filePath, JsonValue jMeta);
}; //namespace FontImporter