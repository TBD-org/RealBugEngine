#pragma once

#include "Component.h"

#include "Math/float3.h"
#include "Math/Quat.h"
#include "Math/float4x4.h"

class ComponentTransform : public Component {
public:
	REGISTER_COMPONENT(ComponentTransform, ComponentType::TRANSFORM, false); // Refer to ComponentType for the Constructor

	// ------- Core Functions ------ //
	void OnEditorUpdate() override;
	void Save(JsonValue jComponent) const override;
	void Load(JsonValue jComponent) override;
	void DuplicateComponent(GameObject& owner) override;

	void InvalidateHierarchy();						// Marks all the hierarchy of the owner GameObject as 'dirty'
	void CalculateGlobalMatrix(bool force = false); // Recalculates the Transformation Matrix from 'position', 'rotation' and 'scale'.

	// ---------- Setters ---------- //
	TESSERACT_ENGINE_API void SetPosition(float3 position);
	TESSERACT_ENGINE_API void SetRotation(Quat rotation);
	TESSERACT_ENGINE_API void SetRotation(float3 rotation);
	TESSERACT_ENGINE_API void SetScale(float3 scale);
	TESSERACT_ENGINE_API void SetTRS(float4x4& newTransform);
	TESSERACT_ENGINE_API void SetGlobalPosition(float3 position);
	TESSERACT_ENGINE_API void SetGlobalRotation(Quat rotation);
	TESSERACT_ENGINE_API void SetGlobalRotation(float3 rotation);
	TESSERACT_ENGINE_API void SetGlobalScale(float3 scale);
	TESSERACT_ENGINE_API void SetGlobalTRS(float4x4& newTransform);

	// ---------- Getters ---------- //

	TESSERACT_ENGINE_API float3 GetPosition() const;
	TESSERACT_ENGINE_API Quat GetRotation() const;
	TESSERACT_ENGINE_API float3 GetScale() const;
	TESSERACT_ENGINE_API float3 GetGlobalPosition();
	TESSERACT_ENGINE_API Quat GetGlobalRotation();
	TESSERACT_ENGINE_API float3 GetGlobalScale();
	TESSERACT_ENGINE_API const float4x4& GetLocalMatrix();
	TESSERACT_ENGINE_API const float4x4& GetGlobalMatrix();

private:
	float3 position = float3::zero;			// Position of the GameObject in world coordinates.
	Quat rotation = Quat::identity;			// Rotation of the GameObject as a Quaternion.
	float3 localEulerAngles = float3::zero; // Rotation of the GameObject as euler angles.
	float3 scale = float3::one;				// Scale of the GameObject.

	float4x4 localMatrix = float4x4::identity;	// Transform Matrix in local coordinates from its parent gameobject.
	float4x4 globalMatrix = float4x4::identity; // Transform Matrix in world coordinates.

	bool dirty = true; // If set to true CalculateGlobalMatrix() will update the Transform when called. Otherwise, it will skip the calculations.
};
