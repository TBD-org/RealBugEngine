#pragma once

#include "Modules/Module.h"
#include "Utils/UID.h"

#include <string>

class Scene;
class GameObject;

class ModuleScene : public Module {
public:
	// ------- Core Functions ------ //
	bool Init() override;
	bool Start() override;
	UpdateStatus Update() override;
	bool CleanUp() override;
	void ReceiveEvent(TesseractEvent& e) override;

	void CreateEmptyScene(); // Crates a new scene with a default game camera and directional light.
	void PreloadScene(UID sceneId);
	void ChangeScene(UID sceneId);
	Scene* GetCurrentScene();

	void BuildPrefab(UID prefabId, GameObject* parent);

	void LoadScene(const char* filePath);
	void SaveScene(const char* filePath);

	void DestroyGameObjectDeferred(GameObject* gameObject); //Event dependant destruction, Gameobjects are destroyed upon the receival of an event, so that info is not null

public:
	UID startSceneId = 0; // First scene to be loaded when in GAME configuration

	//Temporary hardcoded solution
	bool godModeOn = false;

private:
	Scene* scene = nullptr;

	bool shouldChangeScene = false;
	UID loadingSceneId = 0;

	bool shouldLoadScene = false;
	std::string sceneToLoadPath = "";

	bool shouldSaveScene = false;
	std::string sceneToSavePath = "";

	bool shouldBuildPrefab = false;
	UID buildingPrefabId = 0;
	UID buildingPrefabParentId = 0;
};
