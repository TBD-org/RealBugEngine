#pragma once

#include "Resource.h"

enum class TextureMinFilter {
	NEAREST,
	LINEAR,
	NEAREST_MIPMAP_NEAREST,
	LINEAR_MIPMAP_NEAREST,
	NEAREST_MIPMAP_LINEAR,
	LINEAR_MIPMAP_LINEAR
};

enum class TextureMagFilter {
	NEAREST,
	LINEAR
};

enum class TextureWrap {
	REPEAT,
	CLAMP_TO_EDGE,
	CLAMP_TO_BORDER,
	MIRROR_REPEAT,
	MIRROR_CLAMP_TO_EDGE
};

class ResourceTexture : public Resource {
public:
	REGISTER_RESOURCE(ResourceTexture, ResourceType::TEXTURE);

	void Load() override;
	void FinishLoading() override;
	void Unload() override;

	void LoadResourceMeta(JsonValue jResourceMeta) override;
	void SaveResourceMeta(JsonValue jResourceMeta) override;

public:
	unsigned int glTexture = 0;

	TextureWrap wrap = TextureWrap::REPEAT;
	TextureMinFilter minFilter = TextureMinFilter::LINEAR_MIPMAP_LINEAR;
	TextureMagFilter magFilter = TextureMagFilter::LINEAR;

private:
	void UpdateMinFilter(TextureMinFilter filter);
	void UpdateMagFilter(TextureMagFilter filter);
	void UpdateWrap(TextureWrap wrap);

private:
	unsigned char* imageData = nullptr;
	unsigned width = 0;
	unsigned height = 0;
};