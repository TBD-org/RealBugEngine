#pragma once

#include "Component.h"

#include "Math/float3.h"
#include "DetourTileCache/DetourTileCache.h"

class ComponentObstacle : public Component {
public:
	REGISTER_COMPONENT(ComponentObstacle, ComponentType::OBSTACLE, false); // Refer to ComponentType for the Constructor
	~ComponentObstacle();

	void Init() override;							// Add Obstacle to Mesh
	void Update() override;							// If position or rotation changed, reposition by Adding Obstacle
	void OnEditorUpdate() override;					// Radius, Height, or xyz values, Obstacle type can be udpated. If a value is changed, the Obstacle is first removed and then added again to the NavMesh.
	void OnEnable() override;						// Calls AddObstacle
	void OnDisable() override;						// Calls RemoveObstacle
	void Save(JsonValue jComponent) const override; // Serialize
	void Load(JsonValue jComponent) override;		// Deserialize

	TESSERACT_ENGINE_API void AddObstacle();		// If NavMesh and TileCache are built, Removes the current Obstacle and Adds it to the TileCache (NavMesh). Will be added as Box or as Cylinder depending on this.type.
	TESSERACT_ENGINE_API void RemoveObstacle();		// If NavMesh and TileCache are built, Removes the current Obstacle and obstacleReference is set to null

	TESSERACT_ENGINE_API void SetRadius(float newRadius);				// Sets obstacleSize.x = radius and calls AddObstacle
	TESSERACT_ENGINE_API void SetHeight(float newHeight);				// Sets obstacleSize.y = height and calls AddObstacle
	TESSERACT_ENGINE_API void SetBoxSize(float3 size);					// Sets obstacleSize = size and calls AddObstacle
	TESSERACT_ENGINE_API void SetObstacleType(ObstacleType newType);	// Sets obstacleType = newType and calls AddObstacle
	TESSERACT_ENGINE_API void ResetSize();								// Initis obstacleSize. If cylinder, size = {1.0f, 2.0f, 0f}, else size = {1.0f, 1.0f, 1.0f}

private:
	dtObstacleRef* obstacleReference = nullptr;							// Obstacle reference in the TileCache (NavMesh)
	float3 obstacleSize = float3::one;									// Obstacle Size. If Cylinder, radius = obstacleSize.x, height = obstacleSize.y
	float3 currentPosition = float3::zero;
	float3 currentRotation = float3::zero;
	int obstacleType = ObstacleType::DT_OBSTACLE_CYLINDER;
	bool mustBeDrawnGizmo = true;
	bool shouldAddObstacle = true;

private:
	void SetDrawGizmo(bool value);
};
