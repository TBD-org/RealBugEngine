#pragma once

#include "Resources/Resource.h"

class ResourceShader : public Resource {
public:
	REGISTER_RESOURCE(ResourceShader, ResourceType::SHADER);

	void FinishLoading() override;
	void Unload() override;

	unsigned int GetShaderProgram();

private:
	unsigned int shaderProgram = 0;
};
