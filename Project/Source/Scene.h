#pragma once

#include "Utils/PoolMap.h"
#include "Utils/Quadtree.h"
#include "Utils/UID.h"
#include "Components/ComponentTransform.h"
#include "Components/ComponentMeshRenderer.h"
#include "Components/ComponentBoundingBox.h"
#include "Components/ComponentCamera.h"
#include "Components/ComponentLight.h"
#include "Components/UI/ComponentCanvas.h"
#include "Components/UI/ComponentCanvasRenderer.h"
#include "Components/UI/ComponentImage.h"
#include "Components/UI/ComponentTransform2D.h"
#include "Components/UI/ComponentEventSystem.h"
#include "Components/UI/ComponentButton.h"
#include "Components/UI/ComponentToggle.h"
#include "Components/UI/ComponentText.h"
#include "Components/UI/ComponentSlider.h"
#include "Components/ComponentBoundingBox2D.h"
#include "Components/ComponentSkybox.h"
#include "Components/ComponentParticleSystem.h"
#include "Components/ComponentScript.h"
#include "Components/ComponentAnimation.h"
#include "Components/ComponentAudioListener.h"
#include "Components/ComponentAudioSource.h"

class GameObject;

class Scene {
public:
	Scene(unsigned numGameObjects);

	void ClearScene();		// Removes and clears every GameObject from the scene.
	void RebuildQuadtree(); // Recalculates the Quadtree hierarchy with all the GameObjects in the scene.
	void ClearQuadtree();	// Resets the Quadrtee as empty, and removes all GameObjects from it.

	// --- GameObject Management --- //
	GameObject* CreateGameObject(GameObject* parent, UID id, const char* name);
	GameObject* DuplicateGameObject(GameObject* gameObject, GameObject* parent);
	void DestroyGameObject(GameObject* gameObject);
	GameObject* GetGameObject(UID id) const;

	// --- Component Access (other Component-related methods in GameObject.h) --- //
	template<class T> TESSERACT_ENGINE_API T* GetComponent(UID id);

	// --- Component Management (internal, do not use) --- //
	Component* GetComponentByTypeAndId(ComponentType type, UID componentId);
	Component* CreateComponentByTypeAndId(GameObject* owner, ComponentType type, UID componentId);
	void RemoveComponentByTypeAndId(ComponentType type, UID componentId);

	int GetTotalTriangles() const;

public:
	GameObject* root = nullptr;			  // GameObject Root. Parent of everything and god among gods (Game Object Deity) :D.
	PoolMap<UID, GameObject> gameObjects; // Pool of GameObjects. Stores all the memory of all existing GameObject in a contiguous memory space.

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
	PoolMap<UID, ComponentAudioSource> audioSourceComponents;
	PoolMap<UID, ComponentAudioListener> audioListenerComponents;

	// ---- Quadtree Parameters ---- //
	Quadtree<GameObject> quadtree;
	AABB2D quadtreeBounds = {{-1000, -1000}, {1000, 1000}};
	unsigned quadtreeMaxDepth = 4;
	unsigned quadtreeElementsPerNode = 200;
};

template<class T>
inline T* Scene::GetComponent(UID id) {
	return (T*) GetComponentByTypeAndId(T::staticType, id);
}