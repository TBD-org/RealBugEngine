#include "ResourceTexture.h"

#include "Globals.h"
#include "Application.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleEditor.h"
#include "Utils/MSTimer.h"
#include "Utils/Logging.h"

#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include "IL/il.h"
#include "IL/ilu.h"
#include "GL/glew.h"
#include "imgui.h"

#include "Utils/Leaks.h"

#define JSON_TAG_MINFILTER "MinFilter"
#define JSON_TAG_MAGFILTER "MagFilter"
#define JSON_TAG_WRAP "Wrap"

void ResourceTexture::Load() {
	std::string filePath = GetResourceFilePath();
	LOG("Loading texture from path: \"%s\".", filePath.c_str());

	// Timer to measure loading a texture
	MSTimer timer;
	timer.Start();

	// Generate image handler
	unsigned image;
	ilGenImages(1, &image);
	DEFER {
		ilDeleteImages(1, &image);
	};

	// Load image
	ilBindImage(image);
	bool imageLoaded = ilLoad(IL_RAW, filePath.c_str());
	if (!imageLoaded) {
		LOG("Failed to load image.");
		return;
	}

	// Get data
	width = ilGetInteger(IL_IMAGE_WIDTH);
	height = ilGetInteger(IL_IMAGE_HEIGHT);
	unsigned bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	unsigned dataSize = width * height * bpp;
	imageData = new unsigned char[dataSize];
	memcpy(imageData, ilGetData(), dataSize);

	unsigned timeMs = timer.Stop();
	LOG("Texture loaded in %ums.", timeMs);
}

void ResourceTexture::FinishLoading() {
	if (imageData == nullptr) return;
	DEFER {
		RELEASE(imageData);
	};

	// Generate texture from image
	glGenTextures(1, &glTexture);
	glBindTexture(GL_TEXTURE_2D, glTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

	// Generate mipmaps and set filtering and wrapping
	glGenerateMipmap(GL_TEXTURE_2D);
	UpdateWrap(wrap);
	UpdateMinFilter(minFilter);
	UpdateMagFilter(magFilter);
}

void ResourceTexture::Unload() {
	RELEASE(imageData);

	if (glTexture) {
		glDeleteTextures(1, &glTexture);
		glTexture = 0;
	}
}

void ResourceTexture::LoadResourceMeta(JsonValue jResourceMeta) {
	wrap = (TextureWrap)(int) jResourceMeta[JSON_TAG_WRAP];
	minFilter = (TextureMinFilter)(int) jResourceMeta[JSON_TAG_MINFILTER];
	magFilter = (TextureMagFilter)(int) jResourceMeta[JSON_TAG_MAGFILTER];
}

void ResourceTexture::SaveResourceMeta(JsonValue jResourceMeta) {
	jResourceMeta[JSON_TAG_WRAP] = (int) wrap;
	jResourceMeta[JSON_TAG_MINFILTER] = (int) minFilter;
	jResourceMeta[JSON_TAG_MAGFILTER] = (int) magFilter;
}

void ResourceTexture::UpdateMinFilter(TextureMinFilter filter) {
	glBindTexture(GL_TEXTURE_2D, glTexture);
	switch (filter) {
	case TextureMinFilter::NEAREST:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	case TextureMinFilter::LINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		break;
	case TextureMinFilter::NEAREST_MIPMAP_NEAREST:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		break;
	case TextureMinFilter::LINEAR_MIPMAP_NEAREST:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		break;
	case TextureMinFilter::NEAREST_MIPMAP_LINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		break;
	case TextureMinFilter::LINEAR_MIPMAP_LINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		break;
	}
}

void ResourceTexture::UpdateMagFilter(TextureMagFilter filter) {
	glBindTexture(GL_TEXTURE_2D, glTexture);
	switch (filter) {
	case TextureMagFilter::NEAREST:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	case TextureMagFilter::LINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	}
}

void ResourceTexture::UpdateWrap(TextureWrap textureWrap) {
	glBindTexture(GL_TEXTURE_2D, glTexture);
	switch (textureWrap) {
	case TextureWrap::REPEAT:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		break;
	case TextureWrap::CLAMP_TO_EDGE:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		break;
	case TextureWrap::CLAMP_TO_BORDER:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		break;
	case TextureWrap::MIRROR_REPEAT:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		break;
	case TextureWrap::MIRROR_CLAMP_TO_EDGE:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRROR_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRROR_CLAMP_TO_EDGE);
		break;
	}
}