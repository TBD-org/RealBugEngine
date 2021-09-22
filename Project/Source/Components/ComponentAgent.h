#pragma once

#include "Components/Component.h"

#include "Math/float3.h"

class ComponentAgent : public Component {
public:
	REGISTER_COMPONENT(ComponentAgent, ComponentType::AGENT, false); // Refer to ComponentType for the Constructor
	~ComponentAgent();

	void Update() override;							// Updates Agent's position
	void OnEditorUpdate() override;					// MaxSpeed and MaxAcceleration can be udpated
	void OnEnable() override;						// If GameHasStarted, calls AddAgentToCrowd
	void OnDisable() override;						// If GameHasStarted, calls RemoveAgentFromCrowd
	void Save(JsonValue jComponent) const override; // Serialize
	void Load(JsonValue jComponent) override;		// Deserialize

	TESSERACT_ENGINE_API void SetMoveTarget(float3 newTargetPosition, bool usePathfinding = true); // This will set the parameters of the Agent to move to the target position
	TESSERACT_ENGINE_API void SetMaxSpeed(float newSpeed);										   // Sets agent MaxSpeed
	TESSERACT_ENGINE_API void SetMaxAcceleration(float newAcceleration);						   // Sets agent MaxAcceleration
	TESSERACT_ENGINE_API void SetAgentObstacleAvoidance(bool avoidanceActive);					   // Sets the Agent flag Avoidance to the value passed
	TESSERACT_ENGINE_API float GetMaxSpeed();													   // Returns maxSpeed
	TESSERACT_ENGINE_API float GetMaxAcceleration();											   // Returns maxAcceleration
	TESSERACT_ENGINE_API float3 GetTargetPosition();											   // Returns targetPosition
	TESSERACT_ENGINE_API bool IsAvoidingObstacle();												   // Returns avoidingObstacle

	TESSERACT_ENGINE_API void AddAgentToCrowd();	  // If possible, generates a new Agent and adds it to the NavMesh's crowd
	TESSERACT_ENGINE_API void RemoveAgentFromCrowd(); // If possible, removes Agent and adds it to the NavMesh's crowd
	TESSERACT_ENGINE_API float3 GetVelocity() const;

private:
	unsigned int targetPolygon = 0;		  // Target Polygon of the NavMesh to navigate
	float3 targetPosition = float3::zero; // Target position of the NavMesh to navigate
	int agentId = -1;					  // Agent identifier in NavMesh's crowd
	bool shouldAddAgentToCrowd = true;

	float maxSpeed = 5.0f;
	float maxAcceleration = 8.0f;
	bool avoidingObstacle = true;
};
