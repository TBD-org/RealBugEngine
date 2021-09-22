#include "ModuleResources.h"

#include "Globals.h"
#include "Application.h"
#include "Utils/Logging.h"
#include "Utils/PathUtils.h"
#include "Utils/FileDialog.h"
#include "Utils/JsonValue.h"
#include "Resources/ResourcePrefab.h"
#include "Resources/ResourceMaterial.h"
#include "Resources/ResourceMesh.h"
#include "Resources/ResourceScene.h"
#include "Resources/ResourceShader.h"
#include "Resources/ResourceTexture.h"
#include "Resources/ResourceFont.h"
#include "Resources/ResourceSkybox.h"
#include "Resources/ResourceScript.h"
#include "Resources/ResourceAnimation.h"
#include "Resources/ResourceStateMachine.h"
#include "Resources/ResourceClip.h"
#include "Resources/ResourceAudioClip.h"
#include "Resources/ResourceVideo.h"
#include "Resources/ResourceNavMesh.h"
#include "Importers/SceneImporter.h"
#include "Importers/ModelImporter.h"
#include "Importers/PrefabImporter.h"
#include "Importers/TextureImporter.h"
#include "Importers/MaterialImporter.h"
#include "Importers/SkyboxImporter.h"
#include "Importers/ShaderImporter.h"
#include "Importers/AudioImporter.h"
#include "Importers/VideoImporter.h"
#include "Importers/StateMachineImporter.h"
#include "Importers/ClipImporter.h"
#include "Importers/FontImporter.h"
#include "Importers/ScriptImporter.h"
#include "Importers/NavMeshImporter.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleInput.h"
#include "Modules/ModuleEvents.h"
#include "TesseractEvent.h"

#include "IL/il.h"
#include "IL/ilu.h"
#include "GL/glew.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"
#include <string>
#include <future>
#include <chrono>
#include "Brofiler.h"

#include "Utils/Leaks.h"

#define JSON_TAG_RESOURCES "Resources"
#define JSON_TAG_TYPE "Type"
#define JSON_TAG_ID "Id"
#define JSON_TAG_NAME "Name"

static bool ReadJSON(const char* filePath, rapidjson::Document& document) {
	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error reading meta file %s", filePath);
		return false;
	}

	// Parse document from file
	document.Parse<rapidjson::kParseNanAndInfFlag>(buffer.Data());
	if (document.HasParseError()) {
		LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document.GetParseError()), document.GetErrorOffset());
		return false;
	}

	return true;
}

static void SaveJSON(const char* filePath, rapidjson::Document& document) {
	// Write document to buffer
	rapidjson::StringBuffer stringBuffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator, rapidjson::kWriteNanAndInfFlag> writer(stringBuffer);
	document.Accept(writer);

	// Save to file
	App->files->Save(filePath, stringBuffer.GetString(), stringBuffer.GetSize());
}

bool ModuleResources::Init() {
	ilInit();
	iluInit();
	ilEnable(IL_KEEP_DXTC_DATA);
	ilEnable(IL_ORIGIN_SET);
	ilSetInteger(IL_ORIGIN_MODE, IL_ORIGIN_UPPER_LEFT);

	App->events->AddObserverToEvent(TesseractEventType::CREATE_RESOURCE, this);
	App->events->AddObserverToEvent(TesseractEventType::DESTROY_RESOURCE, this);
	App->events->AddObserverToEvent(TesseractEventType::UPDATE_ASSET_CACHE, this);

#if GAME
	ImportLibrary();
#endif

	return true;
}

bool ModuleResources::Start() {
	stopImportThread = false;
	importThread = std::thread(&ModuleResources::UpdateImportAsync, this);

	stopLoadingThread = false;
	loadingThread = std::thread(&ModuleResources::UpdateLoadingAsync, this);

	return true;
}

UpdateStatus ModuleResources::Update() {
	BROFILER_CATEGORY("ModuleResources - Update", Profiler::Color::Orange)

	// Copy dropped file to assets folder
	const char* droppedFilePath = App->input->GetDroppedFilePath();
	if (droppedFilePath != nullptr) {
		std::string newFilePath = std::string(ASSETS_PATH) + "/" + PathUtils::GetFileNameAndExtension(droppedFilePath);
		FileDialog::Copy(droppedFilePath, newFilePath.c_str());
		App->input->ReleaseDroppedFilePath();
	}

	// Finish resource loading
	MSTimer timer;
	timer.Start();
	while (!resourcesToFinishLoading.empty() && timer.Read() < MAX_RESOURCE_LOADING_DURATION_PER_FRAME_MS) {
		Resource* resource = nullptr;
		if (resourcesToFinishLoading.try_pop(resource)) {
			if (resource != nullptr) {
				resource->FinishLoading();
				resource->SetLoaded(true);
				numResourcesLoading -= 1;
			}
		}
	}

	return UpdateStatus::CONTINUE;
}

bool ModuleResources::CleanUp() {
	stopLoadingThread = true;
	loadingThread.join();

	stopImportThread = true;
	importThread.join();

	for (auto& entry : resources) {
		entry.second->Unload();
	}
	resources.clear();

	return true;
}

void ModuleResources::ReceiveEvent(TesseractEvent& e) {
	if (e.type == TesseractEventType::CREATE_RESOURCE) {
		CreateResourceStruct& createResourceStruct = e.Get<CreateResourceStruct>();
		Resource* resource = CreateResourceByType(createResourceStruct.type, createResourceStruct.resourceName.c_str(), createResourceStruct.assetFilePath.c_str(), createResourceStruct.resourceId);
		UID id = resource->GetId();
		resourcesMutex.lock();
		resources[id].reset(resource);
		resourcesMutex.unlock();

		if (GetReferenceCount(id) > 0) {
			LoadResource(resource);
		}

	} else if (e.type == TesseractEventType::DESTROY_RESOURCE) {
		UID id = e.Get<DestroyResourceStruct>().resourceId;
		resourcesMutex.lock();
		auto& it = resources.find(id);
		if (it != resources.end()) {
			it->second->Unload();
			resources.erase(it);
		}
		resourcesMutex.unlock();
	} else if (e.type == TesseractEventType::UPDATE_ASSET_CACHE) {
		AssetCache* newAssetCache = e.Get<UpdateAssetCacheStruct>().assetCache;
		assetCache.reset(newAssetCache);

		e.Get<UpdateAssetCacheStruct>().assetCache = nullptr;
	}
}

void ModuleResources::ValidateAssetResources(JsonValue jMeta, bool& validResourceFiles) {
	JsonValue jResources = jMeta[JSON_TAG_RESOURCES];
	for (unsigned i = 0; i < jResources.Size(); ++i) {
		UID id = jResources[i][JSON_TAG_ID];
		std::string resourcePath = App->resources->GenerateResourcePath(id);
		if (!App->files->Exists(resourcePath.c_str())) {
			validResourceFiles = false;
			break;
		}
	}
}

void ModuleResources::RecreateResources(JsonValue jMeta, const char* filePath) {
	JsonValue jResources = jMeta[JSON_TAG_RESOURCES];
	for (unsigned i = 0; i < jResources.Size(); ++i) {
		JsonValue jResource = jResources[i];
		UID id = jResource[JSON_TAG_ID];
		if (GetResourceInternal<Resource>(id) == nullptr) {
			std::string typeName = jResource[JSON_TAG_TYPE];
			ResourceType type = GetResourceTypeFromName(typeName.c_str());
			SendCreateResourceEventByType(type, "", filePath, id);
		}
	}
}

bool ModuleResources::ImportAssetByExtension(JsonValue jMeta, const char* filePath) {
	bool validExtension = true;
	std::string extension = PathUtils::GetFileExtension(filePath);

	Resource* resource = nullptr;
	if (extension == SCENE_EXTENSION) {
		// Scene files
		SceneImporter::ImportScene(filePath, jMeta);
	} else if (extension == MATERIAL_EXTENSION) {
		// Material files
		MaterialImporter::ImportMaterial(filePath, jMeta);
	} else if (extension == FRAGMENT_SHADER_EXTENSION || extension == VERTEX_SHADER_EXTENSION || extension == DEFAULT_SHADER_EXTENSION) {
		// Shader files
		ShaderImporter::ImportShader(filePath, jMeta);
	} else if (extension == MODEL_EXTENSION) {
		// Model files
		ModelImporter::ImportModel(filePath, jMeta);
	} else if (extension == SKYBOX_EXTENSION) {
		// Skybox files
		SkyboxImporter::ImportSkybox(filePath, jMeta);
	} else if (extension == JPG_TEXTURE_EXTENSION || extension == PNG_TEXTURE_EXTENSION || extension == TIF_TEXTURE_EXTENSION || extension == DDS_TEXTURE_EXTENSION || extension == TGA_TEXTURE_EXTENSION) {
		// Texture files
		TextureImporter::ImportTexture(filePath, jMeta);
	} else if (extension == FONT_EXTENSION) {
		// Font files
		FontImporter::ImportFont(filePath, jMeta);
	} else if (extension == SCRIPT_EXTENSION) {
		// Script files
		ScriptImporter::ImportScript(filePath, jMeta);
	} else if (extension == WAV_AUDIO_EXTENSION || extension == OGG_AUDIO_EXTENSION) {
		// Audio files
		AudioImporter::ImportAudio(filePath, jMeta);
	} else if (extension == MP4_VIDEO_EXTENSION) {
		// Video files
		VideoImporter::ImportVideo(filePath, jMeta);
	} else if (extension == PREFAB_EXTENSION) {
		// Prefab files
		PrefabImporter::ImportPrefab(filePath, jMeta);
	} else if (extension == STATE_MACHINE_EXTENSION) {
		// State Machine files
		StateMachineImporter::ImportStateMachine(filePath, jMeta);
	} else if (extension == ANIMATION_CLIP_EXTENSION) {
		// Clip files
		ClipImporter::ImportClip(filePath, jMeta);
	} else if (extension == NAVMESH_EXTENSION) {
		// NavMesh files
		NavMeshImporter::ImportNavMesh(filePath, jMeta);
	} else {
		validExtension = false;
	}
	return validExtension;
}

std::list<UID> ModuleResources::ImportAssetResources(const char* filePath, bool force) {
	std::list<UID> resources;

	// Return an empty list if the asset couldn't be found
	if (!App->files->Exists(filePath)) return resources;

	std::string metaFilePath = std::string(filePath) + META_EXTENSION;

	// Flag to keep track of the validity of the asset's meta file
	bool validMetaFile = App->files->Exists(metaFilePath.c_str());

	// Flag to check if the asset resources are created/imported successfully
	bool validResourceFiles = true;

	rapidjson::Document document;
	JsonValue jMeta(document, document);

	// Check meta file resources
	if (validMetaFile) {
		validMetaFile = ReadJSON(metaFilePath.c_str(), document);
		if (validMetaFile) {
			ValidateAssetResources(jMeta, validResourceFiles);
		}
	}

	// if resources are valid resources, reimport them or import them if needed or forced to
	if (!force && validMetaFile && validResourceFiles) {
		RecreateResources(jMeta, filePath);
	} else {
		if (ImportAssetByExtension(jMeta, filePath)) {
			if (!validMetaFile) {
				SaveJSON(metaFilePath.c_str(), document);
			}
		}
	}

	// Fill the temporal resource vector with the imported resources
	JsonValue jResources = jMeta[JSON_TAG_RESOURCES];
	for (unsigned i = 0; i < jResources.Size(); ++i) {
		resources.push_back(jResources[i][JSON_TAG_ID]);
	}

	return resources;
}

ResourceType ModuleResources::GetResourceType(UID id) {
	Resource* resource = GetResourceInternal<Resource>(id);
	if (resource == nullptr) return ResourceType::UNKNOWN;
	return resource->GetType();
}

const char* ModuleResources::GetResourceName(UID id) {
	Resource* resource = GetResourceInternal<Resource>(id);
	if (resource == nullptr) return nullptr;
	return resource->GetName().c_str();
}

AssetCache* ModuleResources::GetAssetCache() const {
	return assetCache.get();
}

void ModuleResources::IncreaseReferenceCount(UID id) {
	if (id == 0) return;

	if (referenceCounts.find(id) != referenceCounts.end()) {
		referenceCounts[id] = referenceCounts[id] + 1;
	} else {
		referenceCounts[id] = 1;
		Resource* resource = GetResourceInternal<Resource>(id);
		if (resource != nullptr) {
			LoadResource(resource);
		}
	}
}

void ModuleResources::DecreaseReferenceCount(UID id) {
	if (id == 0) return;

	if (referenceCounts.find(id) != referenceCounts.end()) {
		referenceCounts[id] = referenceCounts[id] - 1;
		if (referenceCounts[id] <= 0) {
			referenceCounts.erase(id);
			Resource* resource = GetResourceInternal<Resource>(id);
			if (resource != nullptr) {
				resource->Unload();
				resource->SetLoaded(false);
			}
		}
	}
}

void ModuleResources::ResetReferenceCount(UID id) {
	if (id == 0) return;

	if (referenceCounts.find(id) != referenceCounts.end()) {
		referenceCounts.erase(id);
		Resource* resource = GetResourceInternal<Resource>(id);
		if (resource != nullptr) {
			resource->SetLoaded(false);
			resource->Unload();
		}
	}
}

unsigned ModuleResources::GetReferenceCount(UID id) const {
	auto it = referenceCounts.find(id);
	return it != referenceCounts.end() ? it->second : 0;
}

bool ModuleResources::HaveResourcesFinishedLoading() {
	return numResourcesLoading == 0;
}

std::string ModuleResources::GenerateResourcePath(UID id) const {
	std::string strId = std::to_string(id);
	std::string metaFolder = std::string(LIBRARY_PATH "/") + strId.substr(0, 2);

	if (!App->files->Exists(metaFolder.c_str())) {
		App->files->CreateFolder(metaFolder.c_str());
	}

	return metaFolder + "/" + strId;
}

void ModuleResources::UpdateImportAsync() {
	while (!stopImportThread) {
#if !GAME
		// Check if any asset file has been modified / deleted
		std::vector<UID> resourcesToRemove;
		std::vector<std::string> assetsToImport;

		for (const auto& entry : concurrentResourceUIDToAssetFilePath) {
			UID resourceId = entry.first;
			const std::string& assetFilePath = entry.second;
			const std::string& resourceFilePath = GenerateResourcePath(resourceId);
			std::string metaFilePath = assetFilePath + META_EXTENSION;

			// Check for deleted assets
			if (!App->files->Exists(assetFilePath.c_str())) {
				resourcesToRemove.push_back(resourceId);
				continue;
			}

			// Check for deleted, invalid or outdated meta files
			if (!App->files->Exists(metaFilePath.c_str())) {
				resourcesToRemove.push_back(resourceId);
				continue;
			} else {
				rapidjson::Document document;
				bool success = ReadJSON(metaFilePath.c_str(), document);
				JsonValue jMeta(document, document);

				if (success) {
					long long metaTimestamp = App->files->GetLocalFileModificationTime(metaFilePath.c_str());
					long long assetTimestamp = App->files->GetLocalFileModificationTime(assetFilePath.c_str());
					if (assetTimestamp > metaTimestamp) {
						if (jMeta[JSON_TAG_RESOURCES].Size() > 1) {
							resourcesToRemove.push_back(resourceId);
						} else {
							resourcesToRemove.push_back(resourceId);
							if (std::find(assetsToImport.begin(), assetsToImport.end(), assetFilePath) == assetsToImport.end()) {
								assetsToImport.push_back(assetFilePath);
								SaveJSON(metaFilePath.c_str(), document);
							}
						}
						continue;
					}
				} else {
					resourcesToRemove.push_back(resourceId);
					continue;
				}
			}
			// Check for deleted resources
			if (!App->files->Exists(resourceFilePath.c_str())) {
				if (std::find(assetsToImport.begin(), assetsToImport.end(), assetFilePath) == assetsToImport.end()) {
					assetsToImport.push_back(assetFilePath);
				}
			}
		}
		for (UID resourceId : resourcesToRemove) {
			const std::string& assetFilePath = concurrentResourceUIDToAssetFilePath.at(resourceId);
			const std::string& resourceFilePath = GenerateResourcePath(resourceId);
			std::string metaFilePath = assetFilePath + META_EXTENSION;
			if (App->files->Exists(metaFilePath.c_str()) && std::find(assetsToImport.begin(), assetsToImport.end(), assetFilePath) == assetsToImport.end()) {
				App->files->Erase(metaFilePath.c_str());
			}
			if (App->files->Exists(resourceFilePath.c_str())) {
				App->files->Erase(resourceFilePath.c_str());
			}

			DestroyResource(resourceId);
		}
		for (const std::string& assetFilePath : assetsToImport) {
			ImportAssetResources(assetFilePath.c_str());
		}
		while (!assetsToReimport.empty()) {
			std::string assetFilePath = "";
			assetsToReimport.try_pop(assetFilePath);
			ImportAssetResources(assetFilePath.c_str(), true);
		}

		// Check if there are any new assets and build cached folder structure
		AssetCache* newAssetCache = new AssetCache(ASSETS_PATH);
		CheckForNewAssetsRecursive(ASSETS_PATH, *newAssetCache, newAssetCache->root);

		TesseractEvent updateAssetCacheEv(TesseractEventType::UPDATE_ASSET_CACHE);
		updateAssetCacheEv.Set<UpdateAssetCacheStruct>(newAssetCache);
		App->events->AddEvent(updateAssetCacheEv);
#endif

		std::this_thread::sleep_for(std::chrono::milliseconds(TIME_BETWEEN_RESOURCE_IMPORT_UPDATES_MS));
	}
}

void ModuleResources::UpdateLoadingAsync() {
	while (!stopLoadingThread) {
		// Load resources
		if (!resourcesToLoad.empty()) {
			Resource* resource = nullptr;
			if (!resourcesToLoad.try_pop(resource)) break;
			if (resource == nullptr) continue;

			resource->Load();
			resourcesToFinishLoading.push(resource);
		}
	}
}

void ModuleResources::CheckForNewAssetsRecursive(const char* path, AssetCache& assetCache, AssetFolder& parentFolder) {
	for (std::string& file : App->files->GetFilesInFolder(path)) {
		std::string filePath = std::string(path) + "/" + file;
		std::string extension = PathUtils::GetFileExtension(file.c_str());
		if (App->files->IsDirectory(filePath.c_str())) {
			parentFolder.folders.push_back(AssetFolder(filePath.c_str()));
			assetCache.foldersMap[filePath] = &parentFolder.folders.back();
			CheckForNewAssetsRecursive(filePath.c_str(), assetCache, parentFolder.folders.back());
		} else if (extension != META_EXTENSION) {
			std::list<UID>& resourceIds = ImportAssetResources(filePath.c_str());
			if (!resourceIds.empty()) {
				AssetFile assetFile(filePath.c_str());
				assetFile.resourceIds = std::move(resourceIds);
				parentFolder.files.push_back(std::move(assetFile));
				assetCache.filesMap[filePath] = &parentFolder.files.back();
			}
		}
	}
}

void ModuleResources::ImportLibrary() {
	for (std::string& libraryFile : App->files->GetFilesInFolder(LIBRARY_PATH)) {
		std::string libraryFilePath = std::string(LIBRARY_PATH "/") + libraryFile;
		if (App->files->IsDirectory(libraryFilePath.c_str())) {
			for (std::string file : App->files->GetFilesInFolder(libraryFilePath.c_str())) {
				std::string filePath = libraryFilePath + "/" + file;
				std::string extension = PathUtils::GetFileExtension(file.c_str());
				if (extension != META_EXTENSION) {
					ImportLibraryResource(filePath.c_str());
				}
			}
		}
	}
}

void ModuleResources::ImportLibraryResource(const char* filePath) {
	std::string resourceMetaFilePath = std::string(filePath) + META_EXTENSION;
	rapidjson::Document document;
	bool validResourceMeta = ReadJSON(resourceMetaFilePath.c_str(), document);
	if (!validResourceMeta) {
		LOG("Invalid resource meta for resource '%s'", filePath);
		return;
	}

	JsonValue jResourceMeta(document, document);
	std::string resourceTypeName = jResourceMeta[JSON_TAG_TYPE];
	std::string resourceName = jResourceMeta[JSON_TAG_NAME];

	ResourceType type = GetResourceTypeFromName(resourceTypeName.c_str());
	std::string fileName = PathUtils::GetFileName(filePath);
	UID id = SDL_strtoull(fileName.c_str(), nullptr, 10);

	Resource* resource = CreateResourceByType(type, resourceName.c_str(), "", id);
	resourcesMutex.lock();
	resources[id].reset(resource);
	resourcesMutex.unlock();

	if (GetReferenceCount(id) > 0) {
		LoadResource(resource);
	}
}

void ModuleResources::SendCreateResourceEventByType(ResourceType type, const char* resourceName, const char* assetFilePath, UID id) {
	concurrentResourceUIDToAssetFilePath[id] = assetFilePath;

	TesseractEvent addResourceEvent(TesseractEventType::CREATE_RESOURCE);
	addResourceEvent.Set<CreateResourceStruct>(type, id, resourceName, assetFilePath);
	App->events->AddEvent(addResourceEvent);
}

Resource* ModuleResources::CreateResourceByType(ResourceType type, const char* resourceName, const char* assetFilePath, UID id) {
	std::string resourceFilePath = GenerateResourcePath(id);
	Resource* resource = nullptr;
	switch (type) {
	case ResourceType::MATERIAL:
		resource = new ResourceMaterial(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::MESH:
		resource = new ResourceMesh(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::PREFAB:
		resource = new ResourcePrefab(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::SCENE:
		resource = new ResourceScene(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::SHADER:
		resource = new ResourceShader(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::TEXTURE:
		resource = new ResourceTexture(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::FONT:
		resource = new ResourceFont(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::SKYBOX:
		resource = new ResourceSkybox(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::SCRIPT:
		resource = new ResourceScript(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::ANIMATION:
		resource = new ResourceAnimation(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::STATE_MACHINE:
		resource = new ResourceStateMachine(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::CLIP:
		resource = new ResourceClip(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::AUDIO:
		resource = new ResourceAudioClip(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::VIDEO:
		resource = new ResourceVideo(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	case ResourceType::NAVMESH:
		resource = new ResourceNavMesh(id, resourceName, assetFilePath, resourceFilePath.c_str());
		break;
	default:
		LOG("Resource of type %i hasn't been registered in ModuleResources::CreateResourceByType.", (unsigned) type);
		assert(false); // ERROR: Resource type not registered
		return nullptr;
	}
	return resource;
}

void ModuleResources::DestroyResource(UID id) {
	concurrentResourceUIDToAssetFilePath.erase(id);

	TesseractEvent destroyResourceEvent(TesseractEventType::DESTROY_RESOURCE);
	destroyResourceEvent.Set<DestroyResourceStruct>(id);
	App->events->AddEvent(destroyResourceEvent);
}

void ModuleResources::LoadResource(Resource* resource) {
	numResourcesLoading += 1;

	// Read resource meta file
	bool resourceMetaLoaded = true;
	std::string resourceMetaFile = resource->GetResourceFilePath() + META_EXTENSION;
	Buffer<char> buffer = App->files->Load(resourceMetaFile.c_str());
	if (buffer.Size() == 0) {
		LOG("Error loading meta file path %s", resourceMetaFile);
		resourceMetaLoaded = false;
	}

	// Parse document from file
	rapidjson::Document document;
	if (resourceMetaLoaded) {
		document.ParseInsitu<rapidjson::kParseNanAndInfFlag>(buffer.Data());
		if (document.HasParseError()) {
			LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document.GetParseError()), document.GetErrorOffset());
			resourceMetaLoaded = false;
		}
	}

	// Load resource meta
	if (resourceMetaLoaded) {
		JsonValue jResourceMeta(document, document);
		std::string resourceName = jResourceMeta[JSON_TAG_NAME];
		resource->SetName(resourceName.c_str());
		resource->LoadResourceMeta(jResourceMeta);
	}

	// Load resource
	resourcesToLoad.push(resource);
}

void ModuleResources::LoadImportOptions(std::unique_ptr<ImportOptions>& importOptions, const char* filePath) {
	if (importOptions == nullptr) {
		std::string extension = PathUtils::GetFileExtension(filePath);
		if (extension == JPG_TEXTURE_EXTENSION || extension == PNG_TEXTURE_EXTENSION || extension == TIF_TEXTURE_EXTENSION || extension == DDS_TEXTURE_EXTENSION || extension == TGA_TEXTURE_EXTENSION) {
			// Texture files
			importOptions.reset(new TextureImportOptions());
		} else {
			return;
		}
	}

	std::string metaFilePath = std::string(filePath) + META_EXTENSION;
	if (!App->files->Exists(metaFilePath.c_str())) return;

	// Read meta file
	Buffer<char> buffer = App->files->Load(metaFilePath.c_str());
	if (buffer.Size() == 0) {
		LOG("Error reading meta file %s", metaFilePath.c_str());
		return;
	}

	// Parse document from file
	rapidjson::Document document;
	document.Parse<rapidjson::kParseNanAndInfFlag>(buffer.Data());
	if (document.HasParseError()) {
		LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document.GetParseError()), document.GetErrorOffset());
		return;
	}

	JsonValue jMeta(document, document);
	importOptions->Load(jMeta);
}