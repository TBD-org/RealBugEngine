#pragma once

/* Creating a new resource type:
*    1. Add a new ResourceType for the new resource
*    2. Add REGISTER_RESOURCE to the .h of the new resource
*    3. Add the new resource to the GetResourceTypeName and GetResourceTypeFromName functions in ResourceType.cpp
*    4. Add the new resource to the ModuleResource::CreateResourceByType function in ModuleResources.cpp
*/

// REGISTER_RESOURCE builds the data structures common to all Resources.
// This includes the Constructor and the Destructor.
#define REGISTER_RESOURCE(resourceClass, resourceType)   \
	static const ResourceType staticType = resourceType; \
	resourceClass(UID id, const char* name, const char* assetFilePath, const char* resourceFilePath) : Resource(staticType, id, name, assetFilePath, resourceFilePath) {}

enum class ResourceType {
	UNKNOWN,
	MATERIAL,
	MESH,
	PREFAB,
	SCENE,
	SHADER,
	TEXTURE,
	FONT,
	SKYBOX,
	SCRIPT,
	ANIMATION,
	NAVMESH,
	STATE_MACHINE,
	CLIP,
	AUDIO,
	VIDEO
};

const char* GetResourceTypeName(ResourceType type);
ResourceType GetResourceTypeFromName(const char* name);