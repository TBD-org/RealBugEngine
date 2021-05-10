#include "GameplaySystems.h"

#include "GameObject.h"
#include "Components/UI/ComponentTransform2D.h"
#include "Components/ComponentCamera.h"
#include "Application.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleInput.h"
#include "Modules/ModuleEvents.h"
#include "Modules/ModuleCamera.h"
#include "Modules/ModuleRender.h"
#include "Modules/ModuleWindow.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleRender.h"
#include "Modules/ModuleCamera.h"
#include "Resources/ResourcePrefab.h"
#include "FileSystem/SceneImporter.h"
#include "Utils/Logging.h"
#include "TesseractEvent.h"

#include "Geometry/Frustum.h"
#include "SDL_events.h"

#include "Utils/Leaks.h"

// ----------- GAMEPLAY ------------ //

GameObject* GameplaySystems::GetGameObject(const char* name) {
	GameObject* root = App->scene->scene->root;
	return root->name == name ? root : root->FindDescendant(name);
}

GameObject* GameplaySystems::GetGameObject(UID id) {
	return App->scene->scene->GetGameObject(id);
}

template<typename T>
T* GameplaySystems::GetResource(UID id) {
	return App->resources->GetResource<T>(id);
}

template TESSERACT_ENGINE_API ResourcePrefab* GameplaySystems::GetResource<ResourcePrefab>(UID id);

void GameplaySystems::SetRenderCamera(ComponentCamera* camera) {
	App->camera->ChangeActiveCamera(camera, true);
	App->camera->ChangeCullingCamera(camera, true);
}

// ------------- DEBUG ------------- //

void Debug::Log(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	LOG(fmt, args);
	va_end(args);
}

void Debug::ToggleDebugMode() {
	App->renderer->ToggleDebugMode();
}

void Debug::ToggleDebugDraw() {
	App->renderer->ToggleDebugDraw();
}

void Debug::ToggleDrawQuadtree() {
	App->renderer->ToggleDrawQuadtree();
}

void Debug::ToggleDrawBBoxes() {
	App->renderer->ToggleDrawBBoxes();
}

void Debug::ToggleDrawSkybox() {
	App->renderer->ToggleDrawSkybox();
}

void Debug::ToggleDrawAnimationBones() {
	App->renderer->ToggleDrawAnimationBones();
}

void Debug::ToggleDrawCameraFrustums() {
	App->renderer->ToggleDrawCameraFrustums();
}

void Debug::ToggleDrawLightGizmos() {
	App->renderer->ToggleDrawLightGizmos();
}
void Debug::ToggleDrawParticleGizmos() {
	App->renderer->ToggleDrawParticleGizmos();
}

void Debug::UpdateShadingMode(const char* shadingMode) {
	App->renderer->UpdateShadingMode(shadingMode);
}

int Debug::GetTotalTriangles() {
	return App->scene->scene->GetTotalTriangles();
}

int Debug::GetCulledTriangles() {
	return App->renderer->GetCulledTriangles();
}

const float3 Debug::GetCameraDirection() {
	return App->camera->GetActiveCamera()->GetFrustum()->Front();
}

// ------------- TIME -------------- //

float Time::GetDeltaTime() {
	return App->time->GetDeltaTime();
}

float Time::GetFPS() {
	return App->time->GetFPS();
}

float Time::GetMS() {
	return App->time->GetMS();
}

// ------------- INPUT ------------- //

bool Input::GetMouseButtonDown(int button) {
	return App->input->GetMouseButtons()[button] == KS_DOWN;
}

bool Input::GetMouseButtonUp(int button) {
	return App->input->GetMouseButtons()[button] == KS_UP;
}

bool Input::GetMouseButtonRepeat(int button) {
	return App->input->GetMouseButtons()[button] == KS_REPEAT;
}

bool Input::GetMouseButton(int button) {
	return App->input->GetMouseButtons()[button];
}

const float2& Input::GetMouseMotion() {
	return App->input->GetMouseMotion();
}

float2 Input::GetMousePosition() {
	return App->input->GetMousePosition(true);
}

bool Input::GetKeyCodeDown(KEYCODE keycode) {
	return App->input->GetKeyboard()[keycode] == KS_DOWN;
}

bool Input::GetKeyCodeUp(KEYCODE keycode) {
	return App->input->GetKeyboard()[keycode] == KS_UP;
}

bool Input::GetKeyCodeRepeat(KEYCODE keycode) {
	return App->input->GetKeyboard()[keycode] == KS_REPEAT;
}

bool Input::GetKeyCode(KEYCODE keycode) {
	return App->input->GetKeyboard()[keycode];
}

// --------- SCENE MANAGER --------- //

void SceneManager::ChangeScene(const char* scenePath) {
	TesseractEvent e(TesseractEventType::CHANGE_SCENE);
	e.Set<ChangeSceneStruct>(scenePath);
	App->events->AddEvent(e);
}

void SceneManager::ExitGame() {
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

float Screen::GetScreenWitdh() {
	return static_cast<float>(App->window->GetWidth());
}

float Screen::GetScreenHeight() {
	return static_cast<float>(App->window->GetHeight());
}