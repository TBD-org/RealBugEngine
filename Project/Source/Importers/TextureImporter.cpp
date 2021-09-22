#include "TextureImporter.h"

#include "Globals.h"
#include "Application.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleEditor.h"
#include "Resources/ResourceTexture.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
#include "Utils/MSTimer.h"
#include "Utils/PathUtils.h"
#include "ImporterCommon.h"

#include "IL/il.h"
#include "IL/ilu.h"
#include "GL/glew.h"
#include "rapidjson/prettywriter.h"

#include "Utils/Leaks.h"

#define JSON_TAG_FLIP "Flip"
#define JSON_TAG_COMPRESSION "Compression"
#define JSON_TAG_WRAP "Wrap"
#define JSON_TAG_MIN_FILTER "MinFilter"
#define JSON_TAG_MAG_FILTER "MagFilter"

void TextureImportOptions::ShowImportOptions() {
	// Flip
	ImGui::Checkbox("Flip", &flip);

	// Compression combo box
	const char* compression_items[] = {"None", "DXT1", "DXT3", "DXT5"};
	const char* compression_item_current = compression_items[int(compression)];
	if (ImGui::BeginCombo("Compression", compression_item_current)) {
		for (int n = 0; n < IM_ARRAYSIZE(compression_items); ++n) {
			bool is_selected = (compression_item_current == compression_items[n]);
			if (ImGui::Selectable(compression_items[n], is_selected)) {
				compression = TextureCompression(n);
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	// Filters
	ImGui::TextColored(App->editor->titleColor, "Filters");

	// Min filter combo box
	const char* min_filter_items[] = {"Nearest", "Linear", "Nearest Mipmap Nearest", "Linear Mipmap Nearest", "Nearest Mipmap Linear", "Linear Mipmap Linear"};
	const char* min_filter_item_current = min_filter_items[int(minFilter)];
	if (ImGui::BeginCombo("Min filter", min_filter_item_current)) {
		for (int n = 0; n < IM_ARRAYSIZE(min_filter_items); ++n) {
			bool is_selected = (min_filter_item_current == min_filter_items[n]);
			if (ImGui::Selectable(min_filter_items[n], is_selected)) {
				minFilter = TextureMinFilter(n);
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	// Mag filter combo box
	const char* mag_filter_items[] = {"Nearest", "Linear"};
	const char* mag_filter_item_current = mag_filter_items[int(magFilter)];
	if (ImGui::BeginCombo("Mag filter", mag_filter_item_current)) {
		for (int n = 0; n < IM_ARRAYSIZE(mag_filter_items); ++n) {
			bool is_selected = (mag_filter_item_current == mag_filter_items[n]);
			if (ImGui::Selectable(mag_filter_items[n], is_selected)) {
				magFilter = TextureMagFilter(n);
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	// Texture wrap combo box
	const char* wrap_items[] = {"Repeat", "Clamp to Edge", "Clamp to Border", "Mirrored Repeat", "Mirrored Clamp to Edge"};
	const char* wrap_item_current = wrap_items[int(wrap)];
	if (ImGui::BeginCombo("Wrap", wrap_item_current)) {
		for (int n = 0; n < IM_ARRAYSIZE(wrap_items); ++n) {
			bool is_selected = (wrap_item_current == wrap_items[n]);
			if (ImGui::Selectable(wrap_items[n], is_selected)) {
				wrap = TextureWrap(n);
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

void TextureImportOptions::Load(JsonValue jMeta) {
	flip = jMeta[JSON_TAG_FLIP];
	compression = (TextureCompression)(int) jMeta[JSON_TAG_COMPRESSION];
	wrap = (TextureWrap)(int) jMeta[JSON_TAG_WRAP];
	minFilter = (TextureMinFilter)(int) jMeta[JSON_TAG_MIN_FILTER];
	magFilter = (TextureMagFilter)(int) jMeta[JSON_TAG_MAG_FILTER];
}

void TextureImportOptions::Save(JsonValue jMeta) {
	jMeta[JSON_TAG_FLIP] = flip;
	jMeta[JSON_TAG_COMPRESSION] = (int) compression;
	jMeta[JSON_TAG_WRAP] = (int) wrap;
	jMeta[JSON_TAG_MIN_FILTER] = (int) minFilter;
	jMeta[JSON_TAG_MAG_FILTER] = (int) magFilter;
}

bool TextureImporter::ImportTexture(const char* filePath, JsonValue jMeta) {
	LOG("Importing texture from path: \"%s\".", filePath);

	// Timer to measure importing a texture
	MSTimer timer;
	timer.Start();

	// Load import options
	TextureImportOptions* importOptions = App->resources->GetImportOptions<TextureImportOptions>(filePath);
	if (!importOptions) {
		LOG("Failed to load import options.");
		return false;
	}

	// Generate image handler
	unsigned image;
	ilGenImages(1, &image);
	DEFER {
		ilDeleteImages(1, &image);
	};

	// Load image
	ilBindImage(image);
	bool imageLoaded = ilLoadImage(filePath);
	if (!imageLoaded) {
		LOG("Failed to load image.");
		return false;
	}

	ILenum format = ilGetInteger(IL_IMAGE_BPP) == 4 ? IL_RGBA : IL_RGB;
	bool imageConverted = ilConvertImage(format, IL_UNSIGNED_BYTE);
	if (!imageConverted) {
		LOG("Failed to convert image.");
		return false;
	}

	// Flip if asked to
	if (importOptions->flip) {
		iluFlipImage();
	}

	// Create texture resource
	unsigned resourceIndex = 0;
	std::unique_ptr<ResourceTexture> texture = ImporterCommon::CreateResource<ResourceTexture>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	texture->compression = importOptions->compression;
	texture->wrap = importOptions->wrap;
	texture->minFilter = importOptions->minFilter;
	texture->magFilter = importOptions->magFilter;

	ILenum type = IL_TGA;
	switch (importOptions->compression) {
	case TextureCompression::DXT1:
		ilSetInteger(IL_DXTC_FORMAT, IL_DXT1);
		type = IL_DDS;
		break;
	case TextureCompression::DXT3:
		ilSetInteger(IL_DXTC_FORMAT, IL_DXT3);
		type = IL_DDS;
		break;
	case TextureCompression::DXT5:
		ilSetInteger(IL_DXTC_FORMAT, IL_DXT5);
		type = IL_DDS;
		break;
	default:
		break;
	}

	// Save import options to the meta file
	importOptions->Save(jMeta);

	// Save image
	size_t size = ilSaveL(type, nullptr, 0);
	if (size == 0) {
		LOG("Failed to save image.");
		return false;
	}
	Buffer<char> buffer = Buffer<char>(size);
	size = ilSaveL(type, buffer.Data(), size);
	if (size == 0) {
		LOG("Failed to save image.");
		return false;
	}

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(texture.get());
	if (!saved) {
		LOG("Failed to save texture resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(texture->GetResourceFilePath().c_str(), buffer);
	if (!saved) {
		LOG("Failed to save texture resource file.");
		return false;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(texture);

	unsigned timeMs = timer.Stop();
	LOG("Texture imported in %ums", timeMs);
	return true;
}
