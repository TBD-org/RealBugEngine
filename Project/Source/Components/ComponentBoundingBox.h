#pragma once

#include "Component.h"

#include "Geometry/AABB.h"
#include "Geometry/OBB.h"

class ComponentBoundingBox : public Component {
public:
	REGISTER_COMPONENT(ComponentBoundingBox, ComponentType::BOUNDING_BOX, false); // Refer to ComponentType for the Constructor

	// ------- Core Functions ------ //
	void OnEditorUpdate() override;
	void Save(JsonValue jComponent) const override;
	void Load(JsonValue jComponent) override;
	void DuplicateComponent(GameObject& owner) override;

	void CalculateWorldBoundingBox(bool force = false); // Recalculates the OBB when the transform of the GameObject has changed.
	void DrawBoundingBox();								// Send to render the edges of the worldOBB.

	const TESSERACT_ENGINE_API float3 GetLocalMinPointAABB();
	const TESSERACT_ENGINE_API float3 GetLocalMaxPointAABB();

	// ---------- Setters ---------- //
	void Invalidate(); // Sets dirty to true. This function must be called any time the Transform of the GameObject has changed, to recalculate the BBs on the next frame.
	void SetLocalBoundingBox(const AABB& boundingBox);

	// ---------- Getters ---------- //
	const OBB& GetWorldOBB();
	const AABB& GetWorldAABB();

private:
	AABB localAABB = {{0, 0, 0}, {0, 0, 0}}; // Axis Aligned Bounding Box, local to the GameObject
	AABB worldAABB = {{0, 0, 0}, {0, 0, 0}}; // Axis Aligned Bounding Box in world coordinates. Used for Culling and other camera calculations.
	OBB worldOBB = {worldAABB};				 // Oriented Bounding Box. This is the one that will be rendered.
	bool dirty = true;						 // If set to true CalculateWorldBoundingBox() will update the BBs on the next frame. Otherwise, it will skip the calculations.
};
