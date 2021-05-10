#pragma once

#include "Scripting/Script.h"

class GameObject;

class EnemySpawnPoint : public Script {
	GENERATE_BODY(EnemySpawnPoint);

public:
	void Start() override;
	void Update() override;

public:
	UID prefabId = 0;
	int amountOfEnemies = 0;

private:
	GameObject* gameObject = nullptr;
	int iterator = 0;
	int offset = 1;
};
