#pragma once

#include "Utils/PoolMap.h"
#include "Utils/Quadtree.h"
#include "Utils/UID.h"
#include "Rendering/FrustumPlanes.h"
#include "Components/ComponentTransform.h"
#include "Components/ComponentMeshRenderer.h"
#include "Components/ComponentBoundingBox.h"
#include "Components/ComponentCamera.h"
#include "Components/ComponentLight.h"
#include "Components/ComponentBillboard.h"
#include "Components/UI/ComponentCanvas.h"
#include "Components/UI/ComponentCanvasRenderer.h"
#include "Components/UI/ComponentImage.h"
#include "Components/UI/ComponentVideo.h"
#include "Components/UI/ComponentTransform2D.h"
#include "Components/UI/ComponentEventSystem.h"
#include "Components/UI/ComponentButton.h"
#include "Components/UI/ComponentToggle.h"
#include "Components/UI/ComponentText.h"
#include "Components/UI/ComponentSlider.h"
#include "Components/UI/ComponentProgressBar.h"
#include "Components/UI/ComponentBoundingBox2D.h"
#include "Components/ComponentSkybox.h"
#include "Components/ComponentTrail.h"
#include "Components/ComponentParticleSystem.h"
#include "Components/ComponentScript.h"
#include "Components/ComponentAnimation.h"
#include "Components/ComponentAudioListener.h"
#include "Components/ComponentAudioSource.h"
#include "Components/Physics/ComponentSphereCollider.h"
#include "Components/Physics/ComponentBoxCollider.h"
#include "Components/Physics/ComponentCapsuleCollider.h"
#include "Components/ComponentAgent.h"
#include "Components/ComponentObstacle.h"
#include "Components/ComponentFog.h"

class GameObject;

class Scene {
public:
	Scene(unsigned numGameObjects);
	~Scene();

	void ClearScene();		// Removes and clears every GameObject from the scene.
	void RebuildQuadtree(); // Recalculates the Quadtree hierarchy with all the GameObjects in the scene.
	void ClearQuadtree();	// Resets the Quadrtee as empty, and removes all GameObjects from it.

	void Init();
	void Start();

	void Load(JsonValue jScene);
	void Save(JsonValue jScene) const;

	// --- GameObject Management --- //
	GameObject* CreateGameObject(GameObject* parent, UID id, const char* name);
	void DestroyGameObject(GameObject* gameObject);
	GameObject* GetGameObject(UID id) const;

	// --- Component Access (other Component-related methods in GameObject.h) --- //
	template<class T> TESSERACT_ENGINE_API T* GetComponent(UID id);

	// --- Component Management (internal, do not use) --- //
	Component* GetComponentByTypeAndId(ComponentType type, UID componentId);
	Component* CreateComponentByTypeAndId(GameObject* owner, ComponentType type, UID componentId);
	void RemoveComponentByTypeAndId(ComponentType type, UID componentId);

	int GetTotalTriangles() const;
	std::vector<float> GetVertices(); // Gets all the vertices from the MeshRenderer Components only if the ResourceMesh is found and the GameObject is Static
	std::vector<int> GetTriangles();  // Gets all the triangles from the MeshRenderer Components only if the ResourceMesh is found and the GameObject is Static
	std::vector<float> GetNormals();

	std::vector<GameObject*> GetCulledMeshes(const FrustumPlanes& planes, const int mask);	// Gets all the game objects inside the given frustum
	std::vector<GameObject*> GetStaticCulledShadowCasters(const FrustumPlanes& planes);		// Gets all the shadow casters game objects inside the given frustum
	std::vector<GameObject*> GetDynamicCulledShadowCasters(const FrustumPlanes& planes);	// Gets all the shadow casters game objects inside the given frustum
	std::vector<GameObject*> GetMainEntitiesCulledShadowCasters(const FrustumPlanes& planes);	// Gets all the shadow casters game objects inside the given frustum


	void RemoveStaticShadowCaster(const GameObject* go);
	void AddStaticShadowCaster(GameObject* go);

	void RemoveDynamicShadowCaster(const GameObject* go);
	void AddDynamicShadowCaster(GameObject* go);

	void RemoveMainEntityShadowCaster(const GameObject* go);
	void AddMainEntityShadowCaster(GameObject* go);

	const std::vector<GameObject*>& GetStaticShadowCasters() const;
	const std::vector<GameObject*>& GetDynamicShadowCasters() const;
	const std::vector<GameObject*>& GetMainEntitiesShadowCasters() const;

	void SetCursor(UID cursor);
	UID GetCursor();

	void SetCursorWidth(int width);
	int GetCursorWidth();
	void SetCursorHeight(int height);
	int GetCursorHeight();


public:
	GameObject* root = nullptr;				// GameObject Root. Parent of everything and god among gods (Game Object Deity) :D.
	GameObject* directionalLight = nullptr; // GameObject of directional light

	PoolMap<UID, GameObject> gameObjects; // Pool of GameObjects. Stores all the memory of all existing GameObject in a contiguous memory space.

	bool sceneLoaded = true; // This is set to true when all scene resources have been loaded

	// ---- Components ---- //
	PoolMap<UID, ComponentTransform> transformComponents;
	PoolMap<UID, ComponentMeshRenderer> meshRendererComponents;
	PoolMap<UID, ComponentBoundingBox> boundingBoxComponents;
	PoolMap<UID, ComponentCamera> cameraComponents;
	PoolMap<UID, ComponentLight> lightComponents;
	PoolMap<UID, ComponentCanvas> canvasComponents;
	PoolMap<UID, ComponentCanvasRenderer> canvasRendererComponents;
	PoolMap<UID, ComponentImage> imageComponents;
	PoolMap<UID, ComponentTransform2D> transform2DComponents;
	PoolMap<UID, ComponentBoundingBox2D> boundingBox2DComponents;
	PoolMap<UID, ComponentEventSystem> eventSystemComponents;
	PoolMap<UID, ComponentToggle> toggleComponents;
	PoolMap<UID, ComponentText> textComponents;
	PoolMap<UID, ComponentButton> buttonComponents;
	PoolMap<UID, ComponentSelectable> selectableComponents;
	PoolMap<UID, ComponentSlider> sliderComponents;
	PoolMap<UID, ComponentSkyBox> skyboxComponents;
	PoolMap<UID, ComponentScript> scriptComponents;
	PoolMap<UID, ComponentAnimation> animationComponents;
	PoolMap<UID, ComponentParticleSystem> particleComponents;
	PoolMap<UID, ComponentTrail> trailComponents;
	PoolMap<UID, ComponentBillboard> billboardComponents;
	PoolMap<UID, ComponentAudioSource> audioSourceComponents;
	PoolMap<UID, ComponentAudioListener> audioListenerComponents;
	PoolMap<UID, ComponentProgressBar> progressbarsComponents;
	PoolMap<UID, ComponentSphereCollider> sphereColliderComponents;
	PoolMap<UID, ComponentBoxCollider> boxColliderComponents;
	PoolMap<UID, ComponentCapsuleCollider> capsuleColliderComponents;
	PoolMap<UID, ComponentAgent> agentComponents;
	PoolMap<UID, ComponentObstacle> obstacleComponents;
	PoolMap<UID, ComponentFog> fogComponents;
	PoolMap<UID, ComponentVideo> videoComponents;

	// ---- Quadtree Parameters ---- //
	Quadtree<GameObject> quadtree;
	AABB2D quadtreeBounds = {{-1000, -1000}, {1000, 1000}};
	unsigned quadtreeMaxDepth = 4;
	unsigned quadtreeElementsPerNode = 200;

	// ---- Game Camera Parameters ---- //
	UID gameCameraId = 0;

	// ---- Ambient Light Parameters ---- //
	float3 ambientColor = {0.25f, 0.25f, 0.25f};

	// ---- Nav Mesh ID parameters ---- //
	UID navMeshId = 0;

	// ---- Cursor parameters ---- //
	UID cursorId = 0;
	int widthCursor = 30;
	int heightCursor = 30;

private:
	bool InsideFrustumPlanes(const FrustumPlanes& planes, const GameObject* go);

private:
	std::vector<GameObject*> staticShadowCasters;
	std::vector<GameObject*> dynamicShadowCasters;
	std::vector<GameObject*> mainEntitiesShadowCasters;
};

template<class T>
inline T* Scene::GetComponent(UID id) {
	return (T*) GetComponentByTypeAndId(T::staticType, id);
}
