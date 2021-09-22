#pragma once

#include "Utils/UID.h"
#include "Resources/Resource.h"
#include "Rendering/LightFrustum.h"

#include "Math/float4.h"
#include "Math/float2.h"

enum class MaterialShader {
	PHONG,
	STANDARD_SPECULAR,
	STANDARD,
	UNLIT,
	STANDARD_DISSOLVE,
	UNLIT_DISSOLVE,
	VOLUMETRIC_LIGHT
};

enum class RenderingMode {
	OPAQUE,
	TRANSPARENT
};

class ResourceMaterial : public Resource {
public:
	REGISTER_RESOURCE(ResourceMaterial, ResourceType::MATERIAL);

	void Load() override;
	void FinishLoading() override;
	void Unload() override;
	void OnEditorUpdate() override;

	void SaveToFile(const char* filePath);

public:
	// Material shader
	MaterialShader shaderType = MaterialShader::STANDARD;

	// Rendering Mode
	RenderingMode renderingMode = RenderingMode::OPAQUE;

	// Diffuse
	float4 diffuseColor = {1.0f, 1.0f, 1.0f, 1.0f};
	UID diffuseMapId = 0;

	// Specular
	float4 specularColor = {0.15f, 0.15f, 0.15f, 1.f};
	UID specularMapId = 0;

	// Metalness
	float metallic = 0.f;
	UID metallicMapId = 0;

	// Normal
	UID normalMapId = 0;
	float normalStrength = 1.f;

	// Emissive
	UID emissiveMapId = 0;
	float emissiveIntensity = .0f;
	float4 emissiveColor = float4::zero;

	// Ambien occlusion
	UID ambientOcclusionMapId = 0;

	// Smoothness
	float smoothness = 1;
	bool hasSmoothnessInAlphaChannel = false;

	// Tilling
	float2 tiling = {1.f, 1.f};
	float2 offset = {0.f, 0.f};

	// Dissolve Values. TODO: All Material properties should be converted into a map of properties and stored as is
	float dissolveScale = 10.0f;
	float2 dissolveOffset = float2::zero;
	float dissolveDuration = 1.0f;
	float dissolveEdgeSize = 0.0f;
	
	// Volumetric Light
	float volumetricLightInstensity = 1.0f;
	float volumetricLightAttenuationExponent = 1.0f;

	// Softness (transparency when near other meshes)
	bool isSoft = false;
	float softRange = 1.0f;

	//Shadows
	ShadowCasterType shadowCasterType = ShadowCasterType::STATIC;
	bool castShadows = false;
};