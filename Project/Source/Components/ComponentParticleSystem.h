#pragma once
#include "Component.h"

#include "Utils/Pool.h"
#include "Utils/UID.h"
#include "Math/float4.h"
#include "Math/float2.h"
#include "Math/float4x4.h"
#include "Math/Quat.h"

#include <vector>
class ComponentTransform;
class ParticleModule;

enum class EmitterType {
	CONE,
	SPHERE,
	HEMISPHERE,
	DONUT,
	CIRCLE,
	RECTANGLE
};

class ComponentParticleSystem : public Component {
public:
	struct Particle {
		float4x4 model = float4x4::identity;

		float3 initialPosition = float3(0.0f, 0.0f, 0.0f);
		float3 position = float3(0.0f, 0.0f, 0.0f);
		float3 direction = float3(0.0f, 0.0f, 0.0f);
		float3 scale = float3(0.1f, 0.1f, 0.1f);

		Quat rotation = Quat(0.0f, 0.0f, 0.0f, 0.0f);

		float velocity = 0.0f;
		float life = 0.0F;
		float currentFrame = 0.0f;
		float colorFrame = 0.0f;
	};

	REGISTER_COMPONENT(ComponentParticleSystem, ComponentType::PARTICLE, false);

	void Update() override;
	void Init() override;
	void DrawGizmos() override;
	void OnEditorUpdate() override;
	void Load(JsonValue jComponent) override;
	void Save(JsonValue jComponent) const override;
	void Draw();
	TESSERACT_ENGINE_API void Play();
	TESSERACT_ENGINE_API void Stop();
	void DuplicateComponent(GameObject& owner) override;
	void SpawnParticle();
	void killParticles();
	float3 CreatePosition();
	float3 CreateVelocity();

private:
	const float4& GetTintColor() const; // Gets an additional color that needs to be applied to the image. Currently gets the color of the Button
	void CreateParticles(float nParticles, float vel);

private:
	UID textureID = 0; // ID of the image
	UID shaderID = 0;  // ID of the shader

	bool looping = false;
	bool isPlaying = true;
	bool alphaTransparency = false; // Enables Alpha Transparency of the image and the color
	bool isRandomFrame = false;
	bool randomDirection = false;

	float3 initC = float3::one;
	float3 finalC = float3::one;
	float4 color = float4::one; // Color used as default tainter

	float scale = 5;
	float maxParticles = 100;
	float velocity = 0.1;
	float kc = 1.0f; //Keep in one to avoid having denominator less than 1
	float kl = 0.045f;
	float kq = 0.0075f;
	float innerAngle = pi / 12;
	float outerAngle = pi / 6;
	float particleLife = 5;

	int particleSpawned = 0;
	int maxDistance = 2;
	int particleActivated = false;
	int Xtiles = 1;
	int Ytiles = 1;

private:
	EmitterType emitterType = EmitterType::CONE;

	Pool<Particle> particles;
	std::vector<Particle*> deadParticles;
	//TODO IMPLEMENT DRAWINSTANCE
	/*float3 particlesPosition[100];*/
};