#pragma once

#include "Module.h"
#include "Utils/Quadtree.h"
#include "Rendering/LightFrustum.h"

#include "MathGeoLibFwd.h"
#include "Math/float3.h"

#include <map>

#define SSAO_KERNEL_SIZE 64
#define RANDOM_TANGENTS_ROWS 4
#define RANDOM_TANGENTS_COLS 4

class GameObject;

enum class TESSERACT_ENGINE_API MSAA_SAMPLES_TYPE {
	MSAA_X2,
	MSAA_X4,
	MSAA_X8,
	COUNT
};

class ModuleRender : public Module {
public:
	// ------- Core Functions ------ //
	bool Init() override;

	bool Start() override;
	UpdateStatus PreUpdate() override;
	UpdateStatus Update() override;
	UpdateStatus PostUpdate() override;
	bool CleanUp() override;
	void ReceiveEvent(TesseractEvent& ev) override;

	void ViewportResized(int width, int height); // Updates the viewport aspect ratio with the new one given by parameters. It will set 'viewportUpdated' to true, to regenerate the framebuffer to its new size using UpdateFramebuffers().
	void UpdateFramebuffers();					 // Generates the rendering framebuffer on Init(). If 'viewportUpdated' was set to true, it will be also called at PostUpdate().
	void ComputeBloomGaussianKernel();

	void SetVSync(bool vsync);

	// -------- Game Debug --------- //
	void ToggleDebugMode();
	void ToggleDebugDraw();
	void ToggleDrawQuadtree();
	void ToggleDrawBBoxes();
	void ToggleDrawSkybox(); // TODO: review Godmodecamera
	void ToggleDrawAnimationBones();
	void ToggleDrawCameraFrustums();
	void ToggleDrawLightGizmos();
	void ToggleDrawLightFrustumGizmo();
	void ToggleDrawParticleGizmos();

	void UpdateShadingMode(const char* shadingMode);

	float4x4 GetLightViewMatrix(unsigned int i, ShadowCasterType lightFrustumType) const;
	float4x4 GetLightProjectionMatrix(unsigned int i, ShadowCasterType lightFrustumType) const;

	std::vector<GameObject*> GetCulledMeshes(const FrustumPlanes& planes, const int mask);
	std::vector<GameObject*> GetStaticCulledShadowCasters(const FrustumPlanes& planes) const;
	std::vector<GameObject*> GetDynamicCulledShadowCasters(const FrustumPlanes& planes) const;

	int GetCulledTriangles() const;
	const float2 GetViewportSize();

	bool ObjectInsideFrustum(GameObject* gameObject);

public:
	void* context = nullptr; // SDL context.

	// - Render Buffer GL pointers - //
	unsigned cubeVAO = 0;
	unsigned cubeVBO = 0;

	unsigned renderTexture = 0;
	unsigned outputTexture = 0;
	unsigned depthsMSTexture = 0;
	unsigned positionsMSTexture = 0;
	unsigned normalsMSTexture = 0;
	unsigned depthsTexture = 0;
	unsigned positionsTexture = 0;
	unsigned normalsTexture = 0;
	unsigned depthMapStaticTextures[NUM_CASCADES_FRUSTUM] = {0, 0, 0, 0};
	unsigned depthMapDynamicTextures[NUM_CASCADES_FRUSTUM] = {0, 0, 0, 0};
	unsigned ssaoTexture = 0;
	unsigned auxBlurTexture = 0;
	unsigned colorTextures[2] = {0, 0}; // position 0: scene render texture; position 1: bloom texture to be blurred
	unsigned bloomBlurTextures[2] = {0, 0};
	unsigned bloomCombineTexture = 0;

	unsigned renderPassBuffer = 0;
	unsigned depthPrepassBuffer = 0;
	unsigned depthPrepassTextureConversionBuffer = 0;
	unsigned depthMapStaticTextureBuffers[NUM_CASCADES_FRUSTUM] = {0, 0, 0, 0};
	unsigned depthMapDynamicTextureBuffers[NUM_CASCADES_FRUSTUM] = {0, 0, 0, 0};
	unsigned ssaoTextureBuffer = 0;
	unsigned ssaoBlurTextureBufferH = 0;
	unsigned ssaoBlurTextureBufferV = 0;
	unsigned colorCorrectionBuffer = 0;
	unsigned hdrFramebuffer = 0;
	unsigned bloomBlurFramebuffers[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Ping-pong buffers to blur bloom horizontally and vertically
	unsigned bloomCombineFramebuffers[5] = {0, 0, 0, 0, 0};

	// ------- Viewport Updated ------- //
	bool viewportUpdated = true;

	// -- Debugging Tools Toggles -- //
	bool debugMode = false; // Flag to activate DrawOptions only ingame (not use in the engine)
	bool drawDebugDraw = true;
	bool drawQuadtree = true;
	bool drawAllBoundingBoxes = false;
	bool skyboxActive = true; // TODO: review Godmodecamera
	bool drawAllBones = false;
	bool drawCameraFrustums = false;
	bool drawLightGizmos = false;
	bool drawLightFrustumGizmo = false;
	bool drawNavMesh = false;
	bool drawParticleGizmos = false;
	bool drawColliders = false;
	int culledTriangles = 0;

	float3 ambientColor = {0.25f, 0.25f, 0.25f}; // Color of ambient Light
	float3 clearColor = {0.1f, 0.1f, 0.1f};		 // Color of the viewport between frames

	// SSAO
	bool ssaoActive = true;
	float ssaoRange = 1.0f;
	float ssaoBias = 0.0f;
	float ssaoPower = 3.0f;
	float ssaoDirectLightingStrength = 0.5f;

	// Bloom
	bool bloomActive = true;
	int gaussSSAOKernelRadius = 0;
	int gaussBloomKernelRadius = 0;

	int gaussVerySmallMipLevel = 1;
	int gaussSmallMipLevel = 2;
	int gaussMediumMipLevel = 3;
	int gaussLargeMipLevel = 4;
	int gaussVeryLargeMipLevel = 5;

	float bloomIntensity = 1.0f;
	float bloomThreshold = 1.0f;
	float bloomSizeMultiplier = 1.0f;
	float bloomVerySmallWeight = 0.5f;
	float bloomSmallWeight = 0.5f;
	float bloomMediumWeight = 0.5f;
	float bloomLargeWeight = 0.5f;
	float bloomVeryLargeWeight = 0.5f;

	bool msaaActive = true;
	MSAA_SAMPLES_TYPE msaaSampleType = MSAA_SAMPLES_TYPE::MSAA_X4;
	int msaaSamplesNumber[static_cast<int>(MSAA_SAMPLES_TYPE::COUNT)] = {2, 4, 8};
	int msaaSampleSingle = 1;

	bool chromaticAberrationActive = false;
	float chromaticAberrationStrength = 1.0f;

	LightFrustum lightFrustumStatic;
	LightFrustum lightFrustumDynamic;

private:
	void DrawQuadtreeRecursive(const Quadtree<GameObject>::Node& node, const AABB2D& aabb);			  // Draws the quadrtee nodes if 'drawQuadtree' is set to true.
	void ClassifyGameObjects();																		  // Classify Game Objects from Scene taking into account Frustum Culling, Shadows and Rendering Mode
	void ClassifyGameObjectsFromQuadtree(const Quadtree<GameObject>::Node& node, const AABB2D& aabb); // Classify Game Objects from Scene taking into account Frustum Culling, Quadtree, Shadows and Rendering Mode
	void DrawGameObject(GameObject* gameObject);													  // ??
	void DrawGameObjectDepthPrepass(GameObject* gameObject);
	void DrawGameObjectShadowPass(GameObject* gameObject, unsigned int i, ShadowCasterType lightFrustumType);
	void DrawAnimation(const GameObject* gameObject, bool hasAnimation = false);
	void RenderUI();
	void SetOrtographicRender();
	void SetPerspectiveRender();

	bool InsideFrustumPlanes(const FrustumPlanes& planes, const GameObject* go) const;

	void ConvertDepthPrepassTextures();
	void ComputeSSAOTexture();
	void BlurSSAOTexture(bool horizontal);
	void BlurBloomTexture(unsigned bloomTexture, bool horizontal, const std::vector<float>& kernel, int kernelRadius, int textureLevel);
	void BloomCombine(unsigned bloomTexture, int bloomTextureLevel, int brightTextureLevel, float bloomWeight);

	void ExecuteColorCorrection();

	void DrawTexture(unsigned texture);
	void DrawScene();

private:
	// ------- Viewport Size ------- //
	float2 viewportSize = float2::zero;
	unsigned int indexDepthMapTexture = UINT_MAX;
	ShadowCasterType shadowCasterType;
	bool drawDepthMapTexture = false;
	bool drawSSAOTexture = false;
	bool drawNormalsTexture = false;
	bool drawPositionsTexture = false;

	std::vector<GameObject*> opaqueGameObjects;			 // Vector of Opaque GameObjects
	std::map<float, GameObject*> transparentGameObjects; // Map with Transparent GameObjects

	std::vector<GameObject*> staticShadowCasters;
	std::vector<GameObject*> dynamicShadowCasters;

	// ------- Kernels ------- //
	std::vector<float> ssaoGaussKernel;
	std::vector<float> bloomGaussKernel;

	float3 ssaoKernel[SSAO_KERNEL_SIZE];
	float3 randomTangents[RANDOM_TANGENTS_ROWS * RANDOM_TANGENTS_COLS];
};
