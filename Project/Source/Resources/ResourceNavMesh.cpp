#include "ResourceNavMesh.h"

#include "Application.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleNavigation.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
#include "Utils/MSTimer.h"

void ResourceNavMesh::Load() {
	// Timer to measure loading a mesh
	MSTimer timer;
	timer.Start();
	std::string filePath = GetResourceFilePath();
	LOG("Loading mesh from path: \"%s\".", filePath.c_str());

	App->navigation->LoadNavMesh(filePath.c_str());

	unsigned timeMs = timer.Stop();
	LOG("Mesh loaded in %ums", timeMs);
}

void ResourceNavMesh::Unload() {
}
