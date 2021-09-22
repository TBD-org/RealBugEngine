#include "NavMeshImporter.h"

#include "Application.h"
#include "Modules/ModuleFiles.h"
#include "Importers/ImporterCommon.h"
#include "Resources/ResourceNavMesh.h"
#include "Navigation/NavMesh.h"
#include "Utils/PathUtils.h"
#include "Utils/Logging.h"
#include "Utils/MSTimer.h"
#include "Utils/Buffer.h"

#include "Utils/Leaks.h"

bool NavMeshImporter::ImportNavMesh(const char* filePath, JsonValue jMeta) {
	LOG("Importing NavMesh from path: \"%s\".", filePath);

	// Timer to measure importing a material
	MSTimer timer;
	timer.Start();

	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error loading NavMesh %s", filePath);
		return false;
	}

	// Create NavMesh resource
	unsigned resourceIndex = 0;
	std::unique_ptr<ResourceNavMesh> navMesh = ImporterCommon::CreateResource<ResourceNavMesh>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(navMesh.get());
	if (!saved) {
		LOG("Failed to save NavMesh resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(navMesh->GetResourceFilePath().c_str(), buffer);
	if (!saved) {
		LOG("Failed to save NavMesh resource file.");
		return false;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(navMesh);

	unsigned timeMs = timer.Stop();
	LOG("NavMesh imported in %ums", timeMs);
	return true;
}

bool NavMeshImporter::ExportNavMesh(NavMesh& navMesh, const char* filePath) {
	LOG("Exporting NavMesh to path: \"%s\".", filePath);
	// Timer to measure importing a material
	MSTimer timer;
	timer.Start();

	// Save to file
	Buffer<char> buffer = navMesh.Save();
	bool saved = App->files->Save(filePath, buffer);
	if (!saved) {
		LOG("Failed to save NavMesh.");
		return false;
	}

	unsigned timeMs = timer.Stop();
	LOG("NavMesh exported in %ums", timeMs);
	return true;
}