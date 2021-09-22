#pragma once

#include "Module.h"
#include "Utils/Buffer.h"

#include "Navigation/NavMesh.h"
#include "Resources/ResourceNavMesh.h"
#include "DetourCrowd/DetourCrowd.h"
#include "Math/float3.h"

class ModuleNavigation : public Module {
public:
	bool Init() override;						   // Listens to PRESSED_PLAY and PRESSED_STOP events
	UpdateStatus Update() override;				   // Updates agents of navMesh's crowd
	void ReceiveEvent(TesseractEvent& e) override; // If PRESSED_PLAY Adds each agent. If PRESSED_STOP removes each agent.

	void BakeNavMesh(); // Builds new navMesh

	void DrawGizmos();																				 // Draws NavMesh Gizmos
	NavMesh* GetCurrentNavMesh();																	 // Returns navMesh
	void Raycast(float3 startPosition, float3 targetPosition, bool& hitResult, float3& hitPosition); // Shoots a raycast between startPosition and targetPosition and detects if there's a wall/obstacle between. Hitresult is set to true and hitPosition is the position where the wall has been detected
};
