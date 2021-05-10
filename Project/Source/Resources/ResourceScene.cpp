#include "ResourceScene.h"

#include "Application.h"
#include "FileSystem/SceneImporter.h"

void ResourceScene::BuildScene() {
	SceneImporter::LoadScene(GetResourceFilePath().c_str());
}