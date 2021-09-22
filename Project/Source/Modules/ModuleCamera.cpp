#include "ModuleCamera.h"

#include "Globals.h"
#include "Application.h"
#include "Utils/Logging.h"
#include "Utils/MSTimer.h"
#include "Utils/UID.h"
#include "GameObject.h"
#include "Components/ComponentType.h"
#include "Components/ComponentBoundingBox.h"
#include "Components/ComponentCamera.h"
#include "Components/ComponentMeshRenderer.h"
#include "Components/ComponentTransform.h"
#include "Resources/ResourceMesh.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleInput.h"
#include "Modules/ModuleWindow.h"
#include "Modules/ModuleRender.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleEvents.h"

#include "Math/float3.h"
#include "Math/float3x3.h"
#include "Math/float4x4.h"
#include "Geometry/Sphere.h"
#include "Geometry/LineSegment.h"
#include "Geometry/Triangle.h"
#include "SDL_mouse.h"
#include "SDL_scancode.h"
#include "SDL_video.h"
#include "Brofiler.h"
#include <vector>

#include "Utils/Leaks.h"

static void WarpMouseOnEdges() {
	const float2& mousePosition = App->input->GetMousePosition();

	SDL_DisplayMode displayMode;
	int displayIndex = SDL_GetWindowDisplayIndex(App->window->window);
	SDL_GetCurrentDisplayMode(displayIndex, &displayMode);
	SDL_Rect displayBounds;
	SDL_GetDisplayBounds(displayIndex, &displayBounds);
	int screenWidth = displayMode.w;
	int screenHeight = displayMode.h;

	if (mousePosition.x < displayBounds.x + 20) {
		App->input->WarpMouse(displayBounds.x + displayBounds.w - 22, (int) mousePosition.y);
	}
	if (mousePosition.y < displayBounds.y + 20) {
		App->input->WarpMouse((int) mousePosition.x, displayBounds.y + displayBounds.h - 22);
	}
	if (mousePosition.x > displayBounds.x + displayBounds.w - 20) {
		App->input->WarpMouse(displayBounds.x + 22, (int) mousePosition.y);
	}
	if (mousePosition.y > displayBounds.y + displayBounds.h - 20) {
		App->input->WarpMouse((int) mousePosition.x, displayBounds.y + 22);
	}
}

bool ModuleCamera::Start() {
	UID uid = GenerateUID();
	activeCamera = &engineCamera;
	cullingCamera = &engineCamera;
	gameCamera = &engineCamera;
	Frustum* activeFrustum = activeCamera->GetFrustum();
	activeFrustum->SetKind(FrustumSpaceGL, FrustumRightHanded);
	activeFrustum->SetViewPlaneDistances(0.1f, 2000.0f);
#if GAME
	float ar = (float) App->window->GetWidth() / (float) App->window->GetHeight();
	activeFrustum->SetHorizontalFovAndAspectRatio(DEGTORAD * 90.0f, ar);
#else
	activeFrustum->SetHorizontalFovAndAspectRatio(DEGTORAD * 90.0f, 1.3f);
#endif
	activeFrustum->SetFront(vec::unitZ);
	activeFrustum->SetUp(vec::unitY);

	SetPosition(vec(2, 3, -5));
	LookAt(0, 0, 0);
	App->events->AddObserverToEvent(TesseractEventType::SCREEN_RESIZED, this);

	return true;
}

UpdateStatus ModuleCamera::Update() {
	BROFILER_CATEGORY("ModuleCamera - Update", Profiler::Color::Blue)

	//Camera updates happen here to prevent rendering problems, the logic followed is:
	//1. ModuleProject updates first, modifying camera values
	//2. Module camera updates camera frustums, updating view and projection matrixes
	//3. Module renderer uses updated view and projection matrixes to correctly draw geometry and skyboxes

	if (activeCamera != &engineCamera) {
		activeCamera->UpdateFrustum();
		return UpdateStatus::CONTINUE;
	}

	Frustum* activeFrustum = activeCamera->GetFrustum();
	float deltaTime = App->time->GetRealTimeDeltaTime();
	const float2& mouseMotion = App->input->GetMouseMotion();

	// Increase zoom and movement speed with shift
	float finalMovementSpeed = movementSpeed;
	float finalZoomSpeed = zoomSpeed;
	if (App->input->GetKey(SDL_SCANCODE_LSHIFT) != KeyState::KS_IDLE || App->input->GetKey(SDL_SCANCODE_RSHIFT) != KeyState::KS_IDLE) {
		finalMovementSpeed *= shiftMultiplier;
		finalZoomSpeed *= shiftMultiplier;
	}

	// Zoom with mouse wheel
	float mouseWheelMotion = App->input->GetMouseWheelMotion();
	if (mouseWheelMotion < -FLT_EPSILON || mouseWheelMotion > FLT_EPSILON) {
		Zoom(mouseWheelMotion * 0.1f * focusDistance);
	}

	if (App->input->GetKey(SDL_SCANCODE_LALT) != KeyState::KS_IDLE) {
		if (App->input->GetMouseButton(SDL_BUTTON_LEFT) != KeyState::KS_IDLE) {
			WarpMouseOnEdges();

			// Orbit with alt + left mouse button
			vec oldFocus = activeFrustum->Pos() + activeFrustum->Front().Normalized() * focusDistance;
			Rotate(float3x3::RotateAxisAngle(activeFrustum->WorldRight().Normalized(), -mouseMotion.y * rotationSpeed * DEGTORAD));
			Rotate(float3x3::RotateY(-mouseMotion.x * rotationSpeed * DEGTORAD));
			vec newFocus = activeFrustum->Pos() + activeFrustum->Front().Normalized() * focusDistance;
			Translate(oldFocus - newFocus);
		} else if (App->input->GetMouseButton(SDL_BUTTON_RIGHT) != KeyState::KS_IDLE) {
			WarpMouseOnEdges();

			// Zoom with alt + right mouse button
			Zoom(mouseMotion.y * finalZoomSpeed * focusDistance);
		}
	} else if (App->input->GetMouseButton(SDL_BUTTON_RIGHT) != KeyState::KS_IDLE) {
		WarpMouseOnEdges();

		// Rotate with mouse motion
		Rotate(float3x3::RotateAxisAngle(activeFrustum->WorldRight().Normalized(), -mouseMotion.y * rotationSpeed * DEGTORAD));
		Rotate(float3x3::RotateY(-mouseMotion.x * rotationSpeed * DEGTORAD));

		// Move with WASD + QE
		if (App->input->GetKey(SDL_SCANCODE_Q) != KeyState::KS_IDLE) {
			Translate(activeFrustum->Up().Normalized() * -finalMovementSpeed * focusDistance * deltaTime);
		}
		if (App->input->GetKey(SDL_SCANCODE_E) != KeyState::KS_IDLE) {
			Translate(activeFrustum->Up().Normalized() * finalMovementSpeed * focusDistance * deltaTime);
		}
		if (App->input->GetKey(SDL_SCANCODE_W) != KeyState::KS_IDLE) {
			Translate(activeFrustum->Front().Normalized() * finalMovementSpeed * focusDistance * deltaTime);
		}
		if (App->input->GetKey(SDL_SCANCODE_S) != KeyState::KS_IDLE) {
			Translate(activeFrustum->Front().Normalized() * -finalMovementSpeed * focusDistance * deltaTime);
		}
		if (App->input->GetKey(SDL_SCANCODE_A) != KeyState::KS_IDLE) {
			Translate(activeFrustum->WorldRight().Normalized() * -finalMovementSpeed * focusDistance * deltaTime);
		}
		if (App->input->GetKey(SDL_SCANCODE_D) != KeyState::KS_IDLE) {
			Translate(activeFrustum->WorldRight().Normalized() * finalMovementSpeed * focusDistance * deltaTime);
		}
	} else {
		// Focus camera around geometry with f key
		if (App->input->GetKey(SDL_SCANCODE_F) != KeyState::KS_IDLE) {
			Focus(App->editor->selectedGameObject);
		}

		// Move with arrow keys
		if (App->input->GetKey(SDL_SCANCODE_UP) != KeyState::KS_IDLE) {
			Translate(activeFrustum->Front().Normalized() * finalMovementSpeed * focusDistance * deltaTime);
		}
		if (App->input->GetKey(SDL_SCANCODE_DOWN) != KeyState::KS_IDLE) {
			Translate(activeFrustum->Front().Normalized() * -finalMovementSpeed * focusDistance * deltaTime);
		}
		if (App->input->GetKey(SDL_SCANCODE_LEFT) != KeyState::KS_IDLE) {
			Translate(activeFrustum->WorldRight().Normalized() * -finalMovementSpeed * focusDistance * deltaTime);
		}
		if (App->input->GetKey(SDL_SCANCODE_RIGHT) != KeyState::KS_IDLE) {
			Translate(activeFrustum->WorldRight().Normalized() * finalMovementSpeed * focusDistance * deltaTime);
		}
	}

	return UpdateStatus::CONTINUE;
}

void ModuleCamera::ReceiveEvent(TesseractEvent& ev) {
	switch (ev.type) {
	case TesseractEventType::SCREEN_RESIZED: {
		int width = ev.Get<ViewportResizedStruct>().newWidth;
		int height = ev.Get<ViewportResizedStruct>().newHeight;
		ViewportResized(width, height);
		break;
	}
	default:
		break;
	}
}

void ModuleCamera::CalculateFrustumNearestObject(float2 pos) {
	MSTimer timer;
	timer.Start();

	if (activeCamera != &engineCamera) return;

	std::vector<GameObject*> intersectingObjects;
	LineSegment ray = engineCamera.GetFrustum()->UnProjectLineSegment(pos.x, pos.y);

	// Check with AABB
	Scene* scene = App->scene->GetCurrentScene();
	for (GameObject& gameObject : scene->gameObjects) {
		gameObject.flag = false;
		if (gameObject.isInQuadtree) continue;

		ComponentBoundingBox* boundingBox = gameObject.GetComponent<ComponentBoundingBox>();
		if (boundingBox == nullptr) continue;

		const AABB& gameObjectAABB = boundingBox->GetWorldAABB();
		if (ray.Intersects(gameObjectAABB)) {
			intersectingObjects.push_back(&gameObject);
		}
	}
	if (scene->quadtree.IsOperative()) {
		GetIntersectingAABBRecursive(scene->quadtree.root, scene->quadtree.bounds, ray, intersectingObjects);
	}

	GameObject* selectedGameObject = nullptr;
	float minDistance = inf;
	float distance = 0;
	for (GameObject* gameObject : intersectingObjects) {
		for (ComponentMeshRenderer& mesh : gameObject->GetComponents<ComponentMeshRenderer>()) {
			ResourceMesh* meshResource = App->resources->GetResource<ResourceMesh>(mesh.GetMeshID());
			if (meshResource == nullptr) continue;

			const float4x4& model = gameObject->GetComponent<ComponentTransform>()->GetGlobalMatrix();
			std::vector<Triangle> triangles = meshResource->ExtractTriangles(model);
			for (Triangle& triangle : triangles) {
				if (!ray.Intersects(triangle, &distance, NULL)) continue;
				if (distance >= minDistance) continue;

				selectedGameObject = gameObject;
				minDistance = distance;
			}
		}
	}

	if (selectedGameObject != nullptr) {
		App->editor->selectedGameObject = selectedGameObject;
	}

	LOG("Ray Tracing in %ums", timer.Stop());
}


void ModuleCamera::CalculateFrustumPlanes() {
	Frustum* cullingFrustum = cullingCamera->GetFrustum();
	frustumPlanes.CalculateFrustumPlanes(*cullingFrustum);
}

bool ModuleCamera::IsEngineCameraActive() const {
	if (activeCamera == &engineCamera) {
		return true;
	}
	return false;
}

void ModuleCamera::Translate(const vec& translation) {
	activeCamera->GetFrustum()->SetPos(activeCamera->GetFrustum()->Pos() + translation);
}

void ModuleCamera::Zoom(float amount) {
	Translate(activeCamera->GetFrustum()->Front().Normalized() * amount);
	focusDistance -= amount;
	if (focusDistance < 0.0f) {
		focusDistance = 0.0f;
	}
}

void ModuleCamera::Rotate(const float3x3& rotationMatrix) {
	Frustum* activeFrustum = activeCamera->GetFrustum();
	vec oldFront = activeFrustum->Front().Normalized();
	vec oldUp = activeFrustum->Up().Normalized();
	activeFrustum->SetFront(rotationMatrix * oldFront);
	activeFrustum->SetUp(rotationMatrix * oldUp);
}

void ModuleCamera::LookAt(float x, float y, float z) {
	Frustum* activeFrustum = activeCamera->GetFrustum();
	vec direction = vec(x, y, z) - activeFrustum->Pos();
	focusDistance = direction.Length();
	direction.Normalize();
	vec up = vec::unitY;

	// Special case for when looking straight up
	if (direction.Cross(up).IsZero()) {
		up = vec::unitZ;
	}

	Rotate(float3x3::LookAt(activeFrustum->Front().Normalized(), direction, activeFrustum->Up().Normalized(), up));
}

void ModuleCamera::Focus(const GameObject* gameObject) {
	if (!gameObject->GetComponent<ComponentTransform>()) return;
	if (gameObject == nullptr) {
		// Focus origin
		SetPosition(float3::zero - GetFront() * 30.f);
	} else {
		// Focus a GameObject
		if (gameObject->HasComponent<ComponentMeshRenderer>()) {
			// If the GO has Mesh, focus on that mesh
			ComponentBoundingBox* boundingBox = gameObject->GetComponent<ComponentBoundingBox>();
			if (!boundingBox) return;
			const AABB& worldBoundingBox = boundingBox->GetWorldAABB();
			if (!worldBoundingBox.IsFinite()) return;

			Sphere boundingSphere = worldBoundingBox.MinimalEnclosingSphere();
			float minHalfAngle = Min(activeCamera->GetFrustum()->HorizontalFov(), activeCamera->GetFrustum()->VerticalFov()) * 0.5f;
			float relativeDistance = boundingSphere.r / Sin(minHalfAngle);
			vec cameraDirection = -activeCamera->GetFrustum()->Front().Normalized();
			vec cameraPosition = boundingSphere.pos + (cameraDirection * relativeDistance);
			vec modelCenter = boundingSphere.pos;
			SetPosition(cameraPosition);
			LookAt(modelCenter.x, modelCenter.y, modelCenter.z);
		} else {
			// If it doesn't have Mesh, focus on its children's Meshes. (Looks down all the hierarchy)
			float3 modelCenter;
			float distance;
			if (gameObject->HasChildren()) {
				modelCenter = gameObject->GetComponent<ComponentTransform>()->GetGlobalPosition();
				float3 minPoint = float3::inf, maxPoint = -float3::inf;
				CalculateExtremePointsRecursive(gameObject, minPoint, maxPoint);
				if (minPoint.IsFinite() && maxPoint.IsFinite()) {
					distance = (maxPoint - minPoint).Length();
				} else {
					distance = 30.f;
				}
			} else {
				// But if it doesn't have children, simply return its position as center and the default distance
				modelCenter = gameObject->GetComponent<ComponentTransform>()->GetGlobalPosition();
				distance = 30.f;
			}
			SetPosition(modelCenter - GetFront() * distance);
			focusDistance = (modelCenter - activeCamera->GetFrustum()->Pos()).Length();
		}
	}
}

void ModuleCamera::CalculateExtremePointsRecursive(const GameObject* gameObject, float3& minPoint, float3& maxPoint) {
	for (GameObject* child : gameObject->GetChildren()) {
		if (child->HasComponent<ComponentMeshRenderer>()) {
			ComponentBoundingBox* childBoundingBox = child->GetComponent<ComponentBoundingBox>();
			if (childBoundingBox->GetWorldOBB().MinimalEnclosingAABB().IsFinite()) {
				minPoint = minPoint.Min(childBoundingBox->GetWorldAABB().minPoint);
				maxPoint = maxPoint.Max(childBoundingBox->GetWorldAABB().maxPoint);
			}
		}
		if (child->HasChildren()) CalculateExtremePointsRecursive(child, minPoint, maxPoint);
	}
}

void ModuleCamera::ViewportResized(int width, int height) {
	for (ComponentCamera& camera : App->scene->GetCurrentScene()->cameraComponents) {
		// TODO: Implement button to force AspectRatio from specific camera
		camera.frustum.SetVerticalFovAndAspectRatio(camera.frustum.VerticalFov(), width / (float) height);
	}
	engineCamera.GetFrustum()->SetVerticalFovAndAspectRatio(engineCamera.GetFrustum()->VerticalFov(), width / (float) height);
}

void ModuleCamera::SetFOV(float hFov) {
	activeCamera->GetFrustum()->SetHorizontalFovAndAspectRatio(hFov, activeCamera->GetFrustum()->AspectRatio());
}

void ModuleCamera::SetAspectRatio(float aspectRatio) {
	activeCamera->GetFrustum()->SetVerticalFovAndAspectRatio(activeCamera->GetFrustum()->VerticalFov(), aspectRatio);
}

void ModuleCamera::SetPlaneDistances(float nearPlane, float farPlane) {
	activeCamera->GetFrustum()->SetViewPlaneDistances(nearPlane, farPlane);
}

void ModuleCamera::SetPosition(const vec& position) {
	activeCamera->GetFrustum()->SetPos(position);
}

void ModuleCamera::SetPosition(float x, float y, float z) {
	activeCamera->GetFrustum()->SetPos(vec(x, y, z));
}

void ModuleCamera::SetOrientation(const float3x3& rotationMatrix) {
	activeCamera->GetFrustum()->SetFront(rotationMatrix * float3::unitZ);
	activeCamera->GetFrustum()->SetUp(rotationMatrix * float3::unitY);
}

void ModuleCamera::ChangeActiveCamera(ComponentCamera* camera, bool change) {
	if (change) {
		activeCamera = camera;
	} else {
		activeCamera = &engineCamera;
	}
}

void ModuleCamera::ChangeCullingCamera(ComponentCamera* camera, bool change) {
	if (change) {
		cullingCamera = camera;
	} else {
		cullingCamera = &engineCamera;
	}
}

void ModuleCamera::ChangeGameCamera(ComponentCamera* camera, bool change) {
	if (change) {
		gameCamera = camera;
	} else {
		gameCamera = &engineCamera;
	}
}

vec ModuleCamera::GetFront() const {
	return activeCamera->GetFrustum()->Front();
}

vec ModuleCamera::GetUp() const {
	return activeCamera->GetFrustum()->Up();
}

vec ModuleCamera::GetWorldRight() const {
	return activeCamera->GetFrustum()->WorldRight();
}

vec ModuleCamera::GetPosition() const {
	return activeCamera->GetFrustum()->Pos();
}

float3 ModuleCamera::GetOrientation() const {
	return activeCamera->GetFrustum()->ViewMatrix().RotatePart().ToEulerXYZ();
}

float ModuleCamera::GetFocusDistance() const {
	return focusDistance;
}

float ModuleCamera::GetNearPlane() const {
	return activeCamera->GetFrustum()->NearPlaneDistance();
}

float ModuleCamera::GetFarPlane() const {
	return activeCamera->GetFrustum()->FarPlaneDistance();
}

float ModuleCamera::GetFOV() const {
	return activeCamera->GetFrustum()->VerticalFov();
}

float ModuleCamera::GetAspectRatio() const {
	return activeCamera->GetFrustum()->AspectRatio();
}

float4x4 ModuleCamera::GetProjectionMatrix() const {
	return activeCamera->GetFrustum()->ProjectionMatrix();
}

float4x4 ModuleCamera::GetViewMatrix() const {
	return activeCamera->GetFrustum()->ViewMatrix();
}

ComponentCamera* ModuleCamera::GetEngineCamera() {
	return &engineCamera;
}

ComponentCamera* ModuleCamera::GetActiveCamera() const {
	return activeCamera;
}

ComponentCamera* ModuleCamera::GetCullingCamera() const {
	return cullingCamera;
}

ComponentCamera* ModuleCamera::GetGameCamera() const {
	return gameCamera;
}

const FrustumPlanes& ModuleCamera::GetFrustumPlanes() const {
	return frustumPlanes;
}

void ModuleCamera::EnableOrtographic() {
	activeCamera->GetFrustum()->SetOrthographic((float) App->renderer->GetViewportSize().x, (float) App->renderer->GetViewportSize().y);
}

void ModuleCamera::EnablePerspective() {
	activeCamera->GetFrustum()->SetPerspective(1.3f, 1.f);
	ViewportResized(static_cast<int>(App->renderer->GetViewportSize().x), static_cast<int>(App->renderer->GetViewportSize().y));
}

void ModuleCamera::GetIntersectingAABBRecursive(const Quadtree<GameObject>::Node& node, const AABB2D& nodeAABB, const LineSegment& ray, std::vector<GameObject*>& intersectingObjects) {
	AABB nodeAABB3d = AABB({nodeAABB.minPoint.x, -1000000.0f, nodeAABB.minPoint.y}, {nodeAABB.maxPoint.x, 1000000.0f, nodeAABB.maxPoint.y});
	if (ray.Intersects(nodeAABB3d)) {
		if (node.IsBranch()) {
			vec2d center = nodeAABB.minPoint + (nodeAABB.maxPoint - nodeAABB.minPoint) * 0.5f;

			const Quadtree<GameObject>::Node& topLeft = node.childNodes->nodes[0];
			AABB2D topLeftAABB = {{nodeAABB.minPoint.x, center.y}, {center.x, nodeAABB.maxPoint.y}};
			GetIntersectingAABBRecursive(topLeft, topLeftAABB, ray, intersectingObjects);

			const Quadtree<GameObject>::Node& topRight = node.childNodes->nodes[1];
			AABB2D topRightAABB = {{center.x, center.y}, {nodeAABB.maxPoint.x, nodeAABB.maxPoint.y}};
			GetIntersectingAABBRecursive(topRight, topRightAABB, ray, intersectingObjects);

			const Quadtree<GameObject>::Node& bottomLeft = node.childNodes->nodes[2];
			AABB2D bottomLeftAABB = {{nodeAABB.minPoint.x, nodeAABB.minPoint.y}, {center.x, center.y}};
			GetIntersectingAABBRecursive(bottomLeft, bottomLeftAABB, ray, intersectingObjects);

			const Quadtree<GameObject>::Node& bottomRight = node.childNodes->nodes[3];
			AABB2D bottomRightAABB = {{center.x, nodeAABB.minPoint.y}, {nodeAABB.maxPoint.x, center.y}};
			GetIntersectingAABBRecursive(bottomRight, bottomRightAABB, ray, intersectingObjects);
		} else {
			const Quadtree<GameObject>::Element* element = node.firstElement;
			while (element != nullptr) {
				GameObject* gameObject = element->object;
				if (!gameObject->flag) {
					ComponentBoundingBox* boundingBox = gameObject->GetComponent<ComponentBoundingBox>();
					const AABB& gameObjectAABB = boundingBox->GetWorldAABB();
					if (ray.Intersects(gameObjectAABB)) {
						intersectingObjects.push_back(gameObject);
					}

					gameObject->flag = true;
				}
				element = element->next;
			}
		}
	}
}
