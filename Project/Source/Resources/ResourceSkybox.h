#pragma once

#include "Resource.h"

#include "Utils/JsonValue.h"

class ResourceSkybox : public Resource {
public:
	REGISTER_RESOURCE(ResourceSkybox, ResourceType::SKYBOX);

	void Load() override;
	void FinishLoading() override;
	void Unload() override;

	unsigned GetGlCubeMap() const {
		return glCubeMap;
	}

	unsigned GetGlIrradianceMap() const {
		return glIrradianceMap;
	}

	unsigned GetGlPreFilteredMap() const {
		return glPreFilteredMap;
	}

	unsigned GetGlEnvironmentBRDF() const {
		return glEnvironmentBRDF;
	}

	int GetPreFilteredMapNumLevels() const {
		return preFilteredMapNumLevels;
	}

private:
	unsigned char* imageData = nullptr;
	unsigned width = 0;
	unsigned height = 0;

	unsigned glCubeMap = 0;
	unsigned glIrradianceMap = 0;
	unsigned glPreFilteredMap = 0;
	unsigned glEnvironmentBRDF = 0;

	int preFilteredMapNumLevels = 0;
};
