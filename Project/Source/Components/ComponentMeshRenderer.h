#pragma once

#include "Component.h"

#include "Modules/ModuleRender.h"

#include "Math/float2.h"
#include "Math/float4x4.h"
#include <unordered_map>

struct aiMesh;

class ComponentMeshRenderer : public Component {
public:
	REGISTER_COMPONENT(ComponentMeshRenderer, ComponentType::MESH_RENDERER, true);
	~ComponentMeshRenderer();

	void Init() override;
	void Start() override;
	void Update() override;

	void OnEditorUpdate() override;
	void Save(JsonValue jComponent) const override;
	void Load(JsonValue jComponent) override;

	void Draw(const float4x4& modelMatrix) const;
	void DrawDepthPrepass(const float4x4& modelMatrix) const;
	void DrawShadow(const float4x4& modelMatrix, unsigned int i, ShadowCasterType lightFrustumType) const;

	TESSERACT_ENGINE_API UID GetMeshID() const;
	TESSERACT_ENGINE_API UID GetMaterialID() const;
	void SetMeshID(UID meshId);
	void SetMaterialID(UID materialId);
	TESSERACT_ENGINE_API void ChangeMesh(UID meshId);
	TESSERACT_ENGINE_API void ChangeMaterial(UID materialId);

	void SetGameObjectBones(const std::unordered_map<std::string, GameObject*>& goBones);

	// Dissolve
	TESSERACT_ENGINE_API void PlayDissolveAnimation(bool reverse = false);	// Plays Dissolve animation. If reverse is true, it will go from transparent to material's color, else it will go from color to transparent
	TESSERACT_ENGINE_API void ResetDissolveValues();				// Resets current timer, dissolve threshold and reverse to false
	
	TESSERACT_ENGINE_API float GetDissolveDuration() const;				// Returns material's dissolve duration
	TESSERACT_ENGINE_API float GetDissolveThreshold() const;			// Returns current dissolve threshold. From 0 to 1.
	TESSERACT_ENGINE_API bool HasDissolveAnimationFinished() const;		// Returns true if the dissolve animation has finished
	TESSERACT_ENGINE_API bool IsDissolvePlayingOnReverse() const;		// Returns true if the dissolve animation is playing on reverse

	// Tilling
	TESSERACT_ENGINE_API float2 GetTextureTiling() const;
	TESSERACT_ENGINE_API float2 GetTextureOffset() const;
	TESSERACT_ENGINE_API void SetTextureTiling(float2 _tiling);
	TESSERACT_ENGINE_API void SetTextureOffset(float2 _offset);

private:
	void UpdateDissolveAnimation();
	float GetDissolveValue() const;

	float2 ChooseTextureTiling(float2 value) const;
	float2 ChooseTextureOffset(float2 value) const;

private:
	UID meshId = 0;
	UID materialId = 0;

	std::vector<float4x4> palette;

	std::unordered_map<std::string, GameObject*> goBones;

	bool bbActive = false;

	// Dissolve variables
	float currentTime = 0.0f;
	float dissolveThreshold = 0.0f;
	bool dissolveAnimationFinished = true;
	bool dissolveAnimationReverse = false;

	// Tiling variable
	float2 textureTiling = {1.0f, 1.0f};
	float2 textureOffset = {0.0f, 0.0f};
};
