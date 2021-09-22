#include "ModelImporter.h"

#include "Application.h"
#include "Scene.h"
#include "GameObject.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
#include "Utils/MSTimer.h"
#include "Utils/PathUtils.h"
#include "Importers/PrefabImporter.h"
#include "Components/ComponentTransform.h"
#include "Components/ComponentBoundingBox.h"
#include "Components/ComponentMeshRenderer.h"
#include "Components/ComponentAnimation.h"
#include "Resources/ResourceMesh.h"
#include "Resources/ResourceMaterial.h"
#include "Resources/ResourceTexture.h"
#include "Resources/ResourcePrefab.h"
#include "Resources/ResourceAnimation.h"
#include "Resources/ResourceClip.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleFiles.h"
#include "ImporterCommon.h"

#include "assimp/mesh.h"
#include "assimp/scene.h"
#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

#include <set>

#define JSON_TAG_ROOT "Root"

static UID ImportMesh(const char* modelFilePath, JsonValue jMeta, const aiMesh* assimpMesh, unsigned& resourceIndex, std::vector<const char*>& bones) {
	// Timer to measure importing a mesh
	MSTimer timer;
	timer.Start();

	unsigned numVertices = assimpMesh->mNumVertices;
	unsigned numIndices = assimpMesh->mNumFaces * 3;
	unsigned materialIndex = assimpMesh->mMaterialIndex;
	unsigned numBones = assimpMesh->mNumBones;

	// Save to custom format buffer
	unsigned positionSize = sizeof(float) * 3;
	unsigned normalSize = sizeof(float) * 3;
	unsigned tangentSize = sizeof(float) * 3;
	unsigned uvSize = sizeof(float) * 2;
	unsigned bonesIDSize = sizeof(unsigned) * 4;
	unsigned weightsSize = sizeof(float) * 4;
	unsigned indexSize = sizeof(unsigned);
	unsigned boneSize = sizeof(unsigned) + sizeof(char) * FILENAME_MAX + sizeof(float) * 10;

	unsigned headerSize = sizeof(unsigned) * 3;
	unsigned vertexSize = positionSize + normalSize + tangentSize + uvSize + bonesIDSize + weightsSize;
	unsigned vertexBufferSize = vertexSize * numVertices;
	unsigned indexBufferSize = indexSize * numIndices;
	unsigned bonesBufferSize = boneSize * numBones;

	size_t size = headerSize + bonesBufferSize + vertexBufferSize + indexBufferSize;
	Buffer<char> buffer = Buffer<char>(size);
	char* cursor = buffer.Data();

	*((unsigned*) cursor) = numVertices;
	cursor += sizeof(unsigned);
	*((unsigned*) cursor) = numIndices;
	cursor += sizeof(unsigned);
	*((unsigned*) cursor) = numBones;
	cursor += sizeof(unsigned);

	std::vector<ResourceMesh::Attach> attaches;
	attaches.resize(numVertices);
	for (unsigned i = 0; i < assimpMesh->mNumBones; ++i) {
		aiBone* aiBone = assimpMesh->mBones[i];

		bones.push_back(aiBone->mName.C_Str());

		*((unsigned*) cursor) = aiBone->mName.length;
		cursor += sizeof(unsigned);

		memcpy_s(cursor, aiBone->mName.length * sizeof(char), aiBone->mName.data, aiBone->mName.length * sizeof(char));
		cursor += FILENAME_MAX * sizeof(char);

		//Transform
		aiVector3D position, scaling;
		aiQuaternion rotation;
		aiBone->mOffsetMatrix.Decompose(scaling, rotation, position);

		//Position
		*((float*) cursor) = position.x;
		cursor += sizeof(float);
		*((float*) cursor) = position.y;
		cursor += sizeof(float);
		*((float*) cursor) = position.z;
		cursor += sizeof(float);

		// Scaling
		*((float*) cursor) = scaling.x;
		cursor += sizeof(float);
		*((float*) cursor) = scaling.y;
		cursor += sizeof(float);
		*((float*) cursor) = scaling.z;
		cursor += sizeof(float);

		// Rotation
		*((float*) cursor) = rotation.x;
		cursor += sizeof(float);
		*((float*) cursor) = rotation.y;
		cursor += sizeof(float);
		*((float*) cursor) = rotation.z;
		cursor += sizeof(float);
		*((float*) cursor) = rotation.w;
		cursor += sizeof(float);

		for (unsigned j = 0; j < aiBone->mNumWeights; j++) {
			aiVertexWeight vtxWeight = aiBone->mWeights[j];

			attaches[vtxWeight.mVertexId].bones[attaches[vtxWeight.mVertexId].numBones] = i;
			attaches[vtxWeight.mVertexId].weights[attaches[vtxWeight.mVertexId].numBones] = vtxWeight.mWeight;
			attaches[vtxWeight.mVertexId].numBones++;
		}
	}

	for (unsigned i = 0; i < assimpMesh->mNumVertices; ++i) {
		aiVector3D& vertex = assimpMesh->mVertices[i];
		aiVector3D& normal = assimpMesh->mNormals[i];
		aiVector3D& tangent = assimpMesh->mTangents[i];
		aiVector3D* textureCoords = assimpMesh->mTextureCoords[0];

		*((float*) cursor) = vertex.x;
		cursor += sizeof(float);
		*((float*) cursor) = vertex.y;
		cursor += sizeof(float);
		*((float*) cursor) = vertex.z;
		cursor += sizeof(float);
		*((float*) cursor) = normal.x;
		cursor += sizeof(float);
		*((float*) cursor) = normal.y;
		cursor += sizeof(float);
		*((float*) cursor) = normal.z;
		cursor += sizeof(float);

		// Check if Tangents exist
		if ((aiVector3D*) assimpMesh->mTangents != nullptr) {
			*((float*) cursor) = tangent.x;
			cursor += sizeof(float);
			*((float*) cursor) = tangent.y;
			cursor += sizeof(float);
			*((float*) cursor) = tangent.z;
			cursor += sizeof(float);
		} else {
			*((float*) cursor) = 0;
			cursor += sizeof(float);
			*((float*) cursor) = 0;
			cursor += sizeof(float);
			*((float*) cursor) = 0;
			cursor += sizeof(float);
		}
		*((float*) cursor) = textureCoords != nullptr ? textureCoords[i].x : 0;
		cursor += sizeof(float);
		*((float*) cursor) = textureCoords != nullptr ? textureCoords[i].y : 0;
		cursor += sizeof(float);

		float weight = attaches[i].weights[0] + attaches[i].weights[1] + attaches[i].weights[2] + attaches[i].weights[3];

		*((unsigned*) cursor) = attaches[i].bones[0];
		cursor += sizeof(unsigned);
		*((unsigned*) cursor) = attaches[i].bones[1];
		cursor += sizeof(unsigned);
		*((unsigned*) cursor) = attaches[i].bones[2];
		cursor += sizeof(unsigned);
		*((unsigned*) cursor) = attaches[i].bones[3];
		cursor += sizeof(unsigned);

		*((float*) cursor) = attaches[i].weights[0] / weight;
		cursor += sizeof(float);
		*((float*) cursor) = attaches[i].weights[1] / weight;
		cursor += sizeof(float);
		*((float*) cursor) = attaches[i].weights[2] / weight;
		cursor += sizeof(float);
		*((float*) cursor) = attaches[i].weights[3] / weight;
		cursor += sizeof(float);
	}

	for (unsigned i = 0; i < assimpMesh->mNumFaces; ++i) {
		aiFace& assimpFace = assimpMesh->mFaces[i];

		// Assume triangles have 3 indices per face
		if (assimpFace.mNumIndices != 3) {
			LOG("Found a face with %i vertices. Discarded.", assimpFace.mNumIndices);

			*((unsigned*) cursor) = 0;
			cursor += sizeof(unsigned);
			*((unsigned*) cursor) = 0;
			cursor += sizeof(unsigned);
			*((unsigned*) cursor) = 0;
			cursor += sizeof(unsigned);
			continue;
		}

		*((unsigned*) cursor) = assimpFace.mIndices[0];
		cursor += sizeof(unsigned);
		*((unsigned*) cursor) = assimpFace.mIndices[1];
		cursor += sizeof(unsigned);
		*((unsigned*) cursor) = assimpFace.mIndices[2];
		cursor += sizeof(unsigned);
	}

	// Create mesh resource
	std::unique_ptr<ResourceMesh> mesh = ImporterCommon::CreateResource<ResourceMesh>(assimpMesh->mName.C_Str(), modelFilePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(mesh.get());
	if (!saved) {
		LOG("Failed to save mesh resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(mesh->GetResourceFilePath().c_str(), buffer);
	if (!saved) {
		LOG("Failed to save mesh resource file.");
		return 0;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(mesh);

	unsigned timeMs = timer.Stop();
	LOG("Mesh imported in %ums", timeMs);
	return mesh->GetId();
}

static void ImportAnimation(const char* modelFilePath, JsonValue jMeta, const aiAnimation* aiAnim, const aiScene* assimpScene, unsigned& resourceIndex, unsigned animationIndex) {
	// Timer to measure importing an animation
	MSTimer timer;
	timer.Start();

	unsigned headerSize = sizeof(unsigned) * 2 + sizeof(float);

	unsigned numKeyFrames = std::max(aiAnim->mChannels[0]->mNumPositionKeys, aiAnim->mChannels[0]->mNumRotationKeys);
	unsigned numChannels = aiAnim->mNumChannels;

	unsigned keyFrameVSize = sizeof(ResourceAnimation::Channel);
	unsigned keyFrameKSize = FILENAME_MAX / 2 * sizeof(char);
	unsigned keyFrameKLengthSize = sizeof(unsigned);
	unsigned keyFrameTotalSize = keyFrameVSize + keyFrameKSize + keyFrameKLengthSize;
	unsigned bufferSize = headerSize + numKeyFrames * numChannels * keyFrameTotalSize;

	Buffer<char> buffer(bufferSize);
	char* cursor = buffer.Data();

	float ticks = static_cast<float>(aiAnim->mTicksPerSecond);
	float duration = static_cast<float>(aiAnim->mDuration);
	float durationInSeconds = ticks > 0 ? (duration / ticks) : 0;
	*((float*) cursor) = durationInSeconds;
	cursor += sizeof(float);

	std::vector<ResourceAnimation::KeyFrameChannels> keyFrames;
	keyFrames.resize(numKeyFrames);

	aiNode* rootNode = assimpScene->mRootNode;

	std::set<std::string> uniqueChannels;
	for (unsigned int i = 0; i < aiAnim->mNumChannels; ++i) {
		aiNodeAnim* aiChannel = aiAnim->mChannels[i];
		std::string channelName(aiChannel->mNodeName.C_Str());

		aiNode* selectedNode = rootNode->FindNode(channelName.c_str());

		size_t pos = channelName.find("_$AssimpFbx$");
		if (pos != std::string::npos) {
			if (selectedNode->mNumChildren == 0) {
				uniqueChannels.insert(channelName);
			} else if (std::string(selectedNode->mChildren[0]->mName.C_Str()).find(channelName.substr(0, pos)) == std::string::npos) {
				uniqueChannels.insert(channelName);
			} else if (std::string(selectedNode->mChildren[0]->mName.C_Str()).find("_$AssimpFbx$") == std::string::npos) {
				uniqueChannels.insert(channelName);
			}
		}

		unsigned int frame = 0;

		for (unsigned int j = 0; j < numKeyFrames; ++j) {
			aiQuaternion aiQuat = (aiChannel->mNumRotationKeys > 1) ? aiChannel->mRotationKeys[j].mValue : aiChannel->mRotationKeys[0].mValue;
			aiVector3D aiV3D = (aiChannel->mNumPositionKeys > 1) ? aiChannel->mPositionKeys[j].mValue : aiChannel->mPositionKeys[0].mValue;

			ResourceAnimation::Channel channel = keyFrames[frame].channels[channelName];
			channel.rotation = Quat(aiQuat.x, aiQuat.y, aiQuat.z, aiQuat.w);
			channel.translation = float3(aiV3D.x, aiV3D.y, aiV3D.z);

			keyFrames[frame++].channels[channelName] = channel;
		}
	}

	// Post-processing: fuse nodes with same name
	for (ResourceAnimation::KeyFrameChannels& keyFrame : keyFrames) {
		for (auto itj = uniqueChannels.begin(); itj != uniqueChannels.end(); ++itj) {
			aiNode* node = rootNode->FindNode((*itj).c_str())->mParent;
			ResourceAnimation::Channel channel = keyFrame.channels[*itj];
			size_t pos = (*itj).find("_$AssimpFbx$");
			if (pos != std::string::npos) {
				std::string channelName = (*itj).substr(0, pos);
				while (std::string(node->mName.C_Str()).find(channelName) != std::string::npos) {
					if (keyFrame.channels.find(node->mName.C_Str()) != keyFrame.channels.end()) {
						channel.rotation = keyFrame.channels[node->mName.C_Str()].rotation * channel.rotation;
						channel.translation = keyFrame.channels[node->mName.C_Str()].translation + keyFrame.channels[node->mName.C_Str()].rotation.Transform(channel.translation);
						keyFrame.channels.erase(node->mName.C_Str());
					} else {
						aiVector3D scaling, translation;
						aiQuaternion rotation;
						node->mTransformation.Decompose(scaling, rotation, translation);
						channel.rotation = Quat(rotation.x, rotation.y, rotation.z, rotation.w) * channel.rotation;
						channel.translation = float3(translation.x, translation.y, translation.z) + Quat(rotation.x, rotation.y, rotation.z, rotation.w).Transform(channel.translation);
					}
					node = node->mParent;
				}
				keyFrame.channels.erase(*itj);
				aiNode* channelNode = rootNode->FindNode(channelName.c_str());
				if (keyFrame.channels.find(channelName) == keyFrame.channels.end() && channelNode) {
					aiVector3D scaling, translation;
					aiQuaternion rotation;
					channelNode->mTransformation.Decompose(scaling, rotation, translation);

					ResourceAnimation::Channel& c = keyFrame.channels[channelName];
					c.rotation = channel.rotation * Quat(rotation.x, rotation.y, rotation.z, rotation.w);
					c.translation = channel.translation + channel.rotation.Transform(float3(translation.x, translation.y, translation.z));
				} else {
					ResourceAnimation::Channel& c = keyFrame.channels[channelName];
					c.rotation = channel.rotation * c.rotation;
					c.translation = channel.translation + channel.rotation.Transform(c.translation);
				}
			}
		}
	}

	*((unsigned*) cursor) = keyFrames.size();
	cursor += sizeof(unsigned);

	*((unsigned*) cursor) = keyFrames[0].channels.size();
	cursor += sizeof(unsigned);

	for (const ResourceAnimation::KeyFrameChannels& keyFrame : keyFrames) {
		for (const auto& entry : keyFrame.channels) {
			const std::string& channelName = entry.first;
			const ResourceAnimation::Channel& channel = entry.second;

			*((unsigned*) cursor) = channelName.length();
			cursor += sizeof(unsigned);

			memcpy_s(cursor, channelName.size() * sizeof(char), channelName.data(), channelName.size() * sizeof(char));
			cursor += (FILENAME_MAX / 2) * sizeof(char);

			// Translation
			*((float*) cursor) = channel.translation.x;
			cursor += sizeof(float);
			*((float*) cursor) = channel.translation.y;
			cursor += sizeof(float);
			*((float*) cursor) = channel.translation.z;
			cursor += sizeof(float);

			// Rotation
			*((float*) cursor) = channel.rotation.x;
			cursor += sizeof(float);
			*((float*) cursor) = channel.rotation.y;
			cursor += sizeof(float);
			*((float*) cursor) = channel.rotation.z;
			cursor += sizeof(float);
			*((float*) cursor) = channel.rotation.w;
			cursor += sizeof(float);
		}
	}

	// Create animation
	std::unique_ptr<ResourceAnimation> animation = ImporterCommon::CreateResource<ResourceAnimation>(aiAnim->mName.C_Str(), modelFilePath, jMeta, resourceIndex);

	// Save resource meta file
	bool animationSaved = ImporterCommon::SaveResourceMetaFile(animation.get());
	if (!animationSaved) {
		LOG("Failed to save animation resource meta file.");
		return;
	}

	// Save animation to file
	animationSaved = App->files->Save(animation->GetResourceFilePath().c_str(), buffer);
	if (!animationSaved) {
		LOG("Failed to save animation resource file.");
		return;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(animation);

	// Load animation to create basic clip
	animation->Load();
	DEFER {
		animation->Unload();
	};

	// Create clip
	std::unique_ptr<ResourceClip> clip = ImporterCommon::CreateResource<ResourceClip>(aiAnim->mName.C_Str(), modelFilePath, jMeta, resourceIndex);
	clip->Init("clip" + std::to_string(animationIndex), animation->GetId(), 0, animation->keyFrames.size() - 1, true);
	clip->frameRate = animation->duration / animation->keyFrames.size();
	// Save resource meta file
	bool clipSaved = ImporterCommon::SaveResourceMetaFile(clip.get());
	if (!clipSaved) {
		LOG("Failed to save clip resource meta file.");
		return;
	}

	// Save clip to file
	clipSaved = clip->SaveToFile(clip->GetResourceFilePath().c_str());
	if (!clipSaved) {
		return;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(clip);

	unsigned timeMs = timer.Stop();
	LOG("Animation imported in %ums", timeMs);
}

static void ImportNode(const char* modelFilePath, JsonValue jMeta, const aiScene* assimpScene, const aiNode* node, Scene* scene, GameObject* parent, std::vector<UID>& materialIds, const float4x4& accumulatedTransform, unsigned& resourceIndex, std::vector<const char*>& bones) {
	std::string name = node->mName.C_Str();
	LOG("Importing node: \"%s\"", name.c_str());

	if (name.find("$AssimpFbx$") != std::string::npos) { // Auxiliary node
		// Import children nodes
		for (unsigned int i = 0; i < node->mNumChildren; ++i) {
			const float4x4& transform = accumulatedTransform * (*(float4x4*) &node->mTransformation);
			ImportNode(modelFilePath, jMeta, assimpScene, node->mChildren[i], scene, parent, materialIds, transform, resourceIndex, bones);
		}
	} else { // Normal node
		// Create GameObject
		GameObject* gameObject = scene->CreateGameObject(parent, GenerateUID(), name.c_str());

		// Load transform
		ComponentTransform* transform = gameObject->CreateComponent<ComponentTransform>();
		const float4x4& matrix = accumulatedTransform * (*(float4x4*) &node->mTransformation);
		float3 position;
		Quat rotation;
		float3 scale;
		matrix.Decompose(position, rotation, scale);
		transform->SetPosition(position);
		transform->SetRotation(rotation);
		transform->SetScale(scale);
		LOG("Transform: (%f, %f, %f), (%f, %f, %f, %f), (%f, %f, %f)", position.x, position.y, position.z, rotation.x, rotation.y, rotation.z, rotation.w, scale.x, scale.y, scale.z);

		// Save min and max points
		vec minPoint = vec(FLOAT_INF, FLOAT_INF, FLOAT_INF);
		vec maxPoint = vec(-FLOAT_INF, -FLOAT_INF, -FLOAT_INF);

		// Load meshes
		for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
			LOG("Importing meshRenderer %i", i);
			aiMesh* assimpMesh = assimpScene->mMeshes[node->mMeshes[i]];

			ComponentMeshRenderer* meshRenderer = gameObject->CreateComponent<ComponentMeshRenderer>();
			meshRenderer->SetMeshID(ImportMesh(modelFilePath, jMeta, assimpMesh, resourceIndex, bones));
			meshRenderer->SetMaterialID(materialIds[assimpMesh->mMaterialIndex]);

			// Update min and max points
			for (unsigned int j = 0; j < assimpMesh->mNumVertices; ++j) {
				aiVector3D vertex = assimpMesh->mVertices[j];
				if (vertex.x < minPoint.x) minPoint.x = vertex.x;
				if (vertex.y < minPoint.y) minPoint.y = vertex.y;
				if (vertex.z < minPoint.z) minPoint.z = vertex.z;
				if (vertex.x > maxPoint.x) maxPoint.x = vertex.x;
				if (vertex.y > maxPoint.y) maxPoint.y = vertex.y;
				if (vertex.z > maxPoint.z) maxPoint.z = vertex.z;
			}
		}

		// Create bounding box
		if (minPoint.x < maxPoint.x) {
			ComponentBoundingBox* boundingBox = gameObject->CreateComponent<ComponentBoundingBox>();
			boundingBox->SetLocalBoundingBox(AABB(minPoint, maxPoint));
		}

		// Import children nodes
		for (unsigned int i = 0; i < node->mNumChildren; ++i) {
			ImportNode(modelFilePath, jMeta, assimpScene, node->mChildren[i], scene, gameObject, materialIds, float4x4::identity, resourceIndex, bones);
		}
	}
}

static bool IsInBones(aiNode* bone, const std::vector<const char*>& bones) {
	for (const char* b : bones) {
		if (std::strcmp(bone->mName.C_Str(), b) != 0) continue;

		return true;
	}
	return false;
}

static aiNode* SearchRootBone(aiNode* rootBone, aiNode* rootBoneParent, const std::vector<const char*>& bones) {
	bool foundInBones = false;
	do {
		// Ignore assimp middle nodes
		std::string name = rootBoneParent->mName.C_Str();
		while (name.find("$AssimpFbx$") != std::string::npos) {
			rootBoneParent = rootBoneParent->mParent;
			name = rootBoneParent->mName.C_Str();
		}
		// Find if node in bones
		foundInBones = IsInBones(rootBoneParent, bones);
		if (foundInBones) {
			rootBone = rootBoneParent;
			rootBoneParent = rootBone->mParent;
		}
	} while (foundInBones);
	// Check whether selected root bone has siblings that are bones
	for (unsigned int i = 0; i < rootBoneParent->mNumChildren; ++i) {
		if (rootBoneParent->mChildren[i] == rootBone) continue;

		if (IsInBones(rootBoneParent->mChildren[i], bones)) {
			rootBone = rootBoneParent;
			break;
		}
	}
	return rootBone;
}

bool ModelImporter::ImportModel(const char* filePath, JsonValue jMeta) {
	// Timer to measure importing a model
	MSTimer timer;
	timer.Start();

	// Check for extension support
	std::string extension = PathUtils::GetFileExtension(filePath);
	if (!aiIsExtensionSupported(extension.c_str())) {
		LOG("Extension is not supported by assimp: \"%s\".", extension);
		return false;
	}

	// Import model
	LOG("Importing scene from path: \"%s\".", filePath);
	const aiScene* assimpScene = aiImportFile(filePath, aiProcessPreset_TargetRealtime_MaxQuality);
	DEFER {
		aiReleaseImport(assimpScene);
	};
	if (!assimpScene) {
		LOG("Error importing scene: %s", filePath, aiGetErrorString());
		return false;
	}

	// Initialize resource accumulator
	unsigned resourceIndex = 0;

	// Import materials
	std::vector<UID> materialIds;
	for (unsigned i = 0; i < assimpScene->mNumMaterials; ++i) {
		LOG("Loading material %i...", i);
		aiMaterial* assimpMaterial = assimpScene->mMaterials[i];

		std::unique_ptr<ResourceMaterial> material = ImporterCommon::CreateResource<ResourceMaterial>(assimpMaterial->GetName().C_Str(), filePath, jMeta, resourceIndex);

		aiString materialFilePath;
		aiTextureMapping mapping;
		aiColor4D color;
		unsigned uvIndex;

		if (assimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &materialFilePath, &mapping, &uvIndex) == AI_SUCCESS) {
			// Check if the material is valid for our purposes
			assert(mapping == aiTextureMapping_UV);
			assert(uvIndex == 0);

			// Try to load from the path given in the model file
			LOG("Trying to import diffuse texture...");
			std::list<UID>& textureResourceIds = App->resources->ImportAssetResources(materialFilePath.C_Str());

			// Try to load relative to the model folder
			if (textureResourceIds.empty()) {
				LOG("Trying to import texture relative to model folder...");
				std::string modelFolderPath = PathUtils::GetFileFolder(filePath);
				std::string modelFolderMaterialFilePath = modelFolderPath + "/" + materialFilePath.C_Str();
				textureResourceIds = App->resources->ImportAssetResources(modelFolderMaterialFilePath.c_str());
			}

			// Try to load relative to the textures folder
			if (textureResourceIds.empty()) {
				LOG("Trying to import texture relative to textures folder...");
				std::string materialFile = PathUtils::GetFileNameAndExtension(materialFilePath.C_Str());
				std::string texturesFolderMaterialFileDir = std::string(TEXTURES_PATH) + "/" + materialFile;
				textureResourceIds = App->resources->ImportAssetResources(texturesFolderMaterialFileDir.c_str());
			}

			if (textureResourceIds.empty()) {
				LOG("Unable to find diffuse texture file.");
			} else {
				LOG("Diffuse texture imported successfuly.");
				material->diffuseMapId = textureResourceIds.front();
			}
		} else {
			LOG("Diffuse texture not found.");
		}

		if (assimpMaterial->GetTexture(aiTextureType_SPECULAR, 0, &materialFilePath, &mapping, &uvIndex) == AI_SUCCESS) {
			// Check if the material is valid for our purposes
			assert(mapping == aiTextureMapping_UV);
			assert(uvIndex == 0);

			// Try to load from the path given in the model file
			LOG("Trying to import specular texture...");
			std::list<UID>& textureResourceIds = App->resources->ImportAssetResources(materialFilePath.C_Str());

			// Try to load relative to the model folder
			if (textureResourceIds.empty()) {
				LOG("Trying to import texture relative to model folder...");
				std::string modelFolderPath = PathUtils::GetFileFolder(filePath);
				std::string modelFolderMaterialFilePath = modelFolderPath + "/" + materialFilePath.C_Str();
				textureResourceIds = App->resources->ImportAssetResources(modelFolderMaterialFilePath.c_str());
			}

			// Try to load relative to the textures folder
			if (textureResourceIds.empty()) {
				LOG("Trying to import texture relative to textures folder...");
				std::string materialFileName = PathUtils::GetFileName(materialFilePath.C_Str());
				std::string texturesFolderMaterialFileDir = std::string(TEXTURES_PATH) + "/" + materialFileName + DDS_TEXTURE_EXTENSION;
				textureResourceIds = App->resources->ImportAssetResources(texturesFolderMaterialFileDir.c_str());
			}

			if (textureResourceIds.empty()) {
				LOG("Unable to find specular texture file.");
			} else {
				LOG("Specular texture imported successfuly.");
				material->specularMapId = textureResourceIds.front();
			}
		} else {
			LOG("Specular texture not found.");
		}

		if (assimpMaterial->GetTexture(aiTextureType_METALNESS, 0, &materialFilePath, &mapping, &uvIndex) == AI_SUCCESS) {
			// Check if the material is valid for our purposes
			assert(mapping == aiTextureMapping_UV);
			assert(uvIndex == 0);

			// Try to load from the path given in the model file
			LOG("Trying to import metalness texture...");
			std::list<UID>& textureResourceIds = App->resources->ImportAssetResources(materialFilePath.C_Str());

			// Try to load relative to the model folder
			if (textureResourceIds.empty()) {
				LOG("Trying to import texture relative to model folder...");
				std::string modelFolderPath = PathUtils::GetFileFolder(filePath);
				std::string modelFolderMaterialFilePath = modelFolderPath + "/" + materialFilePath.C_Str();
				textureResourceIds = App->resources->ImportAssetResources(modelFolderMaterialFilePath.c_str());
			}

			// Try to load relative to the textures folder
			if (textureResourceIds.empty()) {
				LOG("Trying to import texture relative to textures folder...");
				std::string materialFileName = PathUtils::GetFileName(materialFilePath.C_Str());
				std::string texturesFolderMaterialFileDir = std::string(TEXTURES_PATH) + "/" + materialFileName + DDS_TEXTURE_EXTENSION;
				textureResourceIds = App->resources->ImportAssetResources(texturesFolderMaterialFileDir.c_str());
			}

			if (textureResourceIds.empty()) {
				LOG("Unable to find metalness texture file.");
			} else {
				LOG("Metalness texture imported successfuly.");
				material->metallicMapId = textureResourceIds.front();
			}
		} else {
			LOG("Metalness texture not found.");
		}

		if (assimpMaterial->GetTexture(aiTextureType_NORMALS, 0, &materialFilePath, &mapping, &uvIndex) == AI_SUCCESS) {
			// Check if the material is valid for our purposes
			assert(mapping == aiTextureMapping_UV);
			assert(uvIndex == 0);

			// Try to load from the path given in the model file
			LOG("Trying to import normals texture...");
			std::list<UID>& textureResourceIds = App->resources->ImportAssetResources(materialFilePath.C_Str());

			// Try to load relative to the model folder
			if (textureResourceIds.empty()) {
				LOG("Trying to import texture relative to model folder...");
				std::string modelFolderPath = PathUtils::GetFileFolder(filePath);
				std::string modelFolderMaterialFilePath = modelFolderPath + "/" + materialFilePath.C_Str();
				textureResourceIds = App->resources->ImportAssetResources(modelFolderMaterialFilePath.c_str());
			}

			// Try to load relative to the textures folder
			if (textureResourceIds.empty()) {
				LOG("Trying to import texture relative to textures folder...");
				std::string materialFileName = PathUtils::GetFileName(materialFilePath.C_Str());
				std::string texturesFolderMaterialFileDir = std::string(TEXTURES_PATH) + "/" + materialFileName + DDS_TEXTURE_EXTENSION;
				textureResourceIds = App->resources->ImportAssetResources(texturesFolderMaterialFileDir.c_str());
			}

			if (textureResourceIds.empty()) {
				LOG("Unable to find normals texture file.");
			} else {
				LOG("Normals texture imported successfuly.");
				material->normalMapId = textureResourceIds.front();
			}
		} else {
			LOG("Normals texture not found.");
		}

		assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, material->diffuseColor);
		assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, material->specularColor);
		assimpMaterial->Get(AI_MATKEY_SHININESS, material->smoothness);

		// Save resource meta file
		ImporterCommon::SaveResourceMetaFile(material.get());

		// Save material
		material->SaveToFile(material->GetResourceFilePath().c_str());

		// Send resource creation event
		App->resources->SendCreateResourceEvent(material);

		materialIds.push_back(material->GetId());
		LOG("Material imported.");
	}

	// Import nodes
	Scene scene(1000);
	LOG("Importing scene tree.");
	GameObject* root = scene.CreateGameObject(nullptr, GenerateUID(), "Root");
	root->CreateComponent<ComponentTransform>();

	std::vector<const char*> bones;
	ImportNode(filePath, jMeta, assimpScene, assimpScene->mRootNode, &scene, root, materialIds, float4x4::identity, resourceIndex, bones);

	// Load animations
	if (assimpScene->mNumAnimations > 0) {
		LOG("Importing animations");

		// Import animations
		for (unsigned int i = 0; i < assimpScene->mNumAnimations; ++i) {
			ImportAnimation(filePath, jMeta, assimpScene->mAnimations[i], assimpScene, resourceIndex, i);
		}

		// Create empty animation component
		root->GetChildren()[0]->CreateComponent<ComponentAnimation>();
	}

	aiNode* rootBone = nullptr;

	if (!bones.empty()) {
		rootBone = assimpScene->mRootNode->FindNode(bones[0]);
		aiNode* rootBoneParent = rootBone->mParent;
		rootBone = SearchRootBone(rootBone, rootBoneParent, bones);
	}

	if (!bones.empty()) {
		GameObject* rootBoneGO = root->FindDescendant(rootBone->mName.C_Str());
		root->GetChildren()[0]->SetRootBone(rootBoneGO);

		std::unordered_map<std::string, GameObject*> goBones;
		goBones[rootBoneGO->name] = rootBoneGO;
		CacheBones(rootBoneGO, goBones);

		for (GameObject* child : root->GetChildren()[0]->GetChildren()) {
			if (child->name != rootBoneGO->name) {
				SaveBones(child, goBones);
			}
		}
	}

	// Create prefab resource
	std::unique_ptr<ResourcePrefab> prefab = ImporterCommon::CreateResource<ResourcePrefab>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(prefab.get());
	if (!saved) {
		LOG("Failed to save prefab resource meta file.");
		return false;
	}

	// Save prefab
	PrefabImporter::SavePrefab(prefab->GetResourceFilePath().c_str(), root->GetChildren()[0]);

	// Send resource creation event
	App->resources->SendCreateResourceEvent(prefab);

	// Delete temporary GameObject
	scene.DestroyGameObject(root);

	unsigned timeMs = timer.Stop();
	LOG("Scene imported in %ums.", timeMs);
	return true;
}

void ModelImporter::CacheBones(GameObject* node, std::unordered_map<std::string, GameObject*>& goBones) {
	for (GameObject* child : node->GetChildren()) {
		goBones[child->name] = child;
		CacheBones(child, goBones);
	}
}

void ModelImporter::SaveBones(GameObject* node, std::unordered_map<std::string, GameObject*>& goBones) {
	for (ComponentMeshRenderer& meshRenderer : node->GetComponents<ComponentMeshRenderer>()) {
		meshRenderer.SetGameObjectBones(goBones);
	}

	for (GameObject* child : node->GetChildren()) {
		SaveBones(child, goBones);
	}
}