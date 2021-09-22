#pragma once

#include "Utils/JsonValue.h"

namespace SkyboxImporter {
	bool ImportSkybox(const char* filePath, JsonValue jMeta);
};
