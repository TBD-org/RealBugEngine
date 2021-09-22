#pragma once

#include "Components/Component.h"

#include "Components/UI/ComponentTransform2D.h"

#include "Math/float4.h"

#include <array>
#include <vector>

// Component that renders a Text, horizontally aligned based on the width of the component. The default value is Left alignment.
class ComponentText : public Component {
public:
	REGISTER_COMPONENT(ComponentText, ComponentType::TEXT, false);
	~ComponentText();

	void Init() override;			// Generates VBO and VAO of ComponentText
	void OnEditorUpdate() override; // Works as input of Text, FontSize, Color and ShaderID and FontID

	void Save(JsonValue jComponent) const override; // Serializes
	void Load(JsonValue jComponent) override;		// Deserializes

	void Draw(ComponentTransform2D* transform);				// Draws the text ortographically using the active camera and the position of the Tranform2D. It will apply the color as tint
	TESSERACT_ENGINE_API void SetText(const std::string& newText);	// Sets text
	TESSERACT_ENGINE_API void SetFontSize(float newfontSize);		// Sets fontSize
	TESSERACT_ENGINE_API void SetFontColor(const float4& newColor); // Sets color
	TESSERACT_ENGINE_API float4 GetFontColor() const;				// Returns Color

	void Invalidate();

private:
	void RecalculateVertices();								  // Recalculate verticesText. This is called when Text/Font/FontSize/LineHeight/Transform is modified in order to recalculate the position of vertices. Will calculate the position based on the horizontal Text Alignment
	float SubstringWidth(const char* substring, float scale); // Returns the advanced width of the substring until it reaches the end of line or new line character ('\0' or '\n').

private:
	enum TextAlignment {
		LEFT,
		CENTER,
		RIGHT
	};

	std::string text = "Text";									   // Text to display
	std::vector<std::array<std::array<float, 4>, 6>> verticesText; // Vertices per each character

	float fontSize = 24.0f;					 // Font size
	float4 color = float4::one;				 // Color of the font
	float lineHeight = 16.0f;				 // Line height
	float letterSpacing = 0.0f;				 // Space between letters
	int textAlignment = TextAlignment::LEFT; // Horizontal Alignment

	unsigned int vbo = 0; // VBO of the text
	unsigned int vao = 0; // VAO of the text
	UID fontID = 0;		  // Font ID of the text

	bool dirty = true;
};