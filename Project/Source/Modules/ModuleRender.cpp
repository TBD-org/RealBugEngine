#include "ModuleRender.h"

#include "Globals.h"
#include "Application.h"
#include "GameObject.h"
#include "Components/ComponentMeshRenderer.h"
#include "Components/ComponentBoundingBox.h"
#include "Components/ComponentTransform.h"
#include "Components/ComponentAnimation.h"
#include "Components/ComponentCamera.h"
#include "Components/ComponentParticleSystem.h"
#include "Components/ComponentTrail.h"
#include "Components/ComponentBillboard.h"
#include "Components/ComponentSkyBox.h"
#include "Components/ComponentLight.h"
#include "Modules/ModuleWindow.h"
#include "Modules/ModuleCamera.h"
#include "Modules/ModuleDebugDraw.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModulePrograms.h"
#include "Modules/ModuleEvents.h"
#include "Modules/ModuleUserInterface.h"
#include "Modules/ModuleNavigation.h"
#include "Resources/ResourceMesh.h"
#include "Resources/ResourceMaterial.h"
#include "Utils/Logging.h"
#include "Utils/Random.h"
#include "TesseractEvent.h"

#include "Geometry/AABB.h"
#include "Geometry/AABB2D.h"
#include "Geometry/OBB.h"
#include "debugdraw.h"
#include "GL/glew.h"
#include "SDL.h"
#include "Brofiler.h"
#include <string>
#include <math.h>
#include <vector>

float defIntGaussian(const float x, const float mu, const float sigma) {
	return 0.5f * erf((x - mu) / (sqrtf(2) * sigma));
}

void gaussianKernel(const int kernelSize, const float sigma, const float mu, const float step, std::vector<float>& coeff) {
	const float end = 0.5f * kernelSize;
	const float start = -end;
	float sum = 0;
	float x = start;
	float lastInt = defIntGaussian(x, mu, sigma);
	float acc = 0;
	while (x < end) {
		x += step;
		float newInt = defIntGaussian(x, mu, sigma);
		float c = newInt - lastInt;
		if (x >= 0) coeff.push_back(c);
		sum += c;
		lastInt = newInt;
	}

	//normalize
	sum = 1 / sum;
	for (unsigned int i = 0u; i < coeff.size(); ++i) {
		coeff[i] *= sum;
	}
}

// clang-format off
static const float cubeVertices[108] = {
	// Front (x, y, z)
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	// Left (x, y, z)
	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	// Right (x, y, z)
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,

	// Back (x, y, z)
	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	// Top (x, y, z)
	-1.0f,  1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	// Bottom (x, y, z)
	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	1.0f, -1.0f,  1.0f
}; // clang-format on

#if _DEBUG
static void __stdcall OurOpenGLErrorFunction(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	const char *tmpSource = "", *tmpType = "", *tmpSeverity = "";
	switch (source) {
	case GL_DEBUG_SOURCE_API:
		tmpSource = "API";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		tmpSource = "Window System";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		tmpSource = "Shader Compiler";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		tmpSource = "Third Party";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		tmpSource = "Application";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		tmpSource = "Other";
		break;
	};
	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		tmpType = "Error";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		tmpType = "Deprecated Behaviour";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		tmpType = "Undefined Behaviour";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		tmpType = "Portability";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		tmpType = "Performance";
		break;
	case GL_DEBUG_TYPE_MARKER:
		tmpType = "Marker";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		tmpType = "Push Group";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		tmpType = "Pop Group";
		break;
	case GL_DEBUG_TYPE_OTHER:
		tmpType = "Other";
		break;
	};
	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		tmpSeverity = "high";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		tmpSeverity = "medium";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		tmpSeverity = "low";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		tmpSeverity = "notification";
		break;
	};

	if (severity != GL_DEBUG_SEVERITY_HIGH) {
		return;
	}

	LOG("<Source:%s> <Type:%s> <Severity:%s> <ID:%d> <Message:%s>", tmpSource, tmpType, tmpSeverity, id, message);
}
#endif

bool ModuleRender::Init() {
	LOG("Creating Renderer context");

	context = SDL_GL_CreateContext(App->window->window);

	GLenum err = glewInit();
	LOG("Using Glew %s", glewGetString(GLEW_VERSION));

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_MULTISAMPLE);
	glFrontFace(GL_CCW);

#if _DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(&OurOpenGLErrorFunction, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
#endif

	glGenTextures(1, &renderTexture);
	glGenTextures(1, &outputTexture);
	glGenTextures(1, &depthsMSTexture);
	glGenTextures(1, &positionsMSTexture);
	glGenTextures(1, &normalsMSTexture);
	glGenTextures(1, &depthsTexture);
	glGenTextures(1, &positionsTexture);
	glGenTextures(1, &normalsTexture);
	glGenTextures(NUM_CASCADES_FRUSTUM, depthMapStaticTextures);
	glGenTextures(NUM_CASCADES_FRUSTUM, depthMapDynamicTextures);
	glGenTextures(1, &ssaoTexture);
	glGenTextures(1, &auxBlurTexture);
	glGenTextures(2, colorTextures);
	glGenTextures(2, bloomBlurTextures);
	glGenTextures(1, &bloomCombineTexture);

	glGenFramebuffers(1, &renderPassBuffer);
	glGenFramebuffers(1, &depthPrepassBuffer);
	glGenFramebuffers(1, &depthPrepassTextureConversionBuffer);
	glGenFramebuffers(NUM_CASCADES_FRUSTUM, depthMapStaticTextureBuffers);
	glGenFramebuffers(NUM_CASCADES_FRUSTUM, depthMapDynamicTextureBuffers);
	glGenFramebuffers(1, &ssaoTextureBuffer);
	glGenFramebuffers(1, &ssaoBlurTextureBufferH);
	glGenFramebuffers(1, &ssaoBlurTextureBufferV);
	glGenFramebuffers(1, &colorCorrectionBuffer);
	glGenFramebuffers(1, &hdrFramebuffer);
	glGenFramebuffers(12, bloomBlurFramebuffers);
	glGenFramebuffers(5, bloomCombineFramebuffers);

	ViewportResized(App->window->GetWidth(), App->window->GetHeight());
	UpdateFramebuffers();

	// Create Unit Cube VAO
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);

	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
	glBindVertexArray(0);

	// Create SSAO blur kernel
	gaussSSAOKernelRadius = 2;
	ssaoGaussKernel.clear();
	gaussianKernel(2 * gaussSSAOKernelRadius + 1, 1.0f, 0.f, 1.f, ssaoGaussKernel);

	// Calculate SSAO kernel
	for (unsigned i = 0; i < SSAO_KERNEL_SIZE; ++i) {
		float3 position;

		// Random direction
		position.x = Random() * 2.0f - 1.0f;
		position.y = Random() * 2.0f - 1.0f;
		position.z = Random();
		position.Normalize();

		// Random distance
		position *= Random();

		// Distribute according to (y = 0.1f + 0.9x^2)
		float scale = float(i) / float(SSAO_KERNEL_SIZE);
		scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
		position *= scale;

		ssaoKernel[i] = position;
	}

	// Calculate random tangents
	for (unsigned i = 0; i < RANDOM_TANGENTS_ROWS * RANDOM_TANGENTS_COLS; ++i) {
		float3 tangent;
		tangent.x = Random() * 2.0f - 1.0f;
		tangent.y = Random() * 2.0f - 1.0f;
		tangent.z = 0.0f;
		tangent.Normalize();
		randomTangents[i] = tangent;
	}

	return true;
}

void ModuleRender::ClassifyGameObjects() {
	opaqueGameObjects.clear();
	transparentGameObjects.clear();

	staticShadowCasters.clear();
	dynamicShadowCasters.clear();

	App->camera->CalculateFrustumPlanes();
	float3 cameraPos = App->camera->GetActiveCamera()->GetFrustum()->Pos();
	Scene* scene = App->scene->GetCurrentScene();
	for (ComponentBoundingBox& boundingBox : scene->boundingBoxComponents) {
		GameObject& gameObject = boundingBox.GetOwner();
		gameObject.flag = false;
		if (gameObject.isInQuadtree) continue;

		const AABB& gameObjectAABB = boundingBox.GetWorldAABB();
		const OBB& gameObjectOBB = boundingBox.GetWorldOBB();
		if (!App->camera->GetFrustumPlanes().CheckIfInsideFrustumPlanes(gameObjectAABB, gameObjectOBB)) continue;

		ComponentMeshRenderer* meshRenderer = gameObject.GetComponent<ComponentMeshRenderer>();
		if (meshRenderer == nullptr) continue;

		ResourceMaterial* material = App->resources->GetResource<ResourceMaterial>(meshRenderer->GetMaterialID());
		if (material == nullptr) continue;

		if (material->renderingMode == RenderingMode::OPAQUE) {
			opaqueGameObjects.push_back(&gameObject);
		} else {
			ComponentTransform* transform = gameObject.GetComponent<ComponentTransform>();
			float dist = Length(cameraPos - transform->GetGlobalPosition());
			transparentGameObjects[dist] = &gameObject;
		}

		if (material->castShadows) {
			if (material->shadowCasterType == ShadowCasterType::STATIC) {
				staticShadowCasters.push_back(&gameObject);
			} else {
				dynamicShadowCasters.push_back(&gameObject);
			}
		}
	}
	if (scene->quadtree.IsOperative()) {
		ClassifyGameObjectsFromQuadtree(scene->quadtree.root, scene->quadtree.bounds);
	}
}

void ModuleRender::ConvertDepthPrepassTextures() {
	ProgramDepthPrepassConvertTextures* convertProgram = App->programs->depthPrepassConvertTextures;
	if (convertProgram == nullptr) return;

	glUseProgram(convertProgram->program);

	glUniform1i(convertProgram->samplesNumberLocation, msaaActive ? msaaSamplesNumber[static_cast<int>(msaaSampleType)] : msaaSampleSingle);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, depthsMSTexture);
	glUniform1i(convertProgram->depthsLocation, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, positionsMSTexture);
	glUniform1i(convertProgram->positionsLocation, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, normalsMSTexture);
	glUniform1i(convertProgram->normalsLocation, 2);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void ModuleRender::ComputeSSAOTexture() {
	ProgramSSAO* ssaoProgram = App->programs->ssao;
	if (ssaoProgram == nullptr) return;

	float4x4 viewMatrix = App->camera->GetViewMatrix();
	float4x4 projMatrix = App->camera->GetProjectionMatrix();

	glUseProgram(ssaoProgram->program);

	glUniformMatrix4fv(ssaoProgram->projLocation, 1, GL_TRUE, projMatrix.ptr());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, positionsTexture);
	glUniform1i(ssaoProgram->positionsLocation, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalsTexture);
	glUniform1i(ssaoProgram->normalsLocation, 1);

	glUniform3fv(ssaoProgram->kernelSamplesLocation, SSAO_KERNEL_SIZE, ssaoKernel[0].ptr());
	glUniform3fv(ssaoProgram->randomTangentsLocation, RANDOM_TANGENTS_ROWS * RANDOM_TANGENTS_COLS, randomTangents[0].ptr());
	glUniform2f(ssaoProgram->screenSizeLocation, viewportSize.x, viewportSize.y);
	glUniform1f(ssaoProgram->biasLocation, ssaoBias);
	glUniform1f(ssaoProgram->rangeLocation, ssaoRange);
	glUniform1f(ssaoProgram->powerLocation, ssaoPower);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void ModuleRender::BlurSSAOTexture(bool horizontal) {
	ProgramBlur* ssaoBlurProgram = App->programs->blur;
	if (ssaoBlurProgram == nullptr) return;

	glUseProgram(ssaoBlurProgram->program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, horizontal ? ssaoTexture : auxBlurTexture);
	glUniform1i(ssaoBlurProgram->inputTextureLocation, 0);
	glUniform1i(ssaoBlurProgram->textureLevelLocation, 0);

	glUniform1fv(ssaoBlurProgram->kernelLocation, gaussSSAOKernelRadius + 1, &ssaoGaussKernel[0]);
	glUniform1i(ssaoBlurProgram->kernelRadiusLocation, gaussSSAOKernelRadius + 1);
	glUniform1i(ssaoBlurProgram->horizontalLocation, horizontal ? 1 : 0);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void ModuleRender::ExecuteColorCorrection() {
	ProgramColorCorrection* colorCorrectionProgram = App->programs->colorCorrection;
	if (colorCorrectionProgram == nullptr) return;

	glUseProgram(colorCorrectionProgram->program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorTextures[0]);
	glUniform1i(colorCorrectionProgram->sceneTextureLocation, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bloomBlurTextures[0]);
	glUniform1i(colorCorrectionProgram->bloomTextureLocation, 1);
	glUniform1i(colorCorrectionProgram->hasBloomLocation, bloomActive ? 1 : 0);

	glUniform1f(colorCorrectionProgram->bloomIntensityLocation, bloomIntensity);

	glUniform1i(colorCorrectionProgram->hasChromaticAberrationLocation, chromaticAberrationActive ? 1 : 0);
	glUniform1f(colorCorrectionProgram->chromaticAberrationStrengthLocation, chromaticAberrationStrength);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void ModuleRender::BlurBloomTexture(unsigned bloomTexture, bool horizontal, const std::vector<float>& kernel, int kernelRadius, int textureLevel) {
	ProgramBlur* bloomBlurProgram = App->programs->blur;
	if (bloomBlurProgram == nullptr) return;

	glUseProgram(bloomBlurProgram->program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bloomTexture);
	glUniform1i(bloomBlurProgram->inputTextureLocation, 0);
	glUniform1i(bloomBlurProgram->textureLevelLocation, textureLevel);

	glUniform1fv(bloomBlurProgram->kernelLocation, kernelRadius + 1, &kernel[0]);
	glUniform1i(bloomBlurProgram->kernelRadiusLocation, kernelRadius + 1);
	glUniform1i(bloomBlurProgram->horizontalLocation, horizontal ? 1 : 0);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void ModuleRender::BloomCombine(unsigned bloomTexture, int bloomTextureLevel, int brightTextureLevel, float bloomWeight) {
	ProgramBloomCombine* bloomCombineProgram = App->programs->bloomCombine;
	if (bloomCombineProgram == nullptr) return;

	glUseProgram(bloomCombineProgram->program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorTextures[1]);
	glUniform1i(bloomCombineProgram->brightLevelLocation, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bloomTexture);
	glUniform1i(bloomCombineProgram->bloomTextureLocation, 1);

	glUniform1i(bloomCombineProgram->brightLevelLocation, brightTextureLevel);
	glUniform1i(bloomCombineProgram->bloomLevelLocation, bloomTextureLevel);
	glUniform1f(bloomCombineProgram->bloomWeightLocation, bloomWeight);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void ModuleRender::DrawTexture(unsigned texture) {
	ProgramDrawTexture* drawTextureProgram = App->programs->drawTexture;
	if (drawTextureProgram == nullptr) return;

	glUseProgram(drawTextureProgram->program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(drawTextureProgram->textureToDrawLocation, 0);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void ModuleRender::DrawScene() {
	ProgramPostprocess* drawScene = App->programs->postprocess;
	if (drawScene == nullptr) return;

	glUseProgram(drawScene->program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderTexture);
	glUniform1i(drawScene->sceneTextureLocation, 0);
	glUniform1f(drawScene->bloomThresholdLocation, bloomThreshold);
	glUniform1i(drawScene->samplesNumberLocation, msaaActive ? msaaSamplesNumber[static_cast<int>(msaaSampleType)] : msaaSampleSingle);
	glUniform1i(drawScene->bloomActiveLocation, bloomActive);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

bool ModuleRender::Start() {
	App->events->AddObserverToEvent(TesseractEventType::SCREEN_RESIZED, this);
	return true;
}

UpdateStatus ModuleRender::PreUpdate() {
	BROFILER_CATEGORY("ModuleRender - PreUpdate", Profiler::Color::Green)

#if GAME
	App->camera->ViewportResized(App->window->GetWidth(), App->window->GetHeight());
#endif
	glViewport(0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));

	return UpdateStatus::CONTINUE;
}

UpdateStatus ModuleRender::Update() {
	BROFILER_CATEGORY("ModuleRender - Update", Profiler::Color::Green)

	culledTriangles = 0;
	Scene* scene = App->scene->GetCurrentScene();
	float3 gammaClearColor = float3(pow(clearColor.x, 2.2f), pow(clearColor.y, 2.2f), pow(clearColor.y, 2.2f));

	ClassifyGameObjects();

	lightFrustumStatic.UpdateCameraFrustum();
	lightFrustumStatic.UpdateLightFrustum(ShadowCasterType::STATIC);

	lightFrustumDynamic.UpdateCameraFrustum();
	lightFrustumDynamic.UpdateLightFrustum(ShadowCasterType::DYNAMIC);

	// Shadow Pass Static
	for (unsigned int i = 0; i < NUM_CASCADES_FRUSTUM; ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapStaticTextureBuffers[i]);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
		glClear(GL_DEPTH_BUFFER_BIT);

		for (GameObject* gameObject : staticShadowCasters) {
			DrawGameObjectShadowPass(gameObject, i, ShadowCasterType::STATIC);
		}
	}

	// Shadow Pass Dynamic
	for (unsigned int i = 0; i < NUM_CASCADES_FRUSTUM; ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapDynamicTextureBuffers[i]);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
		glClear(GL_DEPTH_BUFFER_BIT);

		for (GameObject* gameObject : dynamicShadowCasters) {
			DrawGameObjectShadowPass(gameObject, i, ShadowCasterType::DYNAMIC);
		}
	}

	// Depth Prepass
	glBindFramebuffer(GL_FRAMEBUFFER, depthPrepassBuffer);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (GameObject* gameObject : opaqueGameObjects) {
		DrawGameObjectDepthPrepass(gameObject);
	}

	// Depth Prepass texture conversion
	glBindFramebuffer(GL_FRAMEBUFFER, depthPrepassTextureConversionBuffer);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_ALWAYS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ConvertDepthPrepassTextures();

	// SSAO pass
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoTextureBuffer);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LESS);
	glClear(GL_COLOR_BUFFER_BIT);

	if (ssaoActive) {
		ComputeSSAOTexture();
	}

	// SSAO horitontal blur
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurTextureBufferH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);

	if (ssaoActive) {
		BlurSSAOTexture(true);
	}

	// SSAO vertical blur
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurTextureBufferV);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);

	if (ssaoActive) {
		BlurSSAOTexture(false);
	}

	// Render pass
	glBindFramebuffer(GL_FRAMEBUFFER, renderPassBuffer);
	glClearColor(gammaClearColor.x, gammaClearColor.y, gammaClearColor.z, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	glClear(GL_COLOR_BUFFER_BIT);

	// Debug textures
	if (drawNormalsTexture) {
		DrawTexture(normalsTexture);

		// Render to screen
		glBindFramebuffer(GL_READ_FRAMEBUFFER, renderPassBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, colorCorrectionBuffer);
		glBlitFramebuffer(0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), GL_COLOR_BUFFER_BIT, GL_NEAREST);
		return UpdateStatus::CONTINUE;
	}
	if (drawPositionsTexture) {
		DrawTexture(positionsTexture);

		// Render to screen
		glBindFramebuffer(GL_READ_FRAMEBUFFER, renderPassBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, colorCorrectionBuffer);
		glBlitFramebuffer(0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), GL_COLOR_BUFFER_BIT, GL_NEAREST);
		return UpdateStatus::CONTINUE;
	}
	if (drawSSAOTexture) {
		DrawTexture(ssaoTexture);

		// Render to screen
		glBindFramebuffer(GL_READ_FRAMEBUFFER, renderPassBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, colorCorrectionBuffer);
		glBlitFramebuffer(0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), GL_COLOR_BUFFER_BIT, GL_NEAREST);
		return UpdateStatus::CONTINUE;
	}
	if (drawDepthMapTexture) {
		assert(drawDepthMapTexture && indexDepthMapTexture < NUM_CASCADES_FRUSTUM);

		if (shadowCasterType == ShadowCasterType::STATIC) {
			DrawTexture(depthMapStaticTextures[indexDepthMapTexture]);
		} else {
			DrawTexture(depthMapDynamicTextures[indexDepthMapTexture]);
		}

		// Render to screen
		glBindFramebuffer(GL_READ_FRAMEBUFFER, renderPassBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, colorCorrectionBuffer);
		glBlitFramebuffer(0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), GL_COLOR_BUFFER_BIT, GL_NEAREST);
		return UpdateStatus::CONTINUE;
	}

	// Draw SkyBox (Always first element)
	for (ComponentSkyBox& skybox : scene->skyboxComponents) {
		if (skybox.IsActive()) skybox.Draw();
	}

	// Draw Opaque
	glDepthFunc(GL_EQUAL);
	for (GameObject* gameObject : opaqueGameObjects) {
		DrawGameObject(gameObject);
	}
	glDepthFunc(GL_LEQUAL);

	// Draw Fog
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (ComponentFog& fog : scene->fogComponents) {
		if (fog.IsActive()) fog.Draw();
	}
	glDisable(GL_BLEND);

	// Draw Transparent
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (std::map<float, GameObject*>::reverse_iterator it = transparentGameObjects.rbegin(); it != transparentGameObjects.rend(); ++it) {
		DrawGameObject((*it).second);
	}
	glDisable(GL_BLEND);

	// Draw particles (TODO: improve with culling)
	for (ComponentParticleSystem& particleSystem : scene->particleComponents) {
		if (particleSystem.IsActive()) particleSystem.Draw();
	}
	for (ComponentBillboard& billboard : scene->billboardComponents) {
		if (billboard.IsActive()) billboard.Draw();
	}
	for (ComponentTrail& trail : scene->trailComponents) {
		if (trail.IsActive()) trail.Draw();
	}

	// Draw Gizmos
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	if (App->camera->IsEngineCameraActive() || debugMode) {
		// Draw NavMesh
		if (drawNavMesh) {
			App->navigation->DrawGizmos();
		}

		GameObject* selectedGameObject = App->editor->selectedGameObject;
		if (selectedGameObject) selectedGameObject->DrawGizmos();

		// --- All Gizmos options
		if (drawCameraFrustums) {
			for (ComponentCamera& camera : scene->cameraComponents) {
				camera.DrawGizmos();
			}
		}
		if (drawLightGizmos) {
			for (ComponentLight& light : scene->lightComponents) {
				light.DrawGizmos();
			}
		}
		if (drawParticleGizmos) {
			for (ComponentParticleSystem& particle : scene->particleComponents) {
				particle.DrawGizmos();
			}
		}

		if (drawColliders) {
			for (ComponentBoxCollider& collider : scene->boxColliderComponents) {
				collider.DrawGizmos();
			}
			for (ComponentSphereCollider& collider : scene->sphereColliderComponents) {
				collider.DrawGizmos();
			}
			for (ComponentCapsuleCollider& collider : scene->capsuleColliderComponents) {
				collider.DrawGizmos();
			}
		}

		// Draw quadtree
		if (drawQuadtree) DrawQuadtreeRecursive(scene->quadtree.root, scene->quadtree.bounds);

		// Draw debug draw
		if (drawDebugDraw) App->debugDraw->Draw(App->camera->GetViewMatrix(), App->camera->GetProjectionMatrix(), static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));

		// Draw Animations
		if (drawAllBones) {
			for (ComponentAnimation& animationComponent : scene->animationComponents) {
				GameObject* rootBone = animationComponent.GetOwner().GetRootBone();
				if (rootBone) DrawAnimation(rootBone);
			}
		}

		// Draw debug draw Light Frustum
		if (drawLightFrustumGizmo) {
			lightFrustumStatic.DrawGizmos();
		}
	}

	// Render UI
	RenderUI();

	// Apply MSAA and bloom threshold
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFramebuffer);
	glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	DrawScene();

	// Bloom blur
	bool horizontal = true;
	if (bloomActive) {
		glBindTexture(GL_TEXTURE_2D, colorTextures[1]);
		glGenerateMipmap(GL_TEXTURE_2D);

		int width = static_cast<int>(viewportSize.x);
		int height = static_cast<int>(viewportSize.y);

		glViewport(0, 0, width / (1 << gaussVeryLargeMipLevel), height / (1 << gaussVeryLargeMipLevel));

		for (unsigned int i = 0; i < 2; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFramebuffers[horizontal]);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glDisable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT);
			BlurBloomTexture(horizontal ? colorTextures[1] : bloomBlurTextures[1], horizontal, bloomGaussKernel, gaussBloomKernelRadius, gaussVeryLargeMipLevel);

			horizontal = !horizontal;
		}

		glViewport(0, 0, width / (1 << gaussLargeMipLevel), height / (1 << gaussLargeMipLevel));

		glBindFramebuffer(GL_FRAMEBUFFER, bloomCombineFramebuffers[0]);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
		BloomCombine(bloomBlurTextures[0], gaussVeryLargeMipLevel, gaussLargeMipLevel, bloomVeryLargeWeight);

		for (unsigned int i = 0; i < 2; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFramebuffers[2 + horizontal]);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glDisable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT);
			BlurBloomTexture(horizontal ? bloomCombineTexture : bloomBlurTextures[1], horizontal, bloomGaussKernel, gaussBloomKernelRadius, gaussLargeMipLevel);

			horizontal = !horizontal;
		}

		glViewport(0, 0, width / (1 << gaussMediumMipLevel), height / (1 << gaussMediumMipLevel));

		glBindFramebuffer(GL_FRAMEBUFFER, bloomCombineFramebuffers[1]);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
		BloomCombine(bloomBlurTextures[0], gaussLargeMipLevel, gaussMediumMipLevel, bloomLargeWeight);

		for (unsigned int i = 0; i < 2; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFramebuffers[4 + horizontal]);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glDisable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT);
			BlurBloomTexture(horizontal ? bloomCombineTexture : bloomBlurTextures[1], horizontal, bloomGaussKernel, gaussBloomKernelRadius, gaussMediumMipLevel);

			horizontal = !horizontal;
		}

		glViewport(0, 0, width / (1 << gaussSmallMipLevel), height / (1 << gaussSmallMipLevel));

		glBindFramebuffer(GL_FRAMEBUFFER, bloomCombineFramebuffers[2]);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
		BloomCombine(bloomBlurTextures[0], gaussMediumMipLevel, gaussSmallMipLevel, bloomMediumWeight);

		for (unsigned int i = 0; i < 2; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFramebuffers[6 + horizontal]);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glDisable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT);
			BlurBloomTexture(horizontal ? bloomCombineTexture : bloomBlurTextures[1], horizontal, bloomGaussKernel, gaussBloomKernelRadius, gaussSmallMipLevel);

			horizontal = !horizontal;
		}

		glViewport(0, 0, width / (1 << gaussVerySmallMipLevel), height / (1 << gaussVerySmallMipLevel));

		glBindFramebuffer(GL_FRAMEBUFFER, bloomCombineFramebuffers[3]);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
		BloomCombine(bloomBlurTextures[0], gaussSmallMipLevel, gaussVerySmallMipLevel, bloomSmallWeight);

		for (unsigned int i = 0; i < 2; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFramebuffers[8 + horizontal]);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glDisable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT);
			BlurBloomTexture(horizontal ? bloomCombineTexture : bloomBlurTextures[1], horizontal, bloomGaussKernel, gaussBloomKernelRadius, gaussVerySmallMipLevel);

			horizontal = !horizontal;
		}

		glViewport(0, 0, width, height);

		glBindFramebuffer(GL_FRAMEBUFFER, bloomCombineFramebuffers[4]);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
		BloomCombine(bloomBlurTextures[0], gaussVerySmallMipLevel, 0, bloomVerySmallWeight);

		for (unsigned int i = 0; i < 2; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFramebuffers[10 + horizontal]);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glDisable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT);
			BlurBloomTexture(horizontal ? bloomCombineTexture : bloomBlurTextures[1], horizontal, bloomGaussKernel, gaussBloomKernelRadius, 0);

			horizontal = !horizontal;
		}
	}

	// Color correction
	glBindFramebuffer(GL_FRAMEBUFFER, colorCorrectionBuffer);
	glClearColor(gammaClearColor.x, gammaClearColor.y, gammaClearColor.z, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	ExecuteColorCorrection();

	// Render to screen
#if GAME
	glBindFramebuffer(GL_READ_FRAMEBUFFER, colorCorrectionBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), GL_COLOR_BUFFER_BIT, GL_NEAREST);
#endif

	return UpdateStatus::CONTINUE;
}

UpdateStatus ModuleRender::PostUpdate() {
	BROFILER_CATEGORY("ModuleRender - PostUpdate", Profiler::Color::Green)

	if (viewportUpdated) {
		UpdateFramebuffers();
		viewportUpdated = false;
	}

	SDL_GL_SwapWindow(App->window->window);

	return UpdateStatus::CONTINUE;
}

bool ModuleRender::CleanUp() {
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);

	glDeleteTextures(1, &renderTexture);
	glDeleteTextures(1, &outputTexture);
	glDeleteTextures(1, &depthsMSTexture);
	glDeleteTextures(1, &positionsMSTexture);
	glDeleteTextures(1, &normalsMSTexture);
	glDeleteTextures(1, &depthsTexture);
	glDeleteTextures(1, &positionsTexture);
	glDeleteTextures(1, &normalsTexture);
	glDeleteTextures(NUM_CASCADES_FRUSTUM, depthMapStaticTextures);
	glDeleteTextures(NUM_CASCADES_FRUSTUM, depthMapDynamicTextures);
	glDeleteTextures(1, &ssaoTexture);
	glDeleteTextures(1, &auxBlurTexture);
	glDeleteTextures(2, colorTextures);
	glDeleteTextures(2, bloomBlurTextures);
	glDeleteTextures(1, &bloomCombineTexture);

	glDeleteFramebuffers(1, &renderPassBuffer);
	glDeleteFramebuffers(1, &depthPrepassBuffer);
	glDeleteFramebuffers(1, &depthPrepassTextureConversionBuffer);
	glDeleteFramebuffers(NUM_CASCADES_FRUSTUM, depthMapStaticTextureBuffers);
	glDeleteFramebuffers(NUM_CASCADES_FRUSTUM, depthMapDynamicTextureBuffers);
	glDeleteFramebuffers(1, &ssaoTextureBuffer);
	glDeleteFramebuffers(1, &ssaoBlurTextureBufferH);
	glDeleteFramebuffers(1, &ssaoBlurTextureBufferV);
	glDeleteFramebuffers(1, &colorCorrectionBuffer);
	glDeleteFramebuffers(1, &hdrFramebuffer);
	glDeleteFramebuffers(12, bloomBlurFramebuffers);
	glDeleteFramebuffers(5, bloomCombineFramebuffers);

	return true;
}

void ModuleRender::ViewportResized(int width, int height) {
	viewportSize.x = static_cast<float>(width);
	viewportSize.y = static_cast<float>(height);

	viewportUpdated = true;
}

void ModuleRender::ReceiveEvent(TesseractEvent& ev) {
	switch (ev.type) {
	case TesseractEventType::SCREEN_RESIZED:
		ViewportResized(ev.Get<ViewportResizedStruct>().newWidth, ev.Get<ViewportResizedStruct>().newHeight);
		break;
	default:
		break;
	}
}

void ModuleRender::UpdateFramebuffers() {
	unsigned msaaSamples = msaaActive ? msaaSamplesNumber[static_cast<int>(msaaSampleType)] : msaaSampleSingle;

	// Depth prepass buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthPrepassBuffer);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, depthsMSTexture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaaSamples, GL_DEPTH24_STENCIL8, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), GL_TRUE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depthsMSTexture, 0);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, positionsMSTexture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaaSamples, GL_RGB16F, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), GL_TRUE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, positionsMSTexture, 0);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, normalsMSTexture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaaSamples, GL_RGB16F, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), GL_TRUE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, normalsMSTexture, 0);

	GLuint drawBuffers2[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
	glDrawBuffers(2, drawBuffers2);

	// Depth prepass texture conversion buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthPrepassTextureConversionBuffer);

	glBindTexture(GL_TEXTURE_2D, depthsTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthsTexture, 0);

	glBindTexture(GL_TEXTURE_2D, positionsTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, positionsTexture, 0);

	glBindTexture(GL_TEXTURE_2D, normalsTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalsTexture, 0);

	glDrawBuffers(2, drawBuffers2);

	// Shadow buffer

	for (unsigned int i = 0; i < NUM_CASCADES_FRUSTUM; ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapStaticTextureBuffers[i]);

		glBindTexture(GL_TEXTURE_2D, depthMapStaticTextures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, static_cast<int>(viewportSize.x * lightFrustumStatic.GetSubFrustums()[i].multiplier), static_cast<int>(viewportSize.y * lightFrustumStatic.GetSubFrustums()[i].multiplier), 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapStaticTextures[i], 0);

		glBindFramebuffer(GL_FRAMEBUFFER, depthMapDynamicTextureBuffers[i]);

		glBindTexture(GL_TEXTURE_2D, depthMapDynamicTextures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, static_cast<int>(viewportSize.x * lightFrustumDynamic.GetSubFrustums()[i].multiplier), static_cast<int>(viewportSize.y * lightFrustumDynamic.GetSubFrustums()[i].multiplier), 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapDynamicTextures[i], 0);
	}

	glDrawBuffer(GL_NONE);

	// SSAO buffer
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoTextureBuffer);

	glBindTexture(GL_TEXTURE_2D, ssaoTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTexture, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	// SSAO horizontal blur buffer
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurTextureBufferH);

	glBindTexture(GL_TEXTURE_2D, auxBlurTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, auxBlurTexture, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	// SSAO vertical blur buffer
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurTextureBufferV);

	glBindTexture(GL_TEXTURE_2D, ssaoTexture);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTexture, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	// Render buffer
	glBindFramebuffer(GL_FRAMEBUFFER, renderPassBuffer);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, depthsMSTexture);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depthsMSTexture, 0);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderTexture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaaSamples, GL_RGB16F, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), GL_TRUE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, renderTexture, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	// HDR buffers
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFramebuffer);

	glBindTexture(GL_TEXTURE_2D, colorTextures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTextures[0], 0);

	glBindTexture(GL_TEXTURE_2D, colorTextures[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, colorTextures[1], 0);

	unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
	glDrawBuffers(2, attachments);

	// Bloom blur buffers
	for (unsigned int i = 0; i < 2; i++) {
		glBindTexture(GL_TEXTURE_2D, bloomBlurTextures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	for (unsigned int i = 0; i < 2; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFramebuffers[i]);
		glBindTexture(GL_TEXTURE_2D, bloomBlurTextures[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomBlurTextures[i], gaussVeryLargeMipLevel);
	}

	for (unsigned int i = 0; i < 2; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFramebuffers[2 + i]);
		glBindTexture(GL_TEXTURE_2D, bloomBlurTextures[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomBlurTextures[i], gaussLargeMipLevel);
	}

	for (unsigned int i = 0; i < 2; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFramebuffers[4 + i]);
		glBindTexture(GL_TEXTURE_2D, bloomBlurTextures[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomBlurTextures[i], gaussMediumMipLevel);
	}

	for (unsigned int i = 0; i < 2; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFramebuffers[6 + i]);
		glBindTexture(GL_TEXTURE_2D, bloomBlurTextures[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomBlurTextures[i], gaussSmallMipLevel);
	}

	for (unsigned int i = 0; i < 2; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFramebuffers[8 + i]);
		glBindTexture(GL_TEXTURE_2D, bloomBlurTextures[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomBlurTextures[i], gaussVerySmallMipLevel);
	}

	for (unsigned int i = 0; i < 2; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFramebuffers[10 + i]);
		glBindTexture(GL_TEXTURE_2D, bloomBlurTextures[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomBlurTextures[i], 0);
	}

	// Bloom combine buffers
	glBindTexture(GL_TEXTURE_2D, bloomCombineTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindFramebuffer(GL_FRAMEBUFFER, bloomCombineFramebuffers[0]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomCombineTexture, gaussLargeMipLevel);

	glBindFramebuffer(GL_FRAMEBUFFER, bloomCombineFramebuffers[1]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomCombineTexture, gaussMediumMipLevel);

	glBindFramebuffer(GL_FRAMEBUFFER, bloomCombineFramebuffers[2]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomCombineTexture, gaussSmallMipLevel);

	glBindFramebuffer(GL_FRAMEBUFFER, bloomCombineFramebuffers[3]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomCombineTexture, gaussVerySmallMipLevel);

	glBindFramebuffer(GL_FRAMEBUFFER, bloomCombineFramebuffers[4]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomCombineTexture, 0);

	// Compute Gaussian kernels
	ComputeBloomGaussianKernel();

	// Color correction buffer
	glBindFramebuffer(GL_FRAMEBUFFER, colorCorrectionBuffer);
	glBindTexture(GL_TEXTURE_2D, outputTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOG("ERROR: Framebuffer is not complete!");
	}
}

void ModuleRender::ComputeBloomGaussianKernel() {
	gaussBloomKernelRadius = (int) roundf(viewportSize.y * 0.008f * bloomSizeMultiplier);
	float term = Ln(1e5f / sqrt(2 * pi));
	float sigma = gaussBloomKernelRadius * gaussBloomKernelRadius / 2.0f;
	sigma = sqrt(sigma / (term - Ln(sigma)));
	bloomGaussKernel.clear();
	gaussianKernel(2 * gaussBloomKernelRadius + 1, sigma, 0.f, 1.f, bloomGaussKernel);
}

void ModuleRender::SetVSync(bool vsync) {
	SDL_GL_SetSwapInterval(vsync);
}

void ModuleRender::ToggleDebugMode() {
	debugMode = !debugMode;
}

void ModuleRender::ToggleDebugDraw() {
	drawDebugDraw = !drawDebugDraw;
}

void ModuleRender::ToggleDrawQuadtree() {
	drawQuadtree = !drawQuadtree;
}

void ModuleRender::ToggleDrawBBoxes() {
	drawAllBoundingBoxes = !drawAllBoundingBoxes;
}

void ModuleRender::ToggleDrawSkybox() { // TODO: review Godmodecamera
	skyboxActive = !skyboxActive;
}

void ModuleRender::ToggleDrawAnimationBones() {
	drawAllBones = !drawAllBones;
}

void ModuleRender::ToggleDrawCameraFrustums() {
	drawCameraFrustums = !drawCameraFrustums;
}

void ModuleRender::ToggleDrawLightGizmos() {
	drawLightGizmos = !drawLightGizmos;
}

void ModuleRender::ToggleDrawParticleGizmos() {
	drawParticleGizmos = !drawParticleGizmos;
}

void ModuleRender::ToggleDrawLightFrustumGizmo() {
	drawLightFrustumGizmo = !drawLightFrustumGizmo;
}

void ModuleRender::UpdateShadingMode(const char* shadingMode) {
	drawDepthMapTexture = false;
	drawSSAOTexture = false;
	drawNormalsTexture = false;
	drawPositionsTexture = false;

	std::string strShadingMode = shadingMode;

	if (strcmp(shadingMode, "Shaded") == 0) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	} else if (strcmp(shadingMode, "Wireframe") == 0) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else if (strcmp(shadingMode, "Ambient Occlusion") == 0) {
		drawSSAOTexture = true;
	} else if (strcmp(shadingMode, "Normals") == 0) {
		drawNormalsTexture = true;
	} else if (strcmp(shadingMode, "Positions") == 0) {
		drawPositionsTexture = true;
	} else if (strShadingMode.find("StaticDepth") != std::string::npos) {
		for (unsigned int i = 0; i < NUM_CASCADES_FRUSTUM; ++i) {
			std::string str = "StaticDepth " + std::to_string(i);
			if (strcmp(shadingMode, str.c_str()) == 0) {
				drawDepthMapTexture = true;
				indexDepthMapTexture = i;
				shadowCasterType = ShadowCasterType::STATIC;
			}
		}
	} else if (strShadingMode.find("DynamicDepth") != std::string::npos) {
		for (unsigned int i = 0; i < NUM_CASCADES_FRUSTUM; ++i) {
			std::string str = "DynamicDepth " + std::to_string(i);
			if (strcmp(shadingMode, str.c_str()) == 0) {
				drawDepthMapTexture = true;
				indexDepthMapTexture = i;
				shadowCasterType = ShadowCasterType::DYNAMIC;
			}
		}
	}
}

float4x4 ModuleRender::GetLightViewMatrix(unsigned int i, ShadowCasterType lightFrustumType) const {
	return (lightFrustumType == ShadowCasterType::STATIC) ? lightFrustumStatic.GetOrthographicFrustum(i).ViewMatrix() : lightFrustumDynamic.GetOrthographicFrustum(i).ViewMatrix();
}

float4x4 ModuleRender::GetLightProjectionMatrix(unsigned int i, ShadowCasterType lightFrustumType) const {
	return (lightFrustumType == ShadowCasterType::STATIC) ? lightFrustumStatic.GetOrthographicFrustum(i).ProjectionMatrix() : lightFrustumDynamic.GetOrthographicFrustum(i).ProjectionMatrix();
}

std::vector<GameObject*> ModuleRender::GetDynamicCulledShadowCasters(const FrustumPlanes& planes) const {
	std::vector<GameObject*> meshes;

	for (GameObject* go : dynamicShadowCasters) {
		if (InsideFrustumPlanes(planes, go)) {
			meshes.push_back(go);
		}
	}

	return meshes;
}

std::vector<GameObject*> ModuleRender::GetCulledMeshes(const FrustumPlanes& planes, const int mask) {
	std::vector<GameObject*> meshes;

	for (ComponentMeshRenderer componentMR : App->scene->GetCurrentScene()->meshRendererComponents) {
		GameObject* go = &componentMR.GetOwner();

		Mask& maskGo = go->GetMask();

		if ((maskGo.bitMask & mask) != 0) {
			if (InsideFrustumPlanes(planes, go)) {
				meshes.push_back(go);
			}
		}
	}

	return meshes;
}

std::vector<GameObject*> ModuleRender::GetStaticCulledShadowCasters(const FrustumPlanes& planes) const {
	std::vector<GameObject*> meshes;

	for (GameObject* go : staticShadowCasters) {
		if (InsideFrustumPlanes(planes, go)) {
			meshes.push_back(go);
		}
	}

	return meshes;
}

int ModuleRender::GetCulledTriangles() const {
	return culledTriangles;
}

void ModuleRender::DrawQuadtreeRecursive(const Quadtree<GameObject>::Node& node, const AABB2D& aabb) {
	if (node.IsBranch()) {
		vec2d center = aabb.minPoint + (aabb.maxPoint - aabb.minPoint) * 0.5f;

		const Quadtree<GameObject>::Node& topLeft = node.childNodes->nodes[0];
		AABB2D topLeftAABB = {{aabb.minPoint.x, center.y}, {center.x, aabb.maxPoint.y}};
		DrawQuadtreeRecursive(topLeft, topLeftAABB);

		const Quadtree<GameObject>::Node& topRight = node.childNodes->nodes[1];
		AABB2D topRightAABB = {{center.x, center.y}, {aabb.maxPoint.x, aabb.maxPoint.y}};
		DrawQuadtreeRecursive(topRight, topRightAABB);

		const Quadtree<GameObject>::Node& bottomLeft = node.childNodes->nodes[2];
		AABB2D bottomLeftAABB = {{aabb.minPoint.x, aabb.minPoint.y}, {center.x, center.y}};
		DrawQuadtreeRecursive(bottomLeft, bottomLeftAABB);

		const Quadtree<GameObject>::Node& bottomRight = node.childNodes->nodes[3];
		AABB2D bottomRightAABB = {{center.x, aabb.minPoint.y}, {aabb.maxPoint.x, center.y}};
		DrawQuadtreeRecursive(bottomRight, bottomRightAABB);
	} else {
		float3 points[8] = {
			{aabb.minPoint.x, 0, aabb.minPoint.y},
			{aabb.maxPoint.x, 0, aabb.minPoint.y},
			{aabb.maxPoint.x, 0, aabb.maxPoint.y},
			{aabb.minPoint.x, 0, aabb.maxPoint.y},
			{aabb.minPoint.x, 30, aabb.minPoint.y},
			{aabb.maxPoint.x, 30, aabb.minPoint.y},
			{aabb.maxPoint.x, 30, aabb.maxPoint.y},
			{aabb.minPoint.x, 30, aabb.maxPoint.y},
		};
		dd::box(points, dd::colors::White);
	}
}

void ModuleRender::ClassifyGameObjectsFromQuadtree(const Quadtree<GameObject>::Node& node, const AABB2D& aabb) {
	AABB aabb3d = AABB({aabb.minPoint.x, -1000000.0f, aabb.minPoint.y}, {aabb.maxPoint.x, 1000000.0f, aabb.maxPoint.y});
	if (App->camera->GetFrustumPlanes().CheckIfInsideFrustumPlanes(aabb3d, OBB(aabb3d))) {
		if (node.IsBranch()) {
			vec2d center = aabb.minPoint + (aabb.maxPoint - aabb.minPoint) * 0.5f;

			const Quadtree<GameObject>::Node& topLeft = node.childNodes->nodes[0];
			AABB2D topLeftAABB = {{aabb.minPoint.x, center.y}, {center.x, aabb.maxPoint.y}};
			ClassifyGameObjectsFromQuadtree(topLeft, topLeftAABB);

			const Quadtree<GameObject>::Node& topRight = node.childNodes->nodes[1];
			AABB2D topRightAABB = {{center.x, center.y}, {aabb.maxPoint.x, aabb.maxPoint.y}};
			ClassifyGameObjectsFromQuadtree(topRight, topRightAABB);

			const Quadtree<GameObject>::Node& bottomLeft = node.childNodes->nodes[2];
			AABB2D bottomLeftAABB = {{aabb.minPoint.x, aabb.minPoint.y}, {center.x, center.y}};
			ClassifyGameObjectsFromQuadtree(bottomLeft, bottomLeftAABB);

			const Quadtree<GameObject>::Node& bottomRight = node.childNodes->nodes[3];
			AABB2D bottomRightAABB = {{center.x, aabb.minPoint.y}, {aabb.maxPoint.x, center.y}};
			ClassifyGameObjectsFromQuadtree(bottomRight, bottomRightAABB);
		} else {
			float3 cameraPos = App->camera->GetActiveCamera()->GetFrustum()->Pos();

			const Quadtree<GameObject>::Element* element = node.firstElement;
			while (element != nullptr) {
				GameObject* gameObject = element->object;
				if (!gameObject->flag) {
					ComponentBoundingBox* boundingBox = gameObject->GetComponent<ComponentBoundingBox>();
					const AABB& gameObjectAABB = boundingBox->GetWorldAABB();
					const OBB& gameObjectOBB = boundingBox->GetWorldOBB();

					if (App->camera->GetFrustumPlanes().CheckIfInsideFrustumPlanes(gameObjectAABB, gameObjectOBB)) {
						ComponentMeshRenderer* meshRenderer = gameObject->GetComponent<ComponentMeshRenderer>();
						if (meshRenderer != nullptr) {
							ResourceMaterial* material = App->resources->GetResource<ResourceMaterial>(meshRenderer->GetMaterialID());
							if (material != nullptr) {
								if (material->renderingMode == RenderingMode::OPAQUE) {
									opaqueGameObjects.push_back(gameObject);
								} else {
									ComponentTransform* transform = gameObject->GetComponent<ComponentTransform>();
									float dist = Length(cameraPos - transform->GetGlobalPosition());
									transparentGameObjects[dist] = gameObject;
								}

								if (material->castShadows) {
									if (material->shadowCasterType == ShadowCasterType::STATIC) {
										staticShadowCasters.push_back(gameObject);
									} else {
										dynamicShadowCasters.push_back(gameObject);
									}
								}
							}
						}
					}

					gameObject->flag = true;
				}
				element = element->next;
			}
		}
	}
}

void ModuleRender::DrawGameObject(GameObject* gameObject) {
	ComponentTransform* transform = gameObject->GetComponent<ComponentTransform>();
	ComponentView<ComponentMeshRenderer> meshes = gameObject->GetComponents<ComponentMeshRenderer>();
	ComponentBoundingBox* boundingBox = gameObject->GetComponent<ComponentBoundingBox>();

	if (boundingBox && drawAllBoundingBoxes && (App->camera->IsEngineCameraActive() || debugMode)) {
		boundingBox->DrawBoundingBox();
	}

	for (ComponentMeshRenderer& mesh : meshes) {
		mesh.Draw(transform->GetGlobalMatrix());

		ResourceMesh* resourceMesh = App->resources->GetResource<ResourceMesh>(mesh.GetMeshID());
		if (resourceMesh != nullptr) {
			culledTriangles += resourceMesh->indices.size() / 3;
		}
	}
}

void ModuleRender::DrawGameObjectDepthPrepass(GameObject* gameObject) {
	ComponentView<ComponentMeshRenderer> meshes = gameObject->GetComponents<ComponentMeshRenderer>();
	ComponentTransform* transform = gameObject->GetComponent<ComponentTransform>();
	assert(transform);

	for (ComponentMeshRenderer& mesh : meshes) {
		mesh.DrawDepthPrepass(transform->GetGlobalMatrix());
	}
}

void ModuleRender::DrawGameObjectShadowPass(GameObject* gameObject, unsigned int i, ShadowCasterType lightFrustumType) {
	ComponentView<ComponentMeshRenderer> meshes = gameObject->GetComponents<ComponentMeshRenderer>();
	ComponentTransform* transform = gameObject->GetComponent<ComponentTransform>();
	assert(transform);

	for (ComponentMeshRenderer& mesh : meshes) {
		mesh.DrawShadow(transform->GetGlobalMatrix(), i, lightFrustumType);
	}
}

void ModuleRender::RenderUI() {
	if (App->userInterface->IsUsing2D()) {
		SetOrtographicRender();
		App->camera->EnableOrtographic();
	}

	glDisable(GL_DEPTH_TEST); // In order to not clip with Models
	App->userInterface->Render();
	glEnable(GL_DEPTH_TEST);

	if (App->userInterface->IsUsing2D()) {
		App->camera->EnablePerspective();
		SetPerspectiveRender();
	}
}

void ModuleRender::SetOrtographicRender() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, viewportSize.x, viewportSize.y, 0, 1, -1);
	glMatrixMode(GL_MODELVIEW);
}

void ModuleRender::SetPerspectiveRender() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1, 1, -1, 1, -1, 1);
	glMatrixMode(GL_MODELVIEW);
}

bool ModuleRender::InsideFrustumPlanes(const FrustumPlanes& planes, const GameObject* go) const {
	ComponentBoundingBox* boundingBox = go->GetComponent<ComponentBoundingBox>();
	if (boundingBox && planes.CheckIfInsideFrustumPlanes(boundingBox->GetWorldAABB(), boundingBox->GetWorldOBB())) {
		return true;
	}
	return false;
}

const float2 ModuleRender::GetViewportSize() {
	return viewportSize;
}

bool ModuleRender::ObjectInsideFrustum(GameObject* gameObject) {
	ComponentBoundingBox* boundingBox = gameObject->GetComponent<ComponentBoundingBox>();
	if (boundingBox) {
		return App->camera->GetFrustumPlanes().CheckIfInsideFrustumPlanes(boundingBox->GetWorldAABB(), boundingBox->GetWorldOBB());
	}
	return false;
}

void ModuleRender::DrawAnimation(const GameObject* gameObject, bool hasAnimation) {
	for (const GameObject* childen : gameObject->GetChildren()) {
		ComponentTransform* transform = childen->GetComponent<ComponentTransform>();

		dd::point(transform->GetGlobalPosition(), dd::colors::Red, 5);
		dd::line(gameObject->GetComponent<ComponentTransform>()->GetGlobalPosition(), transform->GetGlobalPosition(), dd::colors::Cyan, 0, false);

		DrawAnimation(childen, true);
	}
}