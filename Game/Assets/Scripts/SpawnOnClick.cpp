#include "SpawnOnClick.h"

#include "Resources/ResourcePrefab.h"
#include "GameObject.h"
#include "PlayerController.h"
#include "GameplaySystems.h"

#include "Geometry/LineSegment.h"
#include "Geometry/Plane.h"

EXPOSE_MEMBERS(SpawnOnClick) {
	MEMBER(MemberType::GAME_OBJECT_UID, cameraUID),
	MEMBER(MemberType::GAME_OBJECT_UID, enemiesUID),
	MEMBER(MemberType::PREFAB_RESOURCE_UID, prefabUID),
	MEMBER(MemberType::GAME_OBJECT_UID, playerUID)
};

GENERATE_BODY_IMPL(SpawnOnClick);

void SpawnOnClick::Start() {
	gameObject = &GetOwner();
	camera = GameplaySystems::GetGameObject(cameraUID);
	enemies = GameplaySystems::GetGameObject(enemiesUID);
	prefab = GameplaySystems::GetResource<ResourcePrefab>(prefabUID);

	GameObject* playerController = GameplaySystems::GetGameObject(playerUID);

	if (playerController) playerScript = GET_SCRIPT(playerController, PlayerController);
}

void SpawnOnClick::Update() {
	if (Input::GetKeyCode(Input::KEYCODE::KEY_LCTRL) && Input::GetMouseButtonUp(0)) {
		if (prefab != nullptr) {
			UID prefabId = prefab->BuildPrefab(enemies);
			GameObject* go = GameplaySystems::GetGameObject(prefabId);
			go->GetComponent<ComponentTransform>()->SetPosition(DetectMouseLocation());

			if (playerScript) playerScript->AddEnemyInMap(go);
		}
	}
}

float3 SpawnOnClick::DetectMouseLocation() {
	float2 mousePos = Input::GetMousePositionNormalized();
	ComponentTransform* compTransform = enemies->GetComponent<ComponentTransform>();
	if (camera && compTransform) {
		ComponentCamera* compCamera = camera->GetComponent<ComponentCamera>();
		LineSegment ray = compCamera->frustum.UnProjectLineSegment(mousePos.x, mousePos.y);

		Plane p = Plane(compTransform->GetGlobalPosition(), float3(0, 1, 0));

		return p.ClosestPoint(ray);
	}
	return float3(0, 0, 0);
}