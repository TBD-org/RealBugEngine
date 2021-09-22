#include "ComponentAgent.h"

#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleNavigation.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleEditor.h"
#include "Components/ComponentTransform.h"

#include "Utils/Leaks.h"

#define JSON_TAG_MAXSPEED "MaxSpeed"
#define JSON_TAG_MAXACCELERATION "MaxAcceleration"
#define JSON_TAG_AVOIDINGOBSTACLE "AvoidingObstacle"

void ComponentAgent::SetMoveTarget(float3 newTargetPosition, bool usePathfinding) {
	NavMesh* navMesh = GetOwner().scene->GetNavMesh();
	if (navMesh == nullptr || !navMesh->IsGenerated() || agentId == -1) return;

	// Find nearest point on navmesh and set move request to that location.
	dtNavMeshQuery* navquery = navMesh->GetNavMeshQuery();
	dtCrowd* crowd = navMesh->GetCrowd();
	const dtQueryFilter* filter = crowd->getFilter(0);
	const float* ext = crowd->getQueryExtents();

	if (usePathfinding) {
		navquery->findNearestPoly(newTargetPosition.ptr(), ext, filter, &targetPolygon, targetPosition.ptr());

		const dtCrowdAgent* ag = crowd->getAgent(agentId);
		if (ag && ag->active) {
			crowd->requestMoveTarget(agentId, targetPolygon, targetPosition.ptr());
		}

	} else {
		// Request velocity
		const dtCrowdAgent* ag = crowd->getAgent(agentId);
		if (ag && ag->active) {
			float3 targetResultPosition = (newTargetPosition - float3(ag->npos));
			if (!targetResultPosition.Equals(float3::zero)) {
				targetResultPosition.Normalize();
			}
			float3 vel = targetResultPosition * maxSpeed;
			crowd->requestMoveVelocity(agentId, vel.ptr());
		}
	}
}

void ComponentAgent::SetMaxSpeed(float newSpeed) {
	maxSpeed = newSpeed;

	NavMesh* navMesh = GetOwner().scene->GetNavMesh();
	if (navMesh == nullptr) return;

	dtCrowdAgent* ag = navMesh->GetCrowd()->getEditableAgent(agentId);
	if (ag == nullptr) return;

	ag->params.maxSpeed = maxSpeed;
}

void ComponentAgent::SetMaxAcceleration(float newAcceleration) {
	maxAcceleration = newAcceleration;

	NavMesh* navMesh = GetOwner().scene->GetNavMesh();
	if (navMesh == nullptr) return;

	dtCrowdAgent* ag = navMesh->GetCrowd()->getEditableAgent(agentId);
	if (ag == nullptr) return;

	ag->params.maxAcceleration = maxAcceleration;
}

void ComponentAgent::SetAgentObstacleAvoidance(bool avoidanceActive) {
	avoidingObstacle = avoidanceActive;

	NavMesh* navMesh = GetOwner().scene->GetNavMesh();
	if (navMesh == nullptr) return;

	dtCrowdAgent* ag = navMesh->GetCrowd()->getEditableAgent(agentId);
	if (ag == nullptr) return;

	if (avoidanceActive) {
		ag->params.updateFlags |= DT_CROWD_OBSTACLE_AVOIDANCE;
	} else {
		ag->params.updateFlags ^= DT_CROWD_OBSTACLE_AVOIDANCE;
	}
}

float ComponentAgent::GetMaxSpeed() {
	return maxSpeed;
}

float ComponentAgent::GetMaxAcceleration() {
	return maxAcceleration;
}

float3 ComponentAgent::GetTargetPosition() {
	return targetPosition;
}

bool ComponentAgent::IsAvoidingObstacle() {
	return avoidingObstacle;
}

void ComponentAgent::AddAgentToCrowd() {
	shouldAddAgentToCrowd = true;

	NavMesh* navMesh = GetOwner().scene->GetNavMesh();
	if (navMesh == nullptr || !navMesh->IsGenerated() || agentId != -1) return;

	// PARAMS INIT
	dtCrowdAgentParams ap;
	memset(&ap, 0, sizeof(ap));
	ap.radius = navMesh->agentRadius;
	ap.height = navMesh->agentHeight;
	ap.maxAcceleration = maxAcceleration;
	ap.maxSpeed = maxSpeed;
	ap.collisionQueryRange = ap.radius * 12.0f;
	ap.pathOptimizationRange = ap.radius * 30.0f;
	ap.updateFlags = 0;

	ap.updateFlags |= DT_CROWD_ANTICIPATE_TURNS;
	ap.updateFlags |= DT_CROWD_OPTIMIZE_VIS;
	ap.updateFlags |= DT_CROWD_OPTIMIZE_TOPO;
	ap.updateFlags |= DT_CROWD_SEPARATION;
	if (avoidingObstacle) {
		ap.updateFlags |= DT_CROWD_OBSTACLE_AVOIDANCE;
	}

	ap.obstacleAvoidanceType = 3;
	ap.separationWeight = 2;

	agentId = navMesh->GetCrowd()->addAgent(GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition().ptr(), &ap);
	shouldAddAgentToCrowd = false;
}

void ComponentAgent::RemoveAgentFromCrowd() {
	shouldAddAgentToCrowd = false;

	NavMesh* navMesh = GetOwner().scene->GetNavMesh();
	if (navMesh == nullptr || !navMesh->IsGenerated() || agentId == -1) return;

	navMesh->GetCrowd()->removeAgent(agentId);
	agentId = -1;
}

float3 ComponentAgent::GetVelocity() const {
	NavMesh* navMesh = GetOwner().scene->GetNavMesh();
	if (navMesh == nullptr || !navMesh->IsGenerated() || agentId == -1) return float3::zero;

	const dtCrowdAgent* ag = navMesh->GetCrowd()->getAgent(agentId);

	if (ag) {
		return float3(ag->vel);
	}
	return float3::zero;
}

ComponentAgent::~ComponentAgent() {
	RemoveAgentFromCrowd();
}

void ComponentAgent::Update() {
	if (!App->time->IsGameRunning()) return;

	NavMesh* navMesh = GetOwner().scene->GetNavMesh();
	if (navMesh == nullptr || !navMesh->IsGenerated()) return;

	// Try to add the agent to the crowd
	if (shouldAddAgentToCrowd) AddAgentToCrowd();
	if (agentId == -1) return;

	const dtCrowdAgent* ag = navMesh->GetCrowd()->getAgent(agentId);
	ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
	transform->SetGlobalPosition(float3(ag->npos));
}

void ComponentAgent::OnEditorUpdate() {
	if (ImGui::Checkbox("Active", &active)) {
		if (GetOwner().IsActive()) {
			if (active) {
				Enable();
			} else {
				Disable();
			}
		}
	}
	ImGui::Separator();

	if (ImGui::InputFloat("Agent max speed", &maxSpeed, App->editor->dragSpeed2f, 0)) {
		SetMaxSpeed(maxSpeed);
	}

	if (ImGui::InputFloat("Agent max acceleration", &maxAcceleration, App->editor->dragSpeed2f, 0)) {
		SetMaxAcceleration(maxAcceleration);
	}

	if (ImGui::Checkbox("Obstacle Avoidance", &avoidingObstacle)) {
		SetAgentObstacleAvoidance(avoidingObstacle);
	}
}

void ComponentAgent::OnEnable() {
	if (App->time->HasGameStarted()) {
		AddAgentToCrowd();
	}
}

void ComponentAgent::OnDisable() {
	if (App->time->HasGameStarted()) {
		RemoveAgentFromCrowd();
	}
}

void ComponentAgent::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_MAXSPEED] = maxSpeed;
	jComponent[JSON_TAG_MAXACCELERATION] = maxAcceleration;
	jComponent[JSON_TAG_AVOIDINGOBSTACLE] = avoidingObstacle;
}

void ComponentAgent::Load(JsonValue jComponent) {
	maxSpeed = jComponent[JSON_TAG_MAXSPEED];
	maxAcceleration = jComponent[JSON_TAG_MAXACCELERATION];
	avoidingObstacle = jComponent[JSON_TAG_AVOIDINGOBSTACLE];
}
