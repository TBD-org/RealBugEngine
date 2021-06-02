#include "LightFrustum.h"

#include "Globals.h"
#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleRender.h"

#include "debugdraw.h"
#include "Geometry/Plane.h"
#include "Math/float3x3.h"

#include "Utils/Leaks.h"

LightFrustum::LightFrustum() {
	frustum.SetKind(FrustumSpaceGL, FrustumRightHanded);
}

void LightFrustum::ReconstructFrustum() {
	if (!dirty) return;

	GameObject* light = App->scene->scene->directionalLight;
	if (!light) return;

	ComponentTransform* transform = light->GetComponent<ComponentTransform>();
	assert(transform);

	float4x4 lightOrientation = transform->GetGlobalMatrix();
	lightOrientation.SetTranslatePart(float3::zero);

	AABB lightAABB;
	lightAABB.SetNegativeInfinity();

	for (GameObject* go : App->renderer->shadowGameObjects) {
		Mask mask = go->GetMask();
		if ((mask.bitMask & static_cast<int>(MaskType::CAST_SHADOWS)) != 0 && go->HasComponent<ComponentMeshRenderer>()) {
			ComponentBoundingBox* componentBBox = go->GetComponent<ComponentBoundingBox>();
			if (componentBBox) {
				AABB boundingBox = componentBBox->GetWorldAABB();
				OBB orientedBoundingBox = boundingBox.Transform(lightOrientation.Inverted());
				lightAABB.Enclose(orientedBoundingBox.MinimalEnclosingAABB());
			}
		}
	}

	float3 minPoint = lightAABB.minPoint;
	float3 maxPoint = lightAABB.maxPoint;
	float3 position = lightOrientation.RotatePart() * float3((maxPoint.x + minPoint.x) * 0.5f, ((maxPoint.y + minPoint.y) * 0.5f), minPoint.z);

	frustum.SetOrthographic((maxPoint.x - minPoint.x), (maxPoint.y - minPoint.y));
	frustum.SetUp(transform->GetUp());
	frustum.SetFront(transform->GetFront());
	frustum.SetPos(position);
	frustum.SetViewPlaneDistances(0.0f, (maxPoint.z - minPoint.z));

	dirty = true;
}

void LightFrustum::DrawGizmos() {
	dd::frustum(frustum.ViewProjMatrix().Inverted(), dd::colors::Green);
}

Frustum LightFrustum::GetFrustum() const {
	return frustum;
}

void LightFrustum::Invalidate() {
	dirty = true;
}
