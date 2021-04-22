#include "ModuleScene.h"

#include "Globals.h"
#include "Application.h"
#include "Utils/Logging.h"
#include "Utils/FileDialog.h"
#include "FileSystem/SceneImporter.h"
#include "FileSystem/TextureImporter.h"
#include "FileSystem/JsonValue.h"
#include "Resources/ResourceTexture.h"
#include "Resources/ResourceSkybox.h"
#include "Components/Component.h"
#include "Components/ComponentTransform.h"
#include "Components/ComponentLight.h"
#include "Components/ComponentMeshRenderer.h"
#include "Components/ComponentBoundingBox.h"
#include "Components/ComponentCamera.h"
#include "Components/UI/ComponentCanvas.h"
#include "Components/UI/ComponentCanvasRenderer.h"
#include "Components/UI/ComponentTransform2D.h"
#include "Components/UI/ComponentImage.h"
#include "Modules/ModuleInput.h"
#include "Modules/ModulePrograms.h"
#include "Modules/ModuleCamera.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleRender.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleUserInterface.h"
#include "Modules/ModuleEvents.h"
#include "Modules/ModuleTime.h"
#include "Panels/PanelHierarchy.h"

#include "GL/glew.h"
#include "Math/myassert.h"
#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "Math/float4x4.h"
#include "Geometry/Sphere.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"
#include "rapidjson/error/en.h"
#include <string>
#include "Brofiler.h"

#include <Windows.h>
#include <array>

#include "Utils/Leaks.h"

static aiLogStream logStream = {nullptr, nullptr};

static void AssimpLogCallback(const char* message, char* user) {
	std::string messageStr = message;
	std::string finalMessageStr = messageStr.substr(0, messageStr.find_last_of('\n'));
	LOG(finalMessageStr.c_str());
}

bool ModuleScene::Init() {
	scene = new Scene(10000);

#ifdef _DEBUG
	logStream.callback = AssimpLogCallback;
	aiAttachLogStream(&logStream);
#endif

	return true;
}

bool ModuleScene::Start() {
	App->events->AddObserverToEvent(TesseractEventType::GAMEOBJECT_DESTROYED, this);
	App->events->AddObserverToEvent(TesseractEventType::ADD_COMPONENT, this);
	App->events->AddObserverToEvent(TesseractEventType::CHANGE_SCENE, this);
	App->events->AddObserverToEvent(TesseractEventType::RESOURCES_LOADED, this);

	App->files->CreateFolder(LIBRARY_PATH);
	App->files->CreateFolder(TEXTURES_PATH);
	App->files->CreateFolder(SCENES_PATH);

#if GAME
	App->events->AddEvent(TesseractEventType::PRESSED_PLAY);
	SceneImporter::LoadScene("Assets/Scenes/StartScene.scene");
	App->renderer->SetVSync(false);
	App->time->limitFramerate = false;
#else
	CreateEmptyScene();
#endif

	return true;
}

UpdateStatus ModuleScene::Update() {
	BROFILER_CATEGORY("ModuleScene - Update", Profiler::Color::Green)

	// Update GameObjects
	scene->root->Update();

	return UpdateStatus::CONTINUE;
}

bool ModuleScene::CleanUp() {
	scene->ClearScene();
	RELEASE(scene);

#ifdef _DEBUG
	aiDetachAllLogStreams();
#endif

	return true;
}

void ModuleScene::ReceiveEvent(TesseractEvent& e) {
	switch (e.type) {
	case TesseractEventType::GAMEOBJECT_DESTROYED:
		//scene->DestroyGameObject(e.destroyGameObject.gameObject);
		scene->DestroyGameObject(std::get<DestroyGameObjectStruct>(e.variant).gameObject);
		break;
	case TesseractEventType::ADD_COMPONENT:
		//scene->AddComponent(e.addComponent.component);
		scene->AddComponent(std::get<AddComponentStruct>(e.variant).component);
		break;
	case TesseractEventType::CHANGE_SCENE:
		sceneLoaded = false;
		//SceneImporter::LoadScene(e.changeScene.scenePath);
		SceneImporter::LoadScene(std::get<ChangeSceneStruct>(e.variant).scenePath);
		break;
	case TesseractEventType::RESOURCES_LOADED:
		if (App->time->IsGameRunning() && !sceneLoaded) {
			sceneLoaded = true;
			for (auto& it : scene->scriptComponents) {
				it.OnStart();
			}
		}
		break;
	}
}

void ModuleScene::CreateEmptyScene() {
	scene->ClearScene();

	// Create Scene root node
	GameObject* root = scene->CreateGameObject(nullptr, GenerateUID(), "Scene");
	scene->root = root;
	ComponentTransform* sceneTransform = root->CreateComponent<ComponentTransform>();
	root->InitComponents();

	// Create Directional Light
	GameObject* dirLight = scene->CreateGameObject(root, GenerateUID(), "Directional Light");
	ComponentTransform* dirLightTransform = dirLight->CreateComponent<ComponentTransform>();
	dirLightTransform->SetPosition(float3(0, 300, 0));
	dirLightTransform->SetRotation(Quat::FromEulerXYZ(pi / 2, 0.0f, 0.0));
	dirLightTransform->SetScale(float3(1, 1, 1));
	ComponentLight* dirLightLight = dirLight->CreateComponent<ComponentLight>();
	dirLight->InitComponents();

	// Create Game Camera
	GameObject* gameCamera = scene->CreateGameObject(root, GenerateUID(), "Game Camera");
	ComponentTransform* gameCameraTransform = gameCamera->CreateComponent<ComponentTransform>();
	gameCameraTransform->SetPosition(float3(2, 3, -5));
	gameCameraTransform->SetRotation(Quat::identity);
	gameCameraTransform->SetScale(float3(1, 1, 1));
	ComponentCamera* gameCameraCamera = gameCamera->CreateComponent<ComponentCamera>();
	ComponentSkyBox* gameCameraSkybox = gameCamera->CreateComponent<ComponentSkyBox>();
	gameCamera->InitComponents();
}

void ModuleScene::DestroyGameObjectDeferred(GameObject* gameObject) {
	if (gameObject == nullptr) return;

	const std::vector<GameObject*>& children = gameObject->GetChildren();
	for (GameObject* child : children) {
		DestroyGameObjectDeferred(child);
	}
	TesseractEvent e(TesseractEventType::GAMEOBJECT_DESTROYED);
	e.variant.emplace<DestroyGameObjectStruct>(gameObject);
	App->events->AddEvent(e);
}