#pragma once

#include "Components/Component.h"

#include "Geometry/AABB2D.h"

class ComponentBoundingBox2D : public Component {
public:
	REGISTER_COMPONENT(ComponentBoundingBox2D, ComponentType::BOUNDING_BOX_2D, false);

	void Init() override;
	void OnEditorUpdate() override;
	void Update() override;
	void Save(JsonValue jComponent) const override;
	void Load(JsonValue jComponent) override;
	bool CanBeRemoved() const override;

	void SetLocalBoundingBox(const AABB2D& boundingBox);
	void CalculateWorldBoundingBox(bool force = false);
	void DrawGizmos();
	void Invalidate();
	const AABB2D& GetWorldAABB() const;

private:
	bool dirty = true;
	bool drawOutline = false;

	AABB2D localAABB = {{0, 0}, {0, 0}};
	AABB2D worldAABB = {{0, 0}, {0, 0}};
};