#include "ResourceSkybox.h"

#include "Globals.h"
#include "Application.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleCamera.h"
#include "Modules/ModulePrograms.h"
#include "Modules/ModuleRender.h"
#include "Utils/MSTimer.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"

#include "IL/il.h"
#include "IL/ilu.h"
#include "GL/glew.h"
#include <cstring>

#include "Utils/Leaks.h"

#define CUBEMAP_RESOLUTION 512
#define IRRADIANCE_MAP_RESOLUTION 128
#define PRE_FILTERED_MAP_RESOLUTION 128
#define ENVIRONMENT_BRDF_RESOLUTION 512

static void RenderToCubemap(unsigned cubemap, int resolution, ProgramCubemapRender* program, int level = 0) {
	// Frustum setup
	const float3 front[6] = {float3::unitX, -float3::unitX, float3::unitY, -float3::unitY, float3::unitZ, -float3::unitZ};
	const float3 up[6] = {-float3::unitY, -float3::unitY, float3::unitZ, -float3::unitZ, -float3::unitY, -float3::unitY};
	Frustum frustum;
	frustum.SetKind(FrustumSpaceGL, FrustumRightHanded);

	// Set viewport
	glViewport(0, 0, resolution, resolution);

	// Set projection matrix
	frustum.SetPerspective(math::pi / 2.0f, math::pi / 2.0f);
	frustum.SetViewPlaneDistances(0.1f, 100.0f);
	float4x4 proj = frustum.ProjectionMatrix();
	glUniformMatrix4fv(program->projLocation, 1, GL_TRUE, proj.ptr());

	// Bind Unit Cube VAO
	glBindVertexArray(App->renderer->cubeVAO);
	DEFER {
		glBindVertexArray(0);
	};

	// Render cube 6 times (once looking at each face)
	for (unsigned i = 0; i < 6; ++i) {
		// Bind face
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap, level);

		// Set view matrix
		frustum.SetFrame(float3::zero, front[i], up[i]);
		glUniformMatrix4fv(program->viewLocation, 1, GL_TRUE, frustum.ViewMatrix().ptr());

		// Draw cube
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
}

void ResourceSkybox::Load() {
	std::string filePath = GetResourceFilePath();
	LOG("Loading skybox from path: \"%s\".", filePath.c_str());

	// Timer to measure loading a skybox
	MSTimer timer;
	timer.Start();

	// Generate image handler
	unsigned image = 0;
	ilGenImages(1, &image);
	DEFER {
		ilDeleteImages(1, &image);
	};

	// Load image
	ilBindImage(image);
	bool imageLoaded = ilLoad(IL_HDR, filePath.c_str());
	if (!imageLoaded) {
		LOG("Failed to load image.");
		return;
	}

	// Convert image
	bool imageConverted = ilConvertImage(IL_RGB, IL_FLOAT);
	if (!imageConverted) {
		LOG("Failed to convert image.");
		return;
	}

	// Flip image if neccessary
	ILinfo info;
	iluGetImageInfo(&info);
	if (info.Origin == IL_ORIGIN_UPPER_LEFT) {
		iluFlipImage();
	}

	// Get data
	width = ilGetInteger(IL_IMAGE_WIDTH);
	height = ilGetInteger(IL_IMAGE_HEIGHT);
	unsigned bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	unsigned dataSize = width * height * bpp;
	imageData = new unsigned char[dataSize];
	memcpy(imageData, ilGetData(), dataSize);

	unsigned timeMs = timer.Stop();
	LOG("Skybox loaded in %ums.", timeMs);
}

void ResourceSkybox::FinishLoading() {
	if (imageData == nullptr) return;
	DEFER {
		RELEASE(imageData);
	};

	// Get shaders
	ProgramHDRToCubemap* hdrToCubemapProgram = App->programs->hdrToCubemap;
	ProgramIrradiance* irradianceProgram = App->programs->irradiance;
	ProgramPreFilteredMap* preFilteredMapProgram = App->programs->preFilteredMap;
	ProgramEnvironmentBRDF* environmentBRDFProgram = App->programs->environmentBRDF;
	if (!hdrToCubemapProgram || !irradianceProgram || !preFilteredMapProgram || !environmentBRDFProgram) {
		LOG("ERROR: Shaders haven't been loaded.");
		return;
	}

	// Create HDR texture
	unsigned hdrTexture = 0;
	glGenTextures(1, &hdrTexture);
	DEFER {
		glDeleteTextures(1, &hdrTexture);
	};

	// Load HDR texture from image
	glBindTexture(GL_TEXTURE_2D, hdrTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, imageData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create framebuffer
	unsigned captureFBO = 0;
	glCreateFramebuffers(1, &captureFBO);
	DEFER {
		glDeleteFramebuffers(1, &captureFBO);
	};
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	// Disable depth testing
	glDisable(GL_DEPTH_TEST);

	// Create cubemap texture
	glGenTextures(1, &glCubeMap);

	// Set texture parameters
	glBindTexture(GL_TEXTURE_CUBE_MAP, glCubeMap);
	for (unsigned int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, CUBEMAP_RESOLUTION, CUBEMAP_RESOLUTION, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Render cubemap from HDR image
	glUseProgram(hdrToCubemapProgram->program);

	glUniform1i(hdrToCubemapProgram->hdrLocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrTexture);

	RenderToCubemap(glCubeMap, CUBEMAP_RESOLUTION, hdrToCubemapProgram);

	// Generate environment mipmaps
	glBindTexture(GL_TEXTURE_CUBE_MAP, glCubeMap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// Create irradiance texture
	glGenTextures(1, &glIrradianceMap);

	// Set texture parameters
	glBindTexture(GL_TEXTURE_CUBE_MAP, glIrradianceMap);
	for (unsigned int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, IRRADIANCE_MAP_RESOLUTION, IRRADIANCE_MAP_RESOLUTION, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Render Irradiance cubemap
	glUseProgram(irradianceProgram->program);

	glUniform1i(irradianceProgram->environmentLocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, glCubeMap);

	RenderToCubemap(glIrradianceMap, IRRADIANCE_MAP_RESOLUTION, irradianceProgram);

	// Create pre-filtered environment texture
	glGenTextures(1, &glPreFilteredMap);

	// Set texture parameters
	glBindTexture(GL_TEXTURE_CUBE_MAP, glPreFilteredMap);
	for (unsigned int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, PRE_FILTERED_MAP_RESOLUTION, PRE_FILTERED_MAP_RESOLUTION, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	preFilteredMapNumLevels = int(log(float(PRE_FILTERED_MAP_RESOLUTION)) / log(2)) + 1;
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, preFilteredMapNumLevels - 1);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// Render pre-filtered environment cubemaps
	glUseProgram(preFilteredMapProgram->program);

	glUniform1i(preFilteredMapProgram->environmentLocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, glCubeMap);

	glUniform1f(preFilteredMapProgram->environmentResolutionLocation, static_cast<float>(CUBEMAP_RESOLUTION));

	for (int level = 0; level < preFilteredMapNumLevels; ++level) {
		float roughness = (float) level / (float) (preFilteredMapNumLevels - 1);
		glUniform1f(preFilteredMapProgram->roughnessLocation, roughness);

		unsigned levelResolution = static_cast<unsigned>(PRE_FILTERED_MAP_RESOLUTION * std::pow(0.5, level));
		RenderToCubemap(glPreFilteredMap, levelResolution, preFilteredMapProgram, level);
	}

	// Create environment BRDF texture
	glGenTextures(1, &glEnvironmentBRDF);

	// Set texture parameters
	glBindTexture(GL_TEXTURE_2D, glEnvironmentBRDF);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, ENVIRONMENT_BRDF_RESOLUTION, ENVIRONMENT_BRDF_RESOLUTION, 0, GL_RG, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Render environment BRDF texture
	glUseProgram(environmentBRDFProgram->program);

	glViewport(0, 0, ENVIRONMENT_BRDF_RESOLUTION, ENVIRONMENT_BRDF_RESOLUTION);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glEnvironmentBRDF, 0);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void ResourceSkybox::Unload() {
	RELEASE(imageData);

	if (glCubeMap) {
		glDeleteTextures(1, &glCubeMap);
		glCubeMap = 0;
	}

	if (glIrradianceMap) {
		glDeleteTextures(1, &glIrradianceMap);
		glIrradianceMap = 0;
	}

	if (glPreFilteredMap) {
		glDeleteTextures(1, &glPreFilteredMap);
		glPreFilteredMap = 0;
	}

	if (glEnvironmentBRDF) {
		glDeleteTextures(1, &glEnvironmentBRDF);
		glEnvironmentBRDF = 0;
	}
}