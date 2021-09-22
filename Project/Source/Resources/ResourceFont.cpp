#include "ResourceFont.h"

#include "Utils/Logging.h"
#include "Utils/MSTimer.h"
#include "Utils/PathUtils.h"

#include "ft2build.h"
#include "freetype/freetype.h"
#include "GL/glew.h"

#include "Utils/Leaks.h"

void ResourceFont::FinishLoading() {
	// Timer to measure loading a font
	MSTimer timer;
	timer.Start();
	std::string filePath = GetResourceFilePath();
	LOG("Loading font from path: \"%s\".", filePath.c_str());

	FT_Library ft;
	//Initiate freetype library. This piece of code should go into Module initialization
	if (FT_Init_FreeType(&ft)) {
		LOG("Could not init FreeType library.");
		return;
	}

	FT_Face face;

	//Load font as a face
	if (FT_New_Face(ft, filePath.c_str(), 0, &face)) {
		LOG("Could not load the font.");
		return;
	}

	// IF THE HEIGHT IS CHANGED (48), COMPONENTTEXT SCALINGFACTOR MUST BE CHANGED!
	FT_Set_Pixel_Sizes(face, 0, 48);

	//Disable byte-alignment restriction.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// TODO: read char from 32 to 128 + generate only one texture for the whole font
	for (unsigned char c = 0; c < 128; c++) {
		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
			LOG("Failed to load glyph.");
			continue;
		}

		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		Character character = {
			texture,
			float2(static_cast<float>(face->glyph->bitmap.width), static_cast<float>(face->glyph->bitmap.rows)),
			float2(static_cast<float>(face->glyph->bitmap_left), static_cast<float>(face->glyph->bitmap_top)),
			static_cast<unsigned int>(face->glyph->advance.x)};

		//Store the loaded glyph in the map for later use.
		characters.insert(std::pair<char, Character>(c, character));
	}

	//Reset pixel storage mode to default
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//Release resources used by freetype
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	name = PathUtils::GetFileName(filePath.c_str());
}

void ResourceFont::Unload() {
	for (const auto& entry : characters) {
		glDeleteTextures(1, &entry.second.textureID);
	}
	characters.clear();
}
