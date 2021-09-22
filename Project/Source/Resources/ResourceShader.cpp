#include "ResourceShader.h"

#include "Application.h"
#include "Utils/Logging.h"
#include "Modules/ModulePrograms.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleTime.h"

#include "Utils/Leaks.h"

void ResourceShader::FinishLoading() {
	// Timer to measure loading a shader
	MSTimer timer;
	timer.Start();
	std::string filePath = GetResourceFilePath();
	LOG("Loading shader from path: \"%s\".", filePath.c_str());

	shaderProgram = App->programs->CreateProgram(filePath.c_str());

	unsigned timeMs = timer.Stop();
	LOG("Shader loaded in %ums.", timeMs);
}

void ResourceShader::Unload() {
	if (shaderProgram) App->programs->DeleteProgram(shaderProgram);
	shaderProgram = 0;
}

unsigned ResourceShader::GetShaderProgram() {
	return shaderProgram;
}
