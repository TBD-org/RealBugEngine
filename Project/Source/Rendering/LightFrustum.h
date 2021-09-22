#pragma once

#include "FrustumPlanes.h"

constexpr unsigned int NUM_CASCADES_FRUSTUM = 4;
constexpr float MINIMUM_FAR_DISTANE = 50.f;


enum class CascadeMode {
	FitToScene = 0,
	FitToCascade = 1
};

enum class ShadowCasterType {
	STATIC,
	DYNAMIC
};

class LightFrustum {
public:
	struct FrustumInformation {
		Frustum orthographicFrustum; // Light frustum
		Frustum perspectiveFrustum;	 // Camera frustum
		FrustumPlanes planes = FrustumPlanes();
		float3 color = float3(0.0f, 0.0f, 0.0f);
		float multiplier = 1.0f;
	};

	LightFrustum();
	~LightFrustum() {}

	void UpdateCameraFrustum();
	void UpdateLightFrustum(ShadowCasterType shadowCasterType);

	void DrawGizmos();

	Frustum GetOrthographicFrustum(unsigned int i) const;
	Frustum GetPersepectiveFrustum(unsigned int i) const;
	const std::vector<LightFrustum::FrustumInformation>& GetSubFrustums() const;

	FrustumInformation& operator[](int i);

private:
	CascadeMode mode = CascadeMode::FitToScene;
	std::vector<FrustumInformation> subFrustums;
};
