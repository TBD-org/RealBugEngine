#pragma once

#include "Module.h"
#include "Components/ComponentCamera.h"
#include "Utils/Quadtree.h"

#include "MathGeoLibFwd.h"
#include "Math/float4x4.h"
#include "Math/float3.h"
#include "Geometry/Plane.h"
#include "Geometry/Frustum.h"
#include <vector>

class Model;
class GameObject;
//class ComponentCamera;

struct FrustumPlanes {
	float3 points[8]; // 0: ftl, 1: ftr, 2: fbl, 3: fbr, 4: ntl, 5: ntr, 6: nbl, 7: nbr. (far/near, top/bottom, left/right).
	Plane planes[6];  // left, right, up, down, front, back
};

class ModuleCamera : public Module {
public:
	// ------- Core Functions ------ //
	bool Start() override;
	UpdateStatus Update() override;

	void CalculateFrustumNearestObject(float2 pos); // Mouse Picking function. Detects the nearest object to the camera and selects it on a mouse left click. 'pos' is the x,y coordinates of the clicked pixel on the viewport.
	void CalculateFrustumPlanes();					// Calculates the geometry of the 'planes' and 'points' that define the frustum, from the 'cullingFrustum' properties.
	bool IsEngineCameraActive() const;

	// ------ Camera Movement ------ //
	void Translate(const vec& translation);																	// Move the frustum origin to the specified world position.
	void Zoom(float amount);																				// Modifies the 'focusDistance'. This variable modifies the frustum.Front() vector length, to zoom in and out.
	void Rotate(const float3x3& rotationMatrix);															// Modifies the frustum.Front() and frustum.Up() directions, to rotate the camera.
	void LookAt(float x, float y, float z);																	// Rotates the camera to look at a specified point.
	void Focus(const GameObject* gameObject);																// Mixes LookAt() and Translate() to place the camera near, and looking at the selected GameObject.
	void CalculateExtremePointsRecursive(const GameObject* gameObject, float3& minPoint, float3& maxPoint); // Subfunction of Focus(). Calculates the max and min corners of the box containing all the children in the hirerachy of gameObject.

	// ---------- Setters ---------- //
	void ViewportResized(int width, int height); // Called when the viewport panel changes size.
	void SetFOV(float hFov);
	void SetAspectRatio(float aspectRatio);
	void SetPlaneDistances(float nearPlane, float farPlane);
	void SetPosition(const vec& position);
	void SetPosition(float x, float y, float z);
	void SetOrientation(const float3x3& rotationMatrix);
	void ChangeActiveCamera(ComponentCamera* camera, bool change);	// Called from the Inspector, on a ComponentCamera. Changes the Engine camera to that Component if 'change'=true, and back to the default camera if false.
	void ChangeCullingCamera(ComponentCamera* camera, bool change); // Called from the Inspector, on a ComponentCamera. Changes the camera that will perform the frustum culling.
	void ChangeGameCamera(ComponentCamera* camera, bool change);

	// ---------- Getters ---------- //
	vec GetFront() const;
	vec GetUp() const;
	vec GetWorldRight() const;
	vec GetPosition() const;
	float3 GetOrientation() const;
	float GetFocusDistance() const;
	float GetNearPlane() const;
	float GetFarPlane() const;
	float GetFOV() const;
	float GetAspectRatio() const;
	float4x4 GetProjectionMatrix() const;
	float4x4 GetViewMatrix() const;
	ComponentCamera* GetEngineCamera();
	ComponentCamera* GetActiveCamera() const;
	ComponentCamera* GetCullingCamera() const;
	ComponentCamera* GetGameCamera() const;
	const FrustumPlanes& GetFrustumPlanes() const;
	void EnableOrtographic();
	void EnablePerspective();

public:
	float movementSpeed = 0.4f;				 // Deltatime multiplier for the camera movement speed.
	float rotationSpeed = 0.2f;				 // Deltatime multiplier for the camera rotation speed.
	float zoomSpeed = 0.001f;				 // Deltatime multiplier for the camera zooming speed.
	float shiftMultiplier = 5.0f;			 // Multiplier of the previows three, applied when the SHIFT key is pressed.
	ComponentCamera engineCamera = ComponentCamera(nullptr, GenerateUID(), true); // Default Engine camera.

private:
	void GetIntersectingAABBRecursive(const Quadtree<GameObject>::Node& node, const AABB2D& nodeAABB, const LineSegment& ray, std::vector<GameObject*>& intersectingObjects); // Subfunction of CalculateFrustumNearestObject(). Checks the Quatree for the GameObjects which BoundingBox intersect with the click (ray).

private:
	float focusDistance = 0.0f;					   // Defines the distance the camera is placed on Focus(), and to what point the camera orbits on Orbit().
	ComponentCamera* activeCamera = nullptr;	   // The camera that the scene is rendered from. Any camera in the scene can be set as active.
	ComponentCamera* cullingCamera = nullptr;	   // The camera that is performing frustum culling. Can be different from 'activeCamera'.
	ComponentCamera* gameCamera = nullptr;		   // The camera that will be set as active when the game starts.
	FrustumPlanes frustumPlanes = FrustumPlanes(); // Geometry of the frustum
};
