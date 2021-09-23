#include "EnemySpawnPoint.h"

#include "Resources/ResourcePrefab.h"
#include "PlayerController.h"
#include "GameplaySystems.h"
#include "GameObject.h"

EXPOSE_MEMBERS(EnemySpawnPoint) {
	MEMBER(MemberType::GAME_OBJECT_UID, playerUID),
	MEMBER(MemberType::FLOAT, xAxisPos),
	MEMBER(MemberType::FLOAT, zAxisPos),
	MEMBER(MemberType::INT, offset),
	MEMBER(MemberType::INT, firstWaveMeleeAmount),
	MEMBER(MemberType::INT, firstWaveRangeAmount),
	MEMBER(MemberType::INT, secondWaveMeleeAmount),
	MEMBER(MemberType::INT, secondWaveRangeAmount),
	MEMBER(MemberType::INT, thirdWaveMeleeAmount),
	MEMBER(MemberType::INT, thirdWaveRangeAmount),
	MEMBER(MemberType::INT, fourthWaveMeleeAmount),
	MEMBER(MemberType::INT, fourthWaveRangeAmount),
	MEMBER(MemberType::INT, fifthWaveMeleeAmount),
	MEMBER(MemberType::INT, fifthWaveRangeAmount),
};

GENERATE_BODY_IMPL(EnemySpawnPoint);

void EnemySpawnPoint::Start() {
	/* Owner */
	gameObject = &GetOwner();

	/* Load the wave config into the enemy vector */
	enemies.emplace_back(firstWaveMeleeAmount, firstWaveRangeAmount);
	enemies.emplace_back(secondWaveMeleeAmount, secondWaveRangeAmount);
	enemies.emplace_back(thirdWaveMeleeAmount, thirdWaveRangeAmount);
	enemies.emplace_back(fourthWaveMeleeAmount, fourthWaveRangeAmount);
	enemies.emplace_back(fifthWaveMeleeAmount, fifthWaveRangeAmount);
	it = enemies.begin();

	for (auto const& [melee, range] : enemies) amountOfEnemies += melee + range;

	spawnPointControllerScript = GET_SCRIPT(gameObject->GetParent(), SpawnPointController);
	if (spawnPointControllerScript) {
		meleeEnemyPrefab = spawnPointControllerScript->GetMeleePrefab();
		rangeEnemyPrefab = spawnPointControllerScript->GetRangePrefab();
		spawnPointControllerScript->SetCurrentEnemyAmount(index, amountOfEnemies);
	}

	GameObject* player = GameplaySystems::GetGameObject(playerUID);
	if (player) {
		playerScript = GET_SCRIPT(player, PlayerController);
	}
}

void EnemySpawnPoint::Update() {
	if (!gameObject->IsActive() || !meleeEnemyPrefab || !rangeEnemyPrefab) return;

	if (it != enemies.end() && spawn) {
		RenderEnemy(EnemyType::MELEE, std::get<0>(*it));

		/* Reset the X pos of the enemy spawn location */
		xAxisPos = 0;
		RenderEnemy(EnemyType::RANGE, std::get<1>(*it));

		/* Stop the spawn until all the wave enemies die */
		spawn = false;
		waveRemainingEnemies = std::get<0>(*it) + std::get<1>(*it);
		spawnPointControllerScript->SetCurrentEnemyAmount(index, waveRemainingEnemies);
	}

	/* Condition to spawn the next wave */
	if (waveRemainingEnemies == 0 && spawnPointControllerScript->CanSpawn()) {
		spawn = true;
		if (it != enemies.end()) {
			it++;
		}
		else {
			spawnPointControllerScript->SetEnemySpawnStatus(index, false);
		}
	}
}

/* Called each time an enemy dies */
void EnemySpawnPoint::UpdateRemainingEnemies() {
	waveRemainingEnemies--;
	spawnPointControllerScript->SetCurrentEnemyAmount(index, waveRemainingEnemies);
}

void EnemySpawnPoint::RenderEnemy(EnemyType type, unsigned int amount) {
	for (unsigned int i = 0; i < amount; ++i) {
		UID prefabUID = type == EnemyType::MELEE ? meleeEnemyPrefab->BuildPrefab(gameObject) : rangeEnemyPrefab->BuildPrefab(gameObject);
		GameObject* go = GameplaySystems::GetGameObject(prefabUID);
		ComponentTransform* goTransform = go->GetComponent<ComponentTransform>();
		if (go) {
			ComponentBoundingBox* goBounds = nullptr;
			for (auto& child : go->GetChildren()) {
				if (child->HasComponent<ComponentMeshRenderer>()) {
					goBounds = child->GetComponent<ComponentBoundingBox>();
					break;
				}
			}
			if (goTransform && goBounds) {
				/* Spawn range enemies in the back */
				float3 newPosition = float3(0, 0, type == EnemyType::RANGE ? zAxisPos : 0);
				float newXval = goBounds->GetLocalMaxPointAABB().x - abs(goBounds->GetLocalMinPointAABB().x);
				newXval = newXval < 1.f ? 1.f : newXval;
				newPosition.x += xAxisPos * offset * newXval;
				goTransform->SetPosition(newPosition);
				/* After an enemy is spawned at a certain location th next one with be next to it */
				xAxisPos++;
			}

			if (playerScript) playerScript->AddEnemyInMap(go);
		}
	}
}