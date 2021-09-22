#pragma once

#include "Components/Component.h"

#include "Math/float4.h"
#include "Math/float2.h"

class ComponentTransform2D;

// Component that renders an Image on a Quad
class ComponentImage : public Component {
public:
	REGISTER_COMPONENT(ComponentImage, ComponentType::IMAGE, false);
	~ComponentImage();

	void Init() override;							// Inits the component
	void OnEditorUpdate() override;					// Works as input of the AlphaTransparency, color and Texture and Shader used
	void Save(JsonValue jComponent) const override; // Serializes object
	void Load(JsonValue jComponent) override;		// Deserializes object

	void Draw(ComponentTransform2D* transform) const; // Draws the image ortographically using the active camera, and the transform passed as model. It will apply AlphaTransparency if true, and will get Button's additional color to apply if needed
	TESSERACT_ENGINE_API void SetColor(float4 color_);
	TESSERACT_ENGINE_API void SetFillValue(float val);
	TESSERACT_ENGINE_API float4 GetColor() const;
	TESSERACT_ENGINE_API float4 GetMainColor() const; // Gets an additional color that needs to be applied to the image. Currently gets the color of the Button, Slider and Checkbox
	TESSERACT_ENGINE_API void SetIsFill(bool b);
	TESSERACT_ENGINE_API bool IsFill() const;
	TESSERACT_ENGINE_API bool HasAlphaTransparency() const;
	TESSERACT_ENGINE_API void SetAlphaTransparency(bool alphaTransparency);

	TESSERACT_ENGINE_API float2 GetTextureOffset() const;
	TESSERACT_ENGINE_API void SetTextureOffset(float2 offset);

	TESSERACT_ENGINE_API float2 GetTextureTiling() const;
	TESSERACT_ENGINE_API void SetTextureTiling(float2 tiling);

private:
	void RebuildFillQuadVBO();

private:
	float4 color = float4::one;		// Color used as default tainter
	bool alphaTransparency = false; // Enables Alpha Transparency of the image and the color
	bool isFill = false;			// Image rendered in function of fillVal
	UID textureID = 0;				// ID of the image
	float fillVal = 1.0f;			// Percent of image rendered (0 to 1)
	unsigned int fillQuadVBO = 0;
	float2 textureOffset = float2(0, 0);
	float2 textureTiling = float2(1, 1);
};
