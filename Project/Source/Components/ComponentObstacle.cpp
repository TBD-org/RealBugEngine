#include "ComponentObstacle.h"

#include "GameObject.h"
#include "Application.h"
#include "Modules/ModuleNavigation.h"
#include "Modules/ModuleEditor.h"
#include "Navigation/NavMesh.h"

#include "Utils/Leaks.h"

#define JSON_TAG_SIZE "Size"
#define JSON_TAG_TYPE "ObstacleType"
#define JSON_TAG_DRAWGIZMO "DrawGizmo"

ComponentObstacle::~ComponentObstacle() {
	RemoveObstacle();
}

void ComponentObstacle::Init() {
	currentPosition = GetOwner().GetComponent<ComponentTransform>()->GetGlobalPosition();
	AddObstacle();
}

void ComponentObstacle::Update() {
	ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
	float3 newPosition = transform->GetGlobalPosition();
	float3 newRotation = transform->GetGlobalRotation().ToEulerXYZ();
	if (!newPosition.Equals(currentPosition) || !newRotation.Equals(currentRotation)) {
		currentPosition = newPosition;
		currentRotation = newRotation;
		AddObstacle();
	}

	// Try to add the obstacle
	if (shouldAddObstacle) AddObstacle();
}

void ComponentObstacle::OnEditorUpdate() {
	if (ImGui::Checkbox("Active", &active)) {
		if (GetOwner().IsActive()) {
			if (active) {
				Enable();
			} else {
				Disable();
			}
		}
	}

	ImGui::Text("Obstacle type");
	bool mustBeAdded = ImGui::RadioButton("Cylinder", &obstacleType, ObstacleType::DT_OBSTACLE_CYLINDER);
	ImGui::SameLine();
	mustBeAdded |= ImGui::RadioButton("Box", &obstacleType, ObstacleType::DT_OBSTACLE_ORIENTED_BOX);
	if (mustBeAdded) {
		SetObstacleType(static_cast<ObstacleType>(obstacleType));
	}

	if (obstacleType == ObstacleType::DT_OBSTACLE_CYLINDER) {
		if (ImGui::DragFloat("Cylinder radius", &obstacleSize.x, App->editor->dragSpeed2f, 0, inf)) {
			SetRadius(obstacleSize.x);
		}

		if (ImGui::DragFloat("Cylinder height", &obstacleSize.y, App->editor->dragSpeed2f, 0, inf)) {
			SetHeight(obstacleSize.y);
		}
	} else {
		if (ImGui::DragFloat3("Box Size", obstacleSize.ptr(), App->editor->dragSpeed2f, 0, inf)) {
			SetBoxSize(obstacleSize);
		}
	}

	ImGui::Text("");
	ImGui::TextWrapped("For Debug purposes only");
	if (ImGui::Checkbox("Draw Gizmos", &mustBeDrawnGizmo)) {
		SetDrawGizmo(mustBeDrawnGizmo);
	}
}

void ComponentObstacle::OnEnable() {
	AddObstacle();
}

void ComponentObstacle::OnDisable() {
	RemoveObstacle();
}

void ComponentObstacle::Save(JsonValue jComponent) const {
	JsonValue jSize = jComponent[JSON_TAG_SIZE];
	jSize[0] = obstacleSize.x;
	jSize[1] = obstacleSize.y;
	jSize[2] = obstacleSize.z;

	jComponent[JSON_TAG_TYPE] = obstacleType;
	jComponent[JSON_TAG_DRAWGIZMO] = mustBeDrawnGizmo;
}

void ComponentObstacle::Load(JsonValue jComponent) {
	obstacleType = jComponent[JSON_TAG_TYPE];
	mustBeDrawnGizmo = jComponent[JSON_TAG_DRAWGIZMO];

	JsonValue jSize = jComponent[JSON_TAG_SIZE];
	obstacleSize.Set(jSize[0], jSize[1], jSize[2]);
}

void ComponentObstacle::AddObstacle() {
	shouldAddObstacle = true;

	NavMesh* navMesh = GetOwner().scene->GetNavMesh();
	if (navMesh == nullptr || !navMesh->IsGenerated()) return;

	dtTileCache* tileCache = navMesh->GetTileCache();
	if (!tileCache) return;

	RemoveObstacle();

	obstacleReference = new dtObstacleRef;

	ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
	float3 position = transform->GetGlobalPosition();

	switch (obstacleType) {
	case ObstacleType::DT_OBSTACLE_CYLINDER:
		tileCache->addObstacle(&position[0], obstacleSize.x, obstacleSize.y, obstacleReference, mustBeDrawnGizmo);
		break;
	default:	// DT_OBSTACLE_BOX ||  DT_OBSTACLE_ORIENTED_BOX
		tileCache->addBoxObstacle(&position[0], &(obstacleSize / 2)[0], transform->GetGlobalRotation().ToEulerXYZ().y, obstacleReference, mustBeDrawnGizmo);
		break;
	}

	shouldAddObstacle = false;
}

void ComponentObstacle::RemoveObstacle() {
	shouldAddObstacle = false;

	NavMesh* navMesh = GetOwner().scene->GetNavMesh();
	if (navMesh == nullptr || !navMesh->IsGenerated()) return;

	dtTileCache* tileCache = navMesh->GetTileCache();
	if (!tileCache || !obstacleReference) return;

	tileCache->removeObstacle(*obstacleReference);
	RELEASE(obstacleReference);
	obstacleReference = nullptr;
}

void ComponentObstacle::SetRadius(float newRadius) {
	if (obstacleType == ObstacleType::DT_OBSTACLE_CYLINDER) {
		obstacleSize.x = newRadius;
		AddObstacle();
	}
}

void ComponentObstacle::SetHeight(float newHeight) {
	if (obstacleType == ObstacleType::DT_OBSTACLE_CYLINDER) {
		obstacleSize.y = newHeight;
		AddObstacle();
	}
}

void ComponentObstacle::SetBoxSize(float3 size) {
	if (obstacleType == ObstacleType::DT_OBSTACLE_BOX || obstacleType == ObstacleType::DT_OBSTACLE_ORIENTED_BOX) {
		obstacleSize = size;
		AddObstacle();
	}
}

void ComponentObstacle::SetObstacleType(ObstacleType newType) {
	obstacleType = newType;
	AddObstacle();
}

void ComponentObstacle::ResetSize() {
	if (obstacleType == ObstacleType::DT_OBSTACLE_CYLINDER) {
		obstacleSize.x = 1.0f;
		obstacleSize.y = 2.0f;
		obstacleSize.z = 0;
	} else {
		obstacleSize = float3::one;
	}
}

void ComponentObstacle::SetDrawGizmo(bool value) {
	mustBeDrawnGizmo = value;
	AddObstacle();
}
