#pragma once

#include "Application.h"
#include "Modules/ModuleResources.h"
#include "Resources/ResourceType.h"
#include "Utils/JsonValue.h"
#include "Utils/UID.h"

#include <memory>

class Resource;

#define JSON_TAG_RESOURCES "Resources"
#define JSON_TAG_TYPE "Type"
#define JSON_TAG_ID "Id"

namespace ImporterCommon {
	template<typename T> std::unique_ptr<T> CreateResource(const char* resourceName, const char* assetFilePath, JsonValue jMeta, unsigned& resourceIndex);
	bool SaveResourceMetaFile(Resource* resource);
}; // namespace ImporterCommon

template<typename T>
inline std::unique_ptr<T> ImporterCommon::CreateResource(const char* resourceName, const char* assetFilePath, JsonValue jMeta, unsigned& resourceIndex) {
	// Create resource
	JsonValue jResources = jMeta[JSON_TAG_RESOURCES];
	JsonValue jResource = jResources[resourceIndex];
	UID metaId = jResource[JSON_TAG_ID];
	std::unique_ptr<T> resource = App->resources->CreateResource<T>(resourceName, assetFilePath, metaId ? metaId : GenerateUID());

	// Increment resource index
	resourceIndex++;

	// Add resource to meta file
	jResource[JSON_TAG_TYPE] = GetResourceTypeName(resource->GetType());
	jResource[JSON_TAG_ID] = resource->GetId();

	return resource;
}