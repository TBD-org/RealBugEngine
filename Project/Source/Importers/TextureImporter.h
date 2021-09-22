#pragma once

#include "Utils/JsonValue.h"
#include "Importers/ImportOptions.h"
#include "Resources/ResourceTexture.h"

class TextureImportOptions : public ImportOptions {
public:
	void ShowImportOptions() override;
	void Load(JsonValue jMeta) override;
	void Save(JsonValue jMeta) override;

public:
	bool flip = false;
	TextureCompression compression = TextureCompression::NONE;
	TextureWrap wrap = TextureWrap::REPEAT;
	TextureMinFilter minFilter = TextureMinFilter::LINEAR_MIPMAP_LINEAR;
	TextureMagFilter magFilter = TextureMagFilter::LINEAR;
};

namespace TextureImporter {
	bool ImportTexture(const char* filePath, JsonValue jMeta);
}; // namespace TextureImporter