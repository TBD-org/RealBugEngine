#include "ResourceNavMesh.h"

#include "Application.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleNavigation.h"
#include "Modules/ModuleScene.h"
#include "Components/ComponentAgent.h"
#include "Scene.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
#include "Utils/MSTimer.h"

#include "Utils/Leaks.h"

void ResourceNavMesh::Load() {
	// Timer to measure loading a mesh
	MSTimer timer;
	timer.Start();
	std::string filePath = GetResourceFilePath();
	LOG("Loading mesh from path: \"%s\".", filePath.c_str());

	Buffer<char> buffer = App->files->Load(filePath.c_str());
	navMesh = new NavMesh();
	navMesh->Load(buffer);

	unsigned timeMs = timer.Stop();
	LOG("Mesh loaded in %ums", timeMs);
}

void ResourceNavMesh::Unload() {
	RELEASE(navMesh);
}

NavMesh* ResourceNavMesh::GetNavMesh() {
	return navMesh;
}
