#pragma once

#include "Resource.h"
#include "Navigation/NavMesh.h"

class ResourceNavMesh : public Resource {
public:
	REGISTER_RESOURCE(ResourceNavMesh, ResourceType::NAVMESH);

	void Load() override;
	void Unload() override;

	NavMesh* GetNavMesh();

private:
	NavMesh* navMesh = nullptr;
};
