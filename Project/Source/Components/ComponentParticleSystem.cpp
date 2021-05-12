#include "ComponentParticleSystem.h"

#include "GameObject.h"
#include "Components/ComponentTransform.h"
#include "Components/UI/ComponentButton.h"
#include "Application.h"
#include "Modules/ModulePrograms.h"
#include "Modules/ModuleCamera.h"
#include "Modules/ModuleRender.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleUserInterface.h"
#include "Panels/PanelScene.h"
#include "Resources/ResourceTexture.h"
#include "Resources/ResourceShader.h"
#include "FileSystem/TextureImporter.h"
#include "FileSystem/JsonValue.h"
#include "Math/float3x3.h"
#include "Utils/ImGuiUtils.h"
#include "Math/TransformOps.h"
#include "imgui.h"
#include "GL/glew.h"
#include "debugdraw.h"

#include "Utils/Leaks.h"

#define JSON_TAG_TEXTURE_SHADERID "ShaderId"
#define JSON_TAG_TEXTURE_TEXTUREID "TextureId"
#define JSON_TAG_COLOR "Color"
#define JSON_TAG_ALPHATRANSPARENCY "AlphaTransparency"

#define JSON_TAG_ISPLAYING "IsPlaying"
#define JSON_TAG_LOOPING "IsLooping"
#define JSON_TAG_ISRANDOMFRAME "IsRandomFrame"
#define JSON_TAG_ISRANDOMDIRECTION "IsRandomDirection"
#define JSON_TAG_SCALEPARTICLE "ParticleScale"
#define JSON_TAG_MAXPARTICLE "MaxParticle"
#define JSON_TAG_VELOCITY "Velocity"
#define JSON_TAG_LIFE "LifeParticle"
#define JSON_TAG_YTILES "Ytiles"
#define JSON_TAG_XTILES "Xtiles"
#define JSON_TAG_INITCOLOR "InitColor"
#define JSON_TAG_FINALCOLOR "FinalColor"

#include <random>

void ComponentParticleSystem::OnEditorUpdate() {
	if (ImGui::Checkbox("Active", &active)) {
		if (GetOwner().IsActive()) {
			if (active) {
				Enable();
			} else {
				Disable();
			}
		}
	}
	ImGui::Separator();

	ImGui::TextColored(App->editor->textColor, "Texture Settings:");

	ImGui::Checkbox("isPlaying: ", &isPlaying);
	ImGui::Checkbox("Loop: ", &looping);
	if (ImGui::Button("Play")) Play();
	if (ImGui::Button("Stop")) Stop();
	ImGui::Separator();
	const char* emitterTypeCombo[] = {"Cone", "Sphere", "Hemisphere", "Donut", "Circle", "Rectangle"};
	const char* emitterTypeComboCurrent = emitterTypeCombo[(int) emitterType];
	ImGui::TextColored(App->editor->textColor, "Shape:");
	if (ImGui::BeginCombo("##Shape", emitterTypeComboCurrent)) {
		for (int n = 0; n < IM_ARRAYSIZE(emitterTypeCombo); ++n) {
			bool isSelected = (emitterTypeComboCurrent == emitterTypeCombo[n]);
			if (ImGui::Selectable(emitterTypeCombo[n], isSelected)) {
				emitterType = (EmitterType) n;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Checkbox("Random Frame", &isRandomFrame);
	ImGui::Checkbox("Random Direction: ", &randomDirection);
	ImGui::ResourceSlot<ResourceShader>("shader", &shaderID);

	UID oldID = textureID;
	ImGui::ResourceSlot<ResourceTexture>("texture", &textureID);

	ResourceTexture* textureResource = App->resources->GetResource<ResourceTexture>(textureID);

	if (textureResource != nullptr) {
		int width;
		int height;
		glGetTextureLevelParameteriv(textureResource->glTexture, 0, GL_TEXTURE_WIDTH, &width);
		glGetTextureLevelParameteriv(textureResource->glTexture, 0, GL_TEXTURE_HEIGHT, &height);

		if (oldID != textureID) {
			ComponentTransform2D* transform2D = GetOwner().GetComponent<ComponentTransform2D>();
			if (transform2D != nullptr) {
				transform2D->SetSize(float2(static_cast<float>(width), static_cast<float>(height)));
			}
		}

		ImGui::Text("");
		ImGui::Separator();
		ImGui::TextColored(App->editor->titleColor, "Texture Preview");
		ImGui::TextWrapped("Size:");
		ImGui::SameLine();
		ImGui::TextWrapped("%i x %i", width, height);
		ImGui::Image((void*) textureResource->glTexture, ImVec2(200, 200));
		ImGui::InputScalar("Xtiles: ", ImGuiDataType_U32, &Xtiles);
		ImGui::InputScalar("Ytiles: ", ImGuiDataType_U32, &Ytiles);
		ImGui::InputFloat("Scale: ", &scale);
		ImGui::InputFloat("Life: ", &particleLife);

		if (ImGui::InputFloat("Speed: ", &velocity)) {
			CreateParticles(maxParticles, velocity);
		}

		if (ImGui::InputScalar("MaxParticles: ", ImGuiDataType_U32, &maxParticles)) {
			CreateParticles(maxParticles, velocity);
		}

		ImGui::ColorEdit3("InitColor##", initC.ptr());
		ImGui::ColorEdit3("FinalColor##", finalC.ptr());
	}
}

//TODO: DINAMIC PARTICLE NOT HARCODED
float3 ComponentParticleSystem::CreateVelocity() {
	float x, y, z;
	if (emitterType == EmitterType::CONE) {
		ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
		x = (float(rand()) / float((RAND_MAX)) * 0.2f) - 0.2f;
		y = (float(rand()) / float((RAND_MAX)) * 0.5f) - 0.0f;
		z = (float(rand()) / float((RAND_MAX)) * 0.5f) - 0.2f;
		float3 forward = transform->GetGlobalRotation() * float3::unitY;
		if (!randomDirection) return forward;
		return float3(forward.x + x, forward.y + y, forward.z + z);
	}
	//TODO: DINAMIC PARTICLE NOT HARCODED
	if (emitterType == EmitterType::SPHERE) {
		ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
		x = (transform->GetGlobalPosition().x) + (float(rand()) / float((RAND_MAX)) * 2.0f) - 1.0f;
		y = (transform->GetGlobalPosition().y) + (float(rand()) / float((RAND_MAX)) * 2.0f) - 1.0f;
		z = (transform->GetGlobalPosition().z) + (float(rand()) / float((RAND_MAX)) * 2.0f) - 1.0f;
		return float3(x, y, z);
	}

	return float3(0, 0, 0);
};

float3 ComponentParticleSystem::CreatePosition() {
	//TODO: DINAMIC PARTICLE NOT HARCODED
	float x, y, z;

	if (emitterType == EmitterType::CONE) {
		ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
		x = (transform->GetGlobalPosition().x);
		z = (transform->GetGlobalPosition().z);
		y = (transform->GetGlobalPosition().y);
		return (float3(x, y, z));
	}
	//TODO: DINAMIC PARTICLE NOT HARCODED
	if (emitterType == EmitterType::SPHERE) {
		ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
		x = (transform->GetGlobalPosition().x) + (float(rand()) / float((RAND_MAX)) * 0.5f) - 0.5f;
		z = (transform->GetGlobalPosition().z) + (float(rand()) / float((RAND_MAX)) * 0.5f) - 0.5f;
		y = (transform->GetGlobalPosition().y) + (float(rand()) / float((RAND_MAX)) * 0.5f) - 0.5f;
		return float3(transform->GetGlobalPosition());
	}

	return float3(0, 0, 0);
}

void ComponentParticleSystem::CreateParticles(unsigned nParticles, float vel) {
	particles.Allocate(maxParticles);
	for (Particle& currentParticle : particles) {
		currentParticle.scale = float3(0.1f, 0.1f, 0.1f) * scale;
		currentParticle.initialPosition = CreatePosition();
		currentParticle.position = currentParticle.initialPosition;
		currentParticle.direction = CreateVelocity();
		currentParticle.velocity = vel;
	}
}

void ComponentParticleSystem::Load(JsonValue jComponent) {
	shaderID = jComponent[JSON_TAG_TEXTURE_SHADERID];

	if (shaderID != 0) {
		App->resources->IncreaseReferenceCount(shaderID);
	}

	textureID = jComponent[JSON_TAG_TEXTURE_TEXTUREID];

	if (textureID != 0) {
		App->resources->IncreaseReferenceCount(textureID);
	}

	isPlaying = jComponent[JSON_TAG_ISPLAYING];
	looping = jComponent[JSON_TAG_LOOPING];
	isRandomFrame = jComponent[JSON_TAG_ISRANDOMFRAME];
	randomDirection = jComponent[JSON_TAG_ISRANDOMDIRECTION];
	scale = jComponent[JSON_TAG_SCALEPARTICLE];
	maxParticles = jComponent[JSON_TAG_MAXPARTICLE];
	velocity = jComponent[JSON_TAG_VELOCITY];
	particleLife = jComponent[JSON_TAG_LIFE];
	Ytiles = jComponent[JSON_TAG_YTILES];
	Xtiles = jComponent[JSON_TAG_XTILES];
	JsonValue jColor = jComponent[JSON_TAG_INITCOLOR];
	initC.Set(jColor[0], jColor[1], jColor[2]);

	JsonValue jColor2 = jComponent[JSON_TAG_FINALCOLOR];
	finalC.Set(jColor2[0], jColor2[1], jColor2[2]);
	particleSpawned = 0;
	CreateParticles(maxParticles, velocity);
}

void ComponentParticleSystem::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_TEXTURE_SHADERID] = shaderID;
	jComponent[JSON_TAG_TEXTURE_TEXTUREID] = textureID;

	jComponent[JSON_TAG_ISPLAYING] = isPlaying;
	jComponent[JSON_TAG_LOOPING] = looping;
	jComponent[JSON_TAG_ISRANDOMFRAME] = isRandomFrame;
	jComponent[JSON_TAG_ISRANDOMDIRECTION] = randomDirection;
	jComponent[JSON_TAG_SCALEPARTICLE] = scale;
	jComponent[JSON_TAG_MAXPARTICLE] = maxParticles;
	jComponent[JSON_TAG_VELOCITY] = velocity;
	jComponent[JSON_TAG_LIFE] = particleLife;
	jComponent[JSON_TAG_YTILES] = Ytiles;
	jComponent[JSON_TAG_XTILES] = Xtiles;

	JsonValue jColor = jComponent[JSON_TAG_INITCOLOR];
	jColor[0] = initC.x;
	jColor[1] = initC.y;
	jColor[2] = initC.z;
	JsonValue jColor2 = jComponent[JSON_TAG_FINALCOLOR];
	jColor2[0] = finalC.x;
	jColor2[1] = finalC.y;
	jColor2[2] = finalC.z;
}

void ComponentParticleSystem::Update() {
	AABB newBoundingBox = {
		{0, 0, 0},
		{0, 0, 0}};

	deadParticles.clear();
	for (Particle& currentParticle : particles) {
		if (App->time->IsGameRunning()) {
			currentParticle.life -= App->time->GetDeltaTime();
		} else {
			currentParticle.life -= App->time->GetRealTimeDeltaTime();
		}
		currentParticle.position += currentParticle.direction * velocity;
		currentParticle.model = float4x4::FromTRS(currentParticle.position, currentParticle.rotation, currentParticle.scale);
		if (currentParticle.life < 0) {
			deadParticles.push_back(&currentParticle);
		}
		if (currentParticle.position.x > newBoundingBox.maxPoint.x) newBoundingBox.maxPoint.x = currentParticle.position.x;
		if (currentParticle.position.x < newBoundingBox.minPoint.x) newBoundingBox.minPoint.x = currentParticle.position.x;
		if (currentParticle.position.y > newBoundingBox.maxPoint.y) newBoundingBox.maxPoint.y = currentParticle.position.y;
		if (currentParticle.position.y < newBoundingBox.minPoint.y) newBoundingBox.minPoint.y = currentParticle.position.y;
		if (currentParticle.position.z > newBoundingBox.maxPoint.z) newBoundingBox.maxPoint.z = currentParticle.position.z;
		if (currentParticle.position.z < newBoundingBox.minPoint.z) newBoundingBox.minPoint.z = currentParticle.position.z;
	}
	for (Particle* currentParticle : deadParticles) {
		particles.Release(currentParticle);
	}
	ComponentBoundingBox* bBox = GetOwner().GetComponent<ComponentBoundingBox>();
	if (!bBox) return;
	bBox->SetLocalBoundingBox(newBoundingBox);
	//Draw()
}

void ComponentParticleSystem::Init() {
	CreateParticles(maxParticles, velocity);
}

void ComponentParticleSystem::SpawnParticle() {
	Particle* currentParticle = particles.Obtain();
	if (!looping) {
		particleSpawned++;
	}
	if (currentParticle) {
		currentParticle->position = currentParticle->initialPosition;
		currentParticle->life = particleLife;
		if (isRandomFrame) {
			currentParticle->currentFrame = static_cast<float>(rand() % ((Xtiles * Ytiles) + 1));
		} else {
			currentParticle->currentFrame = 0;
		}
		currentParticle->colorFrame = 0;
		currentParticle->direction = CreateVelocity();
		currentParticle->initialPosition = CreatePosition();
		currentParticle->position = currentParticle->initialPosition;
		//TODO: not hardcoded
		currentParticle->scale = float3(0.1f, 0.1f, 0.1f) * scale;
	}
}

void ComponentParticleSystem::killParticles() {
	for (Particle& currentParticle : particles) {
		particles.Release(&currentParticle);
	}
}

void ComponentParticleSystem::DrawGizmos() {
	//TODO: IMPROVE DRAWS
	if (IsActive()) {
		if (emitterType == EmitterType::CONE) {
			ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
			dd::cone(transform->GetGlobalPosition(), transform->GetGlobalRotation() * float3::unitY * 1, dd::colors::White, 1.0f, 0.3f);
		}
		if (emitterType == EmitterType::SPHERE) {
			float delta = kl * kl - 4 * (kc - 10) * kq;
			float distance = Max(abs((-kl + sqrt(delta))) / (2 * kq), abs((-kl - sqrt(delta)) / (2 * kq)));
			ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
			dd::sphere(transform->GetGlobalPosition(), dd::colors::White, 1.0f);
		}
	}
}

float4 ComponentParticleSystem::GetTintColor() const {
	ComponentButton* button = GetOwner().GetComponent<ComponentButton>();
	if (button != nullptr) {
		return button->GetTintColor();
	}
	return float4::one;
}

void ComponentParticleSystem::Draw() {
	if (isPlaying) {
		for (Particle& currentParticle : particles) {
			unsigned int program = 0;
			ResourceShader* shaderResouce = App->resources->GetResource<ResourceShader>(shaderID);
			if (shaderResouce) {
				program = shaderResouce->GetShaderProgram();
			} else {
				return;
			}

			glDisable(GL_CULL_FACE);
			//glDisable(GL_DEPTH_TEST);
			//glDepthMask(GL_FALSE);
			/*		glEnable(GL_BLEND);*/
			//glBlendEquation(GL_MAX);
			//glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);

			glBindBuffer(GL_ARRAY_BUFFER, App->userInterface->GetQuadVBO());
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) (sizeof(float) * 6 * 3));
			glUseProgram(program);

			ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
			currentParticle.model = currentParticle.model;

			float3x3 rotatePart = currentParticle.model.RotatePart();
			Frustum* frustum = App->camera->GetActiveCamera()->GetFrustum();
			float4x4* proj = &App->camera->GetProjectionMatrix();
			float4x4* view = &App->camera->GetViewMatrix();

			float4x4 newModelMatrix = currentParticle.model.LookAt(rotatePart.Col(2), -frustum->Front(), rotatePart.Col(1), float3::unitY);
			float4x4 Final = float4x4::FromTRS(currentParticle.position, newModelMatrix.RotatePart(), currentParticle.scale);
			//-> glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_TRUE, newModelMatrix.ptr());

			glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_TRUE, Final.ptr());
			glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_TRUE, view->ptr());
			glUniformMatrix4fv(glGetUniformLocation(program, "proj"), 1, GL_TRUE, proj->ptr());
			//TODO: ADD delta Time
			if (!isRandomFrame) {
				currentParticle.currentFrame += 0.1f;
			}

			currentParticle.colorFrame += 0.01f;
			glActiveTexture(GL_TEXTURE0);
			glUniform1i(glGetUniformLocation(program, "diffuse"), 0);
			glUniform1f(glGetUniformLocation(program, "currentFrame"), currentParticle.currentFrame);
			glUniform1f(glGetUniformLocation(program, "colorFrame"), currentParticle.colorFrame);
			glUniform4fv(glGetUniformLocation(program, "initColor"), 1, initC.ptr());
			glUniform4fv(glGetUniformLocation(program, "finalColor"), 1, finalC.ptr());

			glUniform1i(glGetUniformLocation(program, "Xtiles"), Xtiles);
			glUniform1i(glGetUniformLocation(program, "Ytiles"), Ytiles);

			ResourceTexture* textureResource = App->resources->GetResource<ResourceTexture>(textureID);
			if (textureResource != nullptr) {
				glBindTexture(GL_TEXTURE_2D, textureResource->glTexture);
			}
			//TODO: implement drawarrays
			//glDrawArraysInstanced(GL_TRIANGLES, 0, 4, 100);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			/*	glDisable(GL_BLEND);
			glEnable(GL_CULL_FACE);*/
			/*		glEnable(GL_DEPTH_TEST);*/
		}
		if (looping || (particleSpawned <= maxParticles)) {
			SpawnParticle();
		}
	}
}

void ComponentParticleSystem::Play() {
	particleSpawned = 0;
	isPlaying = true;
}

void ComponentParticleSystem::Stop() {
	particleSpawned = maxParticles;
	killParticles();
	isPlaying = false;
}