#include "LightFrustum.h"

#include "Globals.h"
#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleCamera.h"
#include "Utils/Random.h"

#include "debugdraw.h"
#include "Geometry/Plane.h"
#include "Math/float3x3.h"

#include "Utils/Leaks.h"

const float3 colors[MAX_NUMBER_OF_CASCADES] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}};

LightFrustum::LightFrustum() {

	subFrustums.resize(MAX_NUMBER_OF_CASCADES);

	for (unsigned i = 0; i < numberOfCascades; i++) {
		subFrustums[i].orthographicFrustum.SetKind(FrustumSpaceGL, FrustumRightHanded);
		subFrustums[i].perspectiveFrustum.SetKind(FrustumSpaceGL, FrustumRightHanded);
		subFrustums[i].color = colors[i];
		subFrustums[i].multiplier = 1.0f;
	}
}

void LightFrustum::UpdateFrustums() {

	ComponentCamera* gameCamera = App->camera->GetGameCamera();
	if (!gameCamera) return;

	Frustum *gameFrustum = gameCamera->GetFrustum();

	float farDistance = MINIMUM_FAR_DISTANCE; //*0.3f;

	if (mode == CascadeMode::FitToScene) {
		
		for (unsigned int i = 0; i < numberOfCascades; i++, farDistance *= 2.f) {
			subFrustums[i].perspectiveFrustum.SetKind(FrustumSpaceGL, FrustumRightHanded);
			subFrustums[i].perspectiveFrustum.SetHorizontalFovAndAspectRatio(gameFrustum->HorizontalFov(), gameFrustum->AspectRatio());
			subFrustums[i].perspectiveFrustum.SetPos(gameFrustum->Pos());
			subFrustums[i].perspectiveFrustum.SetUp(gameFrustum->Up());
			subFrustums[i].perspectiveFrustum.SetFront(gameFrustum->Front());
			subFrustums[i].perspectiveFrustum.SetViewPlaneDistances(gameFrustum->NearPlaneDistance(), farDistance);
			subFrustums[i].planes.CalculateFrustumPlanes(subFrustums[i].perspectiveFrustum);
		}

	} else {

		float nearDistance = 0.0f;

		for (unsigned int i = 0; i < numberOfCascades; i++, nearDistance = farDistance, farDistance *= 2.f) {
			subFrustums[i].perspectiveFrustum.SetKind(FrustumSpaceGL, FrustumRightHanded);
			subFrustums[i].perspectiveFrustum.SetHorizontalFovAndAspectRatio(gameFrustum->HorizontalFov(), gameFrustum->AspectRatio());
			subFrustums[i].perspectiveFrustum.SetPos(gameFrustum->Pos());
			subFrustums[i].perspectiveFrustum.SetUp(gameFrustum->Up());
			subFrustums[i].perspectiveFrustum.SetFront(gameFrustum->Front());
			subFrustums[i].perspectiveFrustum.SetViewPlaneDistances(nearDistance, farDistance);
			subFrustums[i].planes.CalculateFrustumPlanes(subFrustums[i].perspectiveFrustum);
		}
	}

}

void LightFrustum::ReconstructFrustum(ShadowCasterType shadowCasterType) {
	if (!dirty) return;

	UpdateFrustums();

	GameObject* light = App->scene->scene->directionalLight;
	if (!light) return;

	ComponentTransform* transform = light->GetComponent<ComponentTransform>();
	assert(transform);

	for (unsigned int i = 0; i < numberOfCascades; i++) {
		float4x4 lightOrientation = transform->GetGlobalMatrix();
		lightOrientation.SetTranslatePart(float3::zero);
		
		AABB lightAABB;
		lightAABB.SetNegativeInfinity();

		std::vector<GameObject*> gameObjects = (shadowCasterType == ShadowCasterType::STATIC) ? App->scene->scene->GetStaticCulledShadowCasters(subFrustums[i].planes) : App->scene->scene->GetDynamicCulledShadowCasters(subFrustums[i].planes);

		for (GameObject* go : gameObjects) {
			ComponentBoundingBox* componentBBox = go->GetComponent<ComponentBoundingBox>();
			if (componentBBox) {
				AABB boundingBox = componentBBox->GetWorldAABB();
				OBB orientedBoundingBox = boundingBox.Transform(lightOrientation.Inverted());
				lightAABB.Enclose(orientedBoundingBox.MinimalEnclosingAABB());
			}
		}

		float3 minPoint = lightAABB.minPoint;
		float3 maxPoint = lightAABB.maxPoint;
		float3 position = lightOrientation.RotatePart() * float3((maxPoint.x + minPoint.x) * 0.5f, ((maxPoint.y + minPoint.y) * 0.5f), minPoint.z);

		subFrustums[i].orthographicFrustum.SetOrthographic((maxPoint.x - minPoint.x), (maxPoint.y - minPoint.y));
		subFrustums[i].orthographicFrustum.SetUp(transform->GetUp());
		subFrustums[i].orthographicFrustum.SetFront(transform->GetFront());
		subFrustums[i].orthographicFrustum.SetPos(position);
		subFrustums[i].orthographicFrustum.SetViewPlaneDistances(0.0f, (maxPoint.z - minPoint.z));
	}

	dirty = false;
}

void LightFrustum::ConfigureFrustums(unsigned int value) {
	for (unsigned i = 0; i < numberOfCascades; i++) {
		subFrustums[i].orthographicFrustum.SetKind(FrustumSpaceGL, FrustumRightHanded);
		subFrustums[i].perspectiveFrustum.SetKind(FrustumSpaceGL, FrustumRightHanded);
		subFrustums[i].color = colors[i];
		subFrustums[i].multiplier = 1.0f;
	}
}

void LightFrustum::DrawOrthographicGizmos(unsigned int idx) {
	if (idx == INT_MAX) {
		for (unsigned i = 0; i < numberOfCascades; i++) {
			dd::frustum(subFrustums[i].orthographicFrustum.ViewProjMatrix().Inverted(), subFrustums[i].color);
		}
	} else if (idx >= 0 && idx < numberOfCascades) {
		dd::frustum(subFrustums[idx].orthographicFrustum.ViewProjMatrix().Inverted(), subFrustums[idx].color);
	}
}

void LightFrustum::DrawPerspectiveGizmos(unsigned int idx) {
	if (idx == INT_MAX) {
		for (unsigned i = 0; i < numberOfCascades; i++) {
			dd::frustum(subFrustums[i].perspectiveFrustum.ViewProjMatrix().Inverted(), subFrustums[i].color);
		}
	} else if (idx >= 0 && idx < numberOfCascades) {
		dd::frustum(subFrustums[idx].perspectiveFrustum.ViewProjMatrix().Inverted(), subFrustums[idx].color);
	}
}

void LightFrustum::SetNumberOfCascades(unsigned int value) {
	if (value <= 0 || value > MAX_NUMBER_OF_CASCADES) return;
	numberOfCascades = value;
}

unsigned int LightFrustum::GetNumberOfCascades() {
	return numberOfCascades;
}

Frustum LightFrustum::GetOrthographicFrustum(unsigned int i) const {
	return subFrustums[i].orthographicFrustum;
}

Frustum LightFrustum::GetPersepectiveFrustum(unsigned int i) const {
	return subFrustums[i].perspectiveFrustum;
}

const std::vector<LightFrustum::FrustumInformation>& LightFrustum::GetSubFrustums() const {
	return subFrustums;
}

LightFrustum::FrustumInformation& LightFrustum::operator[](unsigned int i) {
	
	assert(i < 0 || i > numberOfCascades && "Out of range");

	return subFrustums[i];

}

void LightFrustum::Invalidate() {
	dirty = true;
}
