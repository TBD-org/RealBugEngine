#pragma once

#include "Utils/JsonValue.h"

#define BUFFER_LEN 4096

namespace AudioImporter {
	bool ImportAudio(const char* filePath, JsonValue jMeta);

	void EncondeWavToOgg(const char* infilename, const char* outfilename);
};
