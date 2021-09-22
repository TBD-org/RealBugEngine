#pragma once

#include "Resources/ResourceType.h"
#include "Utils/UID.h"

#include "Math/float2.h"
#include <string>
#include <variant>

class GameObject;
class Component;
class Resource;
class Scene;

struct AssetCache;

#define EventVariant std::variant< \
	int,                           \
	DestroyGameObjectStruct,       \
	CreateResourceStruct,          \
	DestroyResourceStruct,         \
	UpdateAssetCacheStruct,        \
	ChangeSceneStruct,             \
	LoadSceneStruct,               \
	SaveSceneStruct,               \
	ViewportResizedStruct>

/* Creating a new event type:
*    1. Add a new EventType for the new event (ALWAYS ABOVE COUNT)
*    2. Add a struct containing the necessary information for said event inside the union below (Make sure it has a constructor)
*    3. (If allocating) Make sure you release all allocated resources in ModuleEvents.cpp's CleanUpEvent()
*	 4. Remember to make an std::emplace whenever generating a TesseractEvent, there are no default values so trying to std::get a non initiated variant will return a crash
*/

enum class TesseractEventType {
	UNKNOWN = 0,
	GAMEOBJECT_DESTROYED,
	PRESSED_PLAY,
	PRESSED_PAUSE,
	PRESSED_RESUME,
	PRESSED_STEP,
	PRESSED_STOP,
	CREATE_RESOURCE,
	DESTROY_RESOURCE,
	UPDATE_ASSET_CACHE,
	MOUSE_CLICKED,
	MOUSE_RELEASED,
	CHANGE_SCENE,
	LOAD_SCENE,
	SAVE_SCENE,
	COMPILATION_FINISHED,
	SCREEN_RESIZED,
	COUNT
};

struct CreateResourceStruct {
	ResourceType type = ResourceType::UNKNOWN;
	UID resourceId = 0;
	std::string resourceName = "";
	std::string assetFilePath = "";
	CreateResourceStruct(ResourceType type_, UID resourceId_, const char* resourceName_, const char* assetFilePath_)
		: type(type_)
		, resourceId(resourceId_)
		, resourceName(resourceName_)
		, assetFilePath(assetFilePath_) {}
};

struct DestroyResourceStruct {
	UID resourceId = 0;
	DestroyResourceStruct(UID resourceId_)
		: resourceId(resourceId_) {}
};

struct DestroyGameObjectStruct {
	Scene* scene = nullptr;
	GameObject* gameObject = nullptr;
	DestroyGameObjectStruct(Scene* scene_, GameObject* gameObject_)
		: scene(scene_), gameObject(gameObject_) {}
};

struct UpdateAssetCacheStruct {
	AssetCache* assetCache = nullptr;
	UpdateAssetCacheStruct(AssetCache* assetCache_)
		: assetCache(assetCache_) {
	}
};

struct ChangeSceneStruct {
	UID sceneId = 0;
	ChangeSceneStruct(UID sceneId_)
		: sceneId(sceneId_) {}
};

struct LoadSceneStruct {
	std::string filePath = "";
	LoadSceneStruct(const char* filePath_)
		: filePath(filePath_) {}
};

struct SaveSceneStruct {
	std::string filePath = "";
	SaveSceneStruct(const char* filePath_)
		: filePath(filePath_) {}
};

struct ViewportResizedStruct {
	ViewportResizedStruct(int newWidth_, int newHeight_)
		: newWidth(newWidth_)
		, newHeight(newHeight_) {}
	int newWidth = 0;
	int newHeight = 0;
};

struct TesseractEvent {
public:
	TesseractEvent(TesseractEventType type);

public:
	TesseractEventType type = TesseractEventType::UNKNOWN;

	template<typename T>
	T& Get();

	template<typename T, typename... Args>
	void Set(Args... args);

private:
	EventVariant variant;
};

template<typename T>
inline T& TesseractEvent::Get() {
	return std::get<T>(variant);
}

template<typename T, typename... Args>
inline void TesseractEvent::Set(Args... args) {
	variant.emplace<T>(std::forward<Args>(args)...);
}