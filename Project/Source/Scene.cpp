#include "Scene.h"

#include "GameObject.h"
#include "Application.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleRender.h"
#include "Modules/ModulePhysics.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleCamera.h"
#include "Modules/ModuleRender.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleWindow.h"
#include "Modules/ModuleProject.h"
#include "Modules/ModuleResources.h"
#include "Scripting/PropertyMap.h"
#include "Resources/ResourceMesh.h"
#include "Resources/ResourceMaterial.h"
#include "Resources/ResourceNavMesh.h"
#include "Utils/Logging.h"

#include "Utils/Leaks.h"

#define JSON_TAG_ROOT "Root"
#define JSON_TAG_QUADTREE_BOUNDS "QuadtreeBounds"
#define JSON_TAG_QUADTREE_MAX_DEPTH "QuadtreeMaxDepth"
#define JSON_TAG_QUADTREE_ELEMENTS_PER_NODE "QuadtreeElementsPerNode"
#define JSON_TAG_GAME_CAMERA "GameCamera"
#define JSON_TAG_AMBIENTLIGHT "AmbientLight"
#define JSON_TAG_NAVMESH "NavMesh"
#define JSON_TAG_CURSOR_WIDTH "CursorWidth"
#define JSON_TAG_CURSOR_HEIGHT "CursorHeight"
#define JSON_TAG_CURSOR "Cursor"

Scene::Scene(unsigned numGameObjects) {
	gameObjects.Allocate(numGameObjects);

	transformComponents.Allocate(numGameObjects);
	meshRendererComponents.Allocate(numGameObjects);
	boundingBoxComponents.Allocate(numGameObjects);
	cameraComponents.Allocate(numGameObjects);
	lightComponents.Allocate(numGameObjects);
	canvasComponents.Allocate(numGameObjects);
	canvasRendererComponents.Allocate(numGameObjects);
	imageComponents.Allocate(numGameObjects);
	transform2DComponents.Allocate(numGameObjects);
	boundingBox2DComponents.Allocate(numGameObjects);
	eventSystemComponents.Allocate(numGameObjects);
	toggleComponents.Allocate(numGameObjects);
	textComponents.Allocate(numGameObjects);
	buttonComponents.Allocate(numGameObjects);
	selectableComponents.Allocate(numGameObjects);
	sliderComponents.Allocate(numGameObjects);
	skyboxComponents.Allocate(numGameObjects);
	scriptComponents.Allocate(numGameObjects);
	animationComponents.Allocate(numGameObjects);
	particleComponents.Allocate(numGameObjects);
	trailComponents.Allocate(numGameObjects);
	audioSourceComponents.Allocate(numGameObjects);
	audioListenerComponents.Allocate(numGameObjects);
	progressbarsComponents.Allocate(numGameObjects);
	billboardComponents.Allocate(numGameObjects);
	sphereColliderComponents.Allocate(numGameObjects);
	boxColliderComponents.Allocate(numGameObjects);
	capsuleColliderComponents.Allocate(numGameObjects);
	agentComponents.Allocate(numGameObjects);
	obstacleComponents.Allocate(numGameObjects);
	fogComponents.Allocate(numGameObjects);
	videoComponents.Allocate(numGameObjects);
}

Scene::~Scene() {
	ClearScene();
}

void Scene::ClearScene() {
	DestroyGameObject(root);
	root = nullptr;
	quadtree.Clear();
	SetNavMesh(0);
	SetCursor(0);

	assert(gameObjects.Count() == 0); // There should be no GameObjects outside the scene hierarchy
	gameObjects.Clear();			  // This looks redundant, but it resets the free list so that GameObject order is mantained when saving/loading
}

void Scene::RebuildQuadtree() {
	quadtree.Initialize(quadtreeBounds, quadtreeMaxDepth, quadtreeElementsPerNode);
	for (ComponentBoundingBox& boundingBox : boundingBoxComponents) {
		GameObject& gameObject = boundingBox.GetOwner();
		if (gameObject.IsStatic()) {
			boundingBox.CalculateWorldBoundingBox();
			const AABB& worldAABB = boundingBox.GetWorldAABB();
			quadtree.Add(&gameObject, AABB2D(worldAABB.minPoint.xz(), worldAABB.maxPoint.xz()));
			gameObject.isInQuadtree = true;
		}
	}
	quadtree.Optimize();
}

void Scene::ClearQuadtree() {
	quadtree.Clear();
	for (GameObject& gameObject : gameObjects) {
		gameObject.isInQuadtree = false;
	}
}

void Scene::Init() {
	App->resources->IncreaseReferenceCount(navMeshId);
	App->resources->IncreaseReferenceCount(cursorId);

	root->Init();
}

void Scene::Start() {
	App->project->GetGameState()->Clear();

	if (App->camera->GetGameCamera()) {
		// Set the Game Camera as active
		App->camera->ChangeActiveCamera(App->camera->GetGameCamera(), true);
		App->camera->ChangeCullingCamera(App->camera->GetGameCamera(), true);
	} else {
		// TODO: Modal window. Warning - camera not set.
	}

	App->window->SetCursor(cursorId, widthCursor, heightCursor);
	App->window->ActivateCursor(true);

	root->Start();
}

void Scene::Load(JsonValue jScene) {
	ClearScene();

	// Load GameObjects
	JsonValue jRoot = jScene[JSON_TAG_ROOT];
	root = gameObjects.Obtain(0);
	root->scene = this;
	root->Load(jRoot);

	// Quadtree generation
	JsonValue jQuadtreeBounds = jScene[JSON_TAG_QUADTREE_BOUNDS];
	quadtreeBounds = {{jQuadtreeBounds[0], jQuadtreeBounds[1]}, {jQuadtreeBounds[2], jQuadtreeBounds[3]}};
	quadtreeMaxDepth = jScene[JSON_TAG_QUADTREE_MAX_DEPTH];
	quadtreeElementsPerNode = jScene[JSON_TAG_QUADTREE_ELEMENTS_PER_NODE];
	RebuildQuadtree();

	// Game Camera
	gameCameraId = jScene[JSON_TAG_GAME_CAMERA];

	// Ambient Light
	JsonValue ambientLight = jScene[JSON_TAG_AMBIENTLIGHT];
	ambientColor = {ambientLight[0], ambientLight[1], ambientLight[2]};

	// NavMesh
	navMeshId = jScene[JSON_TAG_NAVMESH];

	// Cursor
	heightCursor = jScene[JSON_TAG_CURSOR_HEIGHT];
	widthCursor = jScene[JSON_TAG_CURSOR_WIDTH];
	cursorId = jScene[JSON_TAG_CURSOR];
}

void Scene::Save(JsonValue jScene) const {
	// Save scene information
	JsonValue jQuadtreeBounds = jScene[JSON_TAG_QUADTREE_BOUNDS];
	jQuadtreeBounds[0] = quadtreeBounds.minPoint.x;
	jQuadtreeBounds[1] = quadtreeBounds.minPoint.y;
	jQuadtreeBounds[2] = quadtreeBounds.maxPoint.x;
	jQuadtreeBounds[3] = quadtreeBounds.maxPoint.y;
	jScene[JSON_TAG_QUADTREE_MAX_DEPTH] = quadtreeMaxDepth;
	jScene[JSON_TAG_QUADTREE_ELEMENTS_PER_NODE] = quadtreeElementsPerNode;

	ComponentCamera* gameCamera = App->camera->GetGameCamera();
	jScene[JSON_TAG_GAME_CAMERA] = gameCamera ? gameCamera->GetID() : 0;

	JsonValue ambientLight = jScene[JSON_TAG_AMBIENTLIGHT];
	ambientLight[0] = App->renderer->ambientColor.x;
	ambientLight[1] = App->renderer->ambientColor.y;
	ambientLight[2] = App->renderer->ambientColor.z;

	// NavMesh
	jScene[JSON_TAG_NAVMESH] = navMeshId;

	// Cursor
	jScene[JSON_TAG_CURSOR_HEIGHT] = heightCursor;
	jScene[JSON_TAG_CURSOR_WIDTH] = widthCursor;
	jScene[JSON_TAG_CURSOR] = cursorId;

	// Save GameObjects
	JsonValue jRoot = jScene[JSON_TAG_ROOT];
	root->Save(jRoot);
}

GameObject* Scene::CreateGameObject(GameObject* parent, UID id, const char* name) {
	GameObject* gameObject = gameObjects.Obtain(id);
	gameObject->scene = this;
	gameObject->id = id;
	gameObject->name = name;
	gameObject->SetParent(parent);

	return gameObject;
}

void Scene::DestroyGameObject(GameObject* gameObject) {
	if (gameObject == nullptr) return;

	// If the removed GameObject is the directionalLight of the scene, set it to nullptr
	if (gameObject == directionalLight) directionalLight = nullptr;

	// We need a copy because we are invalidating the iterator by removing GameObjects
	std::vector<GameObject*> children = gameObject->GetChildren();
	for (GameObject* child : children) {
		DestroyGameObject(child);
	}

	if (gameObject->isInQuadtree) {
		quadtree.Remove(gameObject);
	}

	bool selected = App->editor->selectedGameObject == gameObject;
	if (selected) App->editor->selectedGameObject = nullptr;

	gameObject->RemoveAllComponents();
	gameObject->SetParent(nullptr);
	gameObjects.Release(gameObject->GetID());
}

GameObject* Scene::GetGameObject(UID id) const {
	return gameObjects.Find(id);
}

Component* Scene::GetComponentByTypeAndId(ComponentType type, UID componentId) {
	switch (type) {
	case ComponentType::TRANSFORM:
		return transformComponents.Find(componentId);
	case ComponentType::MESH_RENDERER:
		return meshRendererComponents.Find(componentId);
	case ComponentType::BOUNDING_BOX:
		return boundingBoxComponents.Find(componentId);
	case ComponentType::CAMERA:
		return cameraComponents.Find(componentId);
	case ComponentType::LIGHT:
		return lightComponents.Find(componentId);
	case ComponentType::CANVAS:
		return canvasComponents.Find(componentId);
	case ComponentType::CANVASRENDERER:
		return canvasRendererComponents.Find(componentId);
	case ComponentType::IMAGE:
		return imageComponents.Find(componentId);
	case ComponentType::TRANSFORM2D:
		return transform2DComponents.Find(componentId);
	case ComponentType::BUTTON:
		return buttonComponents.Find(componentId);
	case ComponentType::EVENT_SYSTEM:
		return eventSystemComponents.Find(componentId);
	case ComponentType::BOUNDING_BOX_2D:
		return boundingBox2DComponents.Find(componentId);
	case ComponentType::TOGGLE:
		return toggleComponents.Find(componentId);
	case ComponentType::TEXT:
		return textComponents.Find(componentId);
	case ComponentType::SELECTABLE:
		return selectableComponents.Find(componentId);
	case ComponentType::SLIDER:
		return sliderComponents.Find(componentId);
	case ComponentType::SKYBOX:
		return skyboxComponents.Find(componentId);
	case ComponentType::ANIMATION:
		return animationComponents.Find(componentId);
	case ComponentType::SCRIPT:
		return scriptComponents.Find(componentId);
	case ComponentType::PARTICLE:
		return particleComponents.Find(componentId);
	case ComponentType::TRAIL:
		return trailComponents.Find(componentId);
	case ComponentType::BILLBOARD:
		return billboardComponents.Find(componentId);
	case ComponentType::AUDIO_SOURCE:
		return audioSourceComponents.Find(componentId);
	case ComponentType::AUDIO_LISTENER:
		return audioListenerComponents.Find(componentId);
	case ComponentType::PROGRESS_BAR:
		return progressbarsComponents.Find(componentId);
	case ComponentType::SPHERE_COLLIDER:
		return sphereColliderComponents.Find(componentId);
	case ComponentType::BOX_COLLIDER:
		return boxColliderComponents.Find(componentId);
	case ComponentType::CAPSULE_COLLIDER:
		return capsuleColliderComponents.Find(componentId);
	case ComponentType::AGENT:
		return agentComponents.Find(componentId);
	case ComponentType::OBSTACLE:
		return obstacleComponents.Find(componentId);
	case ComponentType::FOG:
		return fogComponents.Find(componentId);
	case ComponentType::VIDEO:
		return videoComponents.Find(componentId);
	default:
		LOG("Component of type %i hasn't been registered in Scene::GetComponentByTypeAndId.", (unsigned) type);
		assert(false);
		return nullptr;
	}
}

Component* Scene::CreateComponentByTypeAndId(GameObject* owner, ComponentType type, UID componentId) {
	switch (type) {
	case ComponentType::TRANSFORM:
		return transformComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::MESH_RENDERER:
		return meshRendererComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::BOUNDING_BOX:
		return boundingBoxComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::CAMERA:
		return cameraComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::LIGHT:
		return lightComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::CANVAS:
		return canvasComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::CANVASRENDERER:
		return canvasRendererComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::IMAGE:
		return imageComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::TRANSFORM2D:
		return transform2DComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::BUTTON:
		return buttonComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::EVENT_SYSTEM:
		return eventSystemComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::BOUNDING_BOX_2D:
		return boundingBox2DComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::TOGGLE:
		return toggleComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::TEXT:
		return textComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::SELECTABLE:
		return selectableComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::SLIDER:
		return sliderComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::SKYBOX:
		return skyboxComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::ANIMATION:
		return animationComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::SCRIPT:
		return scriptComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::PARTICLE:
		return particleComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::TRAIL:
		return trailComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::BILLBOARD:
		return billboardComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::AUDIO_SOURCE:
		return audioSourceComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::AUDIO_LISTENER:
		return audioListenerComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::PROGRESS_BAR:
		return progressbarsComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::SPHERE_COLLIDER:
		return sphereColliderComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::BOX_COLLIDER:
		return boxColliderComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::CAPSULE_COLLIDER:
		return capsuleColliderComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::AGENT:
		return agentComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::OBSTACLE:
		return obstacleComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::FOG:
		return fogComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	case ComponentType::VIDEO:
		return videoComponents.Obtain(componentId, owner, componentId, owner->IsActive());
	default:
		LOG("Component of type %i hasn't been registered in Scene::CreateComponentByTypeAndId.", (unsigned) type);
		assert(false);
		return nullptr;
	}
}

void Scene::RemoveComponentByTypeAndId(ComponentType type, UID componentId) {
	switch (type) {
	case ComponentType::TRANSFORM:
		transformComponents.Release(componentId);
		break;
	case ComponentType::MESH_RENDERER:
		meshRendererComponents.Release(componentId);
		break;
	case ComponentType::BOUNDING_BOX:
		boundingBoxComponents.Release(componentId);
		break;
	case ComponentType::CAMERA:
		cameraComponents.Release(componentId);
		break;
	case ComponentType::LIGHT:
		lightComponents.Release(componentId);
		break;
	case ComponentType::CANVAS:
		canvasComponents.Release(componentId);
		break;
	case ComponentType::CANVASRENDERER:
		canvasRendererComponents.Release(componentId);
		break;
	case ComponentType::IMAGE:
		imageComponents.Release(componentId);
		break;
	case ComponentType::TRANSFORM2D:
		transform2DComponents.Release(componentId);
		break;
	case ComponentType::BUTTON:
		buttonComponents.Release(componentId);
		break;
	case ComponentType::EVENT_SYSTEM:
		eventSystemComponents.Release(componentId);
		break;
	case ComponentType::BOUNDING_BOX_2D:
		boundingBox2DComponents.Release(componentId);
		break;
	case ComponentType::TOGGLE:
		toggleComponents.Release(componentId);
		break;
	case ComponentType::TEXT:
		textComponents.Release(componentId);
		break;
	case ComponentType::SELECTABLE:
		selectableComponents.Release(componentId);
		break;
	case ComponentType::SLIDER:
		sliderComponents.Release(componentId);
		break;
	case ComponentType::SKYBOX:
		skyboxComponents.Release(componentId);
		break;
	case ComponentType::ANIMATION:
		animationComponents.Release(componentId);
		break;
	case ComponentType::SCRIPT:
		scriptComponents.Release(componentId);
		break;
	case ComponentType::PARTICLE:
		particleComponents.Release(componentId);
		break;
	case ComponentType::TRAIL:
		trailComponents.Release(componentId);
		break;
	case ComponentType::BILLBOARD:
		billboardComponents.Release(componentId);
		break;
	case ComponentType::AUDIO_SOURCE:
		audioSourceComponents.Release(componentId);
		break;
	case ComponentType::AUDIO_LISTENER:
		audioListenerComponents.Release(componentId);
		break;
	case ComponentType::PROGRESS_BAR:
		progressbarsComponents.Release(componentId);
		break;
	case ComponentType::SPHERE_COLLIDER:
		if (App->time->IsGameRunning()) App->physics->RemoveSphereRigidbody(sphereColliderComponents.Find(componentId));
		sphereColliderComponents.Release(componentId);
		break;
	case ComponentType::BOX_COLLIDER:
		if (App->time->IsGameRunning()) App->physics->RemoveBoxRigidbody(boxColliderComponents.Find(componentId));
		boxColliderComponents.Release(componentId);
		break;
	case ComponentType::CAPSULE_COLLIDER:
		if (App->time->IsGameRunning()) App->physics->RemoveCapsuleRigidbody(capsuleColliderComponents.Find(componentId));
		capsuleColliderComponents.Release(componentId);
		break;
	case ComponentType::AGENT:
		agentComponents.Release(componentId);
		break;
	case ComponentType::OBSTACLE:
		obstacleComponents.Release(componentId);
		break;
	case ComponentType::FOG:
		fogComponents.Release(componentId);
		break;
	case ComponentType::VIDEO:
		videoComponents.Release(componentId);
		break;
	default:
		LOG("Component of type %i hasn't been registered in Scene::RemoveComponentByTypeAndId.", (unsigned) type);
		assert(false);
		break;
	}
}

int Scene::GetTotalTriangles() const {
	int triangles = 0;
	for (const ComponentMeshRenderer& meshComponent : meshRendererComponents) {
		ResourceMesh* mesh = App->resources->GetResource<ResourceMesh>(meshComponent.GetMeshID());
		if (mesh != nullptr) {
			triangles += mesh->indices.size() / 3;
		}
	}
	return triangles;
}

std::vector<float> Scene::GetVertices() {
	std::vector<float> result;

	for (ComponentMeshRenderer& meshRenderer : meshRendererComponents) {
		ResourceMesh* mesh = App->resources->GetResource<ResourceMesh>(meshRenderer.GetMeshID());
		ComponentTransform* transform = meshRenderer.GetOwner().GetComponent<ComponentTransform>();
		if (mesh != nullptr && transform->GetOwner().IsStatic()) {
			for (const ResourceMesh::Vertex& vertex : mesh->vertices) {
				float4 transformedVertex = transform->GetGlobalMatrix() * float4(vertex.position, 1.0f);
				result.push_back(transformedVertex.x);
				result.push_back(transformedVertex.y);
				result.push_back(transformedVertex.z);
			}
		}
	}

	return result;
}

std::vector<int> Scene::GetTriangles() {
	int triangles = 0;
	std::vector<int> maxVertMesh;
	maxVertMesh.push_back(0);
	for (ComponentMeshRenderer& meshRenderer : meshRendererComponents) {
		ResourceMesh* mesh = App->resources->GetResource<ResourceMesh>(meshRenderer.GetMeshID());
		if (mesh != nullptr && meshRenderer.GetOwner().IsStatic()) {
			triangles += mesh->indices.size() / 3;
			maxVertMesh.push_back(mesh->vertices.size());
		}
	}
	std::vector<int> result(triangles * 3);

	int currentGlobalTri = 0;
	int vertOverload = 0;
	int i = 0;

	for (ComponentMeshRenderer& meshRenderer : meshRendererComponents) {
		ResourceMesh* mesh = App->resources->GetResource<ResourceMesh>(meshRenderer.GetMeshID());
		if (mesh != nullptr && meshRenderer.GetOwner().IsStatic()) {
			vertOverload += maxVertMesh[i];
			for (unsigned j = 0; j < mesh->indices.size(); j += 3) {
				result[currentGlobalTri] = mesh->indices[j] + vertOverload;
				result[currentGlobalTri + 1] = mesh->indices[j + 1] + vertOverload;
				result[currentGlobalTri + 2] = mesh->indices[j + 2] + vertOverload;
				currentGlobalTri += 3;
			}
			i++;
		}
	}

	return result;
}

std::vector<float> Scene::GetNormals() {
	std::vector<float> result;

	for (ComponentMeshRenderer& meshRenderer : meshRendererComponents) {
		ResourceMesh* mesh = App->resources->GetResource<ResourceMesh>(meshRenderer.GetMeshID());
		ComponentTransform* transform = meshRenderer.GetOwner().GetComponent<ComponentTransform>();
		if (mesh != nullptr && transform->GetOwner().IsStatic()) {
			for (const ResourceMesh::Vertex& vertex : mesh->vertices) {
				float4 transformedVertex = transform->GetGlobalMatrix() * float4(vertex.normal, 1.0f);
				result.push_back(transformedVertex.x);
				result.push_back(transformedVertex.y);
				result.push_back(transformedVertex.z);
			}
		}
	}

	return result;
}

void Scene::SetNavMesh(UID id) {
	if (navMeshId != 0) {
		App->resources->DecreaseReferenceCount(navMeshId);
	}

	navMeshId = id;

	if (id != 0) {
		App->resources->IncreaseReferenceCount(id);
	}
}

NavMesh* Scene::GetNavMesh() {
	ResourceNavMesh* navMeshResource = App->resources->GetResource<ResourceNavMesh>(navMeshId);
	return navMeshResource ? navMeshResource->GetNavMesh() : nullptr;
}

void Scene::SetCursor(UID cursor) {
	if (cursorId != 0) {
		App->resources->DecreaseReferenceCount(cursorId);
	}

	cursorId = cursor;

	if (cursor != 0) {
		App->resources->IncreaseReferenceCount(cursor);
	}
	App->window->SetCursor(cursorId, widthCursor, heightCursor);
#if GAME
	App->window->ActivateCursor(true);
#endif
}

UID Scene::GetCursor() {
	return cursorId;
}

void Scene::SetCursorWidth(int width) {
	widthCursor = width;
}

int Scene::GetCursorWidth() {
	return widthCursor;
}

void Scene::SetCursorHeight(int height) {
	heightCursor = height;
}

int Scene::GetCursorHeight() {
	return heightCursor;
}
