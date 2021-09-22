#include "ComponentBillboard.h"

#include "Application.h"
#include "GameObject.h"
#include "Modules/ModulePrograms.h"
#include "Modules/ModuleCamera.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleUserInterface.h"
#include "Components/ComponentTransform.h"
#include "Components/UI/ComponentTransform2D.h"
#include "Resources/ResourceTexture.h"
#include "Resources/ResourceShader.h"
#include "Utils/JsonValue.h"
#include "Utils/ImGuiUtils.h"

#include "Math/float3x3.h"
#include "Math/TransformOps.h"
#include "imgui_color_gradient.h"
#include "GL/glew.h"

#include "Utils/Leaks.h"

#define JSON_TAG_BILLBOARD_TYPE "BillboardType"
#define JSON_TAG_TEXTURE_TEXTUREID "TextureId"

#define JSON_TAG_YTILES "Ytiles"
#define JSON_TAG_XTILES "Xtiles"
#define JSON_TAG_ANIMATION_SPEED "AnimationSpeed"

#define JSON_TAG_COLOR_OVER_LIFETIME "ColorOverLifetime"
#define JSON_TAG_COLOR_LIFETIME "ColorLifeTime"
#define JSON_TAG_NUMBER_COLORS "NumberColors"
#define JSON_TAG_GRADIENT_COLORS "GradientColors"

#define JSON_TAG_FLIP_TEXTURE "FlipTexture"

ComponentBillboard::~ComponentBillboard() {
	RELEASE(gradient);

	App->resources->DecreaseReferenceCount(textureID);
}

void ComponentBillboard::Init() {
	App->resources->IncreaseReferenceCount(textureID);

	if (!gradient) gradient = new ImGradient();
	ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();
	initPos = transform->GetGlobalPosition();
	previousPos = transform->GetGlobalRotation() * float3::unitY;
	float3x3 newRotation = float3x3::FromEulerXYZ(0.f, 0.f, -pi / 2);
	modelStretch = transform->GetGlobalMatrix() * newRotation;
}

void ComponentBillboard::Update() {
	ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();

	float3 position = transform->GetGlobalPosition();
	if (!previousPos.Equals(position)) {
		direction = (position - previousPos).Normalized();
	}
	previousPos = position;

	colorFrame += App->time->GetDeltaTimeOrRealDeltaTime();
	currentFrame += animationSpeed * App->time->GetDeltaTimeOrRealDeltaTime();
}

void ComponentBillboard::OnEditorUpdate() {
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

	const char* billboardTypeCombo[] = {"LookAt", "Stretch", "Horitzontal"};
	const char* billboardTypeComboCurrent = billboardTypeCombo[(int) billboardType];
	ImGui::TextColored(App->editor->textColor, "Type:");
	if (ImGui::BeginCombo("##Type", billboardTypeComboCurrent)) {
		for (int n = 0; n < IM_ARRAYSIZE(billboardTypeCombo); ++n) {
			bool isSelected = (billboardTypeComboCurrent == billboardTypeCombo[n]);
			if (ImGui::Selectable(billboardTypeCombo[n], isSelected)) {
				billboardType = (BillboardType) n;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Separator();

	ImGui::TextColored(App->editor->textColor, "Texture Settings");
	ImGui::Separator();

	ImGui::Checkbox("Random Frame", &isRandomFrame);

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

		ImGui::NewLine();
		ImGui::Separator();
		ImGui::TextColored(App->editor->titleColor, "Texture Preview");
		ImGui::TextWrapped("Size:");
		ImGui::SameLine();
		ImGui::TextWrapped("%i x %i", width, height);
		ImGui::Image((void*) textureResource->glTexture, ImVec2(200, 200));

		ImGui::NewLine();
		ImGui::TextColored(App->editor->titleColor, "Texture Sheet Animation");
		ImGui::DragScalar("Xtiles", ImGuiDataType_U32, &Xtiles);
		ImGui::DragScalar("Ytiles", ImGuiDataType_U32, &Ytiles);
		ImGui::DragFloat("Animation Speed", &animationSpeed, App->editor->dragSpeed2f, -inf, inf);

		ImGui::NewLine();
		ImGui::TextColored(App->editor->titleColor, "Color over Lifetime");
		ImGui::Checkbox("##color_over_lifetime", &colorOverLifetime);
		if (colorOverLifetime) {
			ImGui::SameLine();
			ImGui::GradientEditor(gradient, draggingGradient, selectedGradient);
			ImGui::DragFloat("Color Lifetime", &colorLifetime, App->editor->dragSpeed2f, 0, inf);
			if (ImGui::Button("Reset Color")) {
				ResetColor();
			}
		}

		ImGui::NewLine();
		ImGui::Text("Flip: ");
		ImGui::SameLine();
		ImGui::Checkbox("X", &flipTexture[0]);
		ImGui::SameLine();
		ImGui::Checkbox("Y", &flipTexture[1]);
	}
}

void ComponentBillboard::Load(JsonValue jComponent) {
	billboardType = (BillboardType)(int) jComponent[JSON_TAG_BILLBOARD_TYPE];

	textureID = jComponent[JSON_TAG_TEXTURE_TEXTUREID];

	Ytiles = jComponent[JSON_TAG_YTILES];
	Xtiles = jComponent[JSON_TAG_XTILES];
	animationSpeed = jComponent[JSON_TAG_ANIMATION_SPEED];

	colorOverLifetime = jComponent[JSON_TAG_COLOR_OVER_LIFETIME];
	colorLifetime = jComponent[JSON_TAG_COLOR_LIFETIME];
	int numberColors = jComponent[JSON_TAG_NUMBER_COLORS];
	if (!gradient) gradient = new ImGradient();
	gradient->clearList();
	JsonValue jColor = jComponent[JSON_TAG_GRADIENT_COLORS];
	for (int i = 0; i < numberColors; ++i) {
		JsonValue jMark = jColor[i];
		gradient->addMark(jMark[4], ImColor((float) jMark[0], (float) jMark[1], (float) jMark[2], (float) jMark[3]));
	}

	JsonValue jFlip = jComponent[JSON_TAG_FLIP_TEXTURE];
	flipTexture[0] = jFlip[0];
	flipTexture[1] = jFlip[1];
}

void ComponentBillboard::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_BILLBOARD_TYPE] = (int) billboardType;

	jComponent[JSON_TAG_TEXTURE_TEXTUREID] = textureID;

	jComponent[JSON_TAG_YTILES] = Ytiles;
	jComponent[JSON_TAG_XTILES] = Xtiles;
	jComponent[JSON_TAG_ANIMATION_SPEED] = animationSpeed;

	jComponent[JSON_TAG_COLOR_OVER_LIFETIME] = colorOverLifetime;
	jComponent[JSON_TAG_COLOR_LIFETIME] = colorLifetime;
	int color = 0;
	JsonValue jColor = jComponent[JSON_TAG_GRADIENT_COLORS];
	for (ImGradientMark* mark : gradient->getMarks()) {
		JsonValue jMask = jColor[color];
		jMask[0] = mark->color[0];
		jMask[1] = mark->color[1];
		jMask[2] = mark->color[2];
		jMask[3] = mark->color[3];
		jMask[4] = mark->position;

		color++;
	}
	jComponent[JSON_TAG_NUMBER_COLORS] = gradient->getMarks().size();

	JsonValue jFlip = jComponent[JSON_TAG_FLIP_TEXTURE];
	jFlip[0] = flipTexture[0];
	jFlip[1] = flipTexture[1];
}

void ComponentBillboard::Draw() {
	ProgramBillboard* program = App->programs->billboard;
	glUseProgram(program->program);

	unsigned glTexture = 0;
	ResourceTexture* texture = App->resources->GetResource<ResourceTexture>(textureID);
	glTexture = texture ? texture->glTexture : 0;
	int hasDiffuseMap = texture ? 1 : 0;

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	glBindBuffer(GL_ARRAY_BUFFER, App->userInterface->GetQuadVBO());
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) (sizeof(float) * 6 * 3));

	ComponentTransform* transform = GetOwner().GetComponent<ComponentTransform>();

	float3x3 rotatePart = transform->GetGlobalMatrix().RotatePart();
	float3 position = transform->GetGlobalPosition();
	float4x4 modelMatrix = transform->GetGlobalMatrix();
	float3 scale = transform->GetGlobalScale();

	Frustum* frustum = App->camera->GetActiveCamera()->GetFrustum();
	float4x4* proj = &App->camera->GetProjectionMatrix();
	float4x4* view = &App->camera->GetViewMatrix();

	float4x4 newModelMatrix;

	if (billboardType == BillboardType::NORMAL) {
		newModelMatrix = modelMatrix.LookAt(rotatePart.Col(2), -frustum->Front(), rotatePart.Col(1), float3::unitY);
		newModelMatrix = float4x4::FromTRS(position, newModelMatrix.RotatePart() * modelMatrix.RotatePart(), scale);

	} else if (billboardType == BillboardType::STRETCH) {
		float3 cameraPos = App->camera->GetActiveCamera()->GetFrustum()->Pos();
		float3 cameraDir = (cameraPos - position).Normalized();
		float3 upDir = Cross(direction, cameraDir);
		float3 newCameraDir = Cross(direction, upDir);

		float3x3 newRotation;
		newRotation.SetCol(0, upDir);
		newRotation.SetCol(1, direction);
		newRotation.SetCol(2, newCameraDir);

		newModelMatrix = float4x4::FromTRS(position, newRotation * modelStretch.RotatePart(), scale);

	} else if (billboardType == BillboardType::HORIZONTAL) {
		newModelMatrix = modelMatrix.LookAt(rotatePart.Col(2), float3::unitY, rotatePart.Col(1), float3::unitY);
		newModelMatrix = float4x4::FromTRS(position, newModelMatrix.RotatePart() * modelMatrix.RotatePart(), scale);

	} else if (billboardType == BillboardType::VERTICAL) {
		// TODO: Implement it
		modelMatrix = modelMatrix;
	}

	glUniformMatrix4fv(program->modelLocation, 1, GL_TRUE, newModelMatrix.ptr());
	glUniformMatrix4fv(program->viewLocation, 1, GL_TRUE, view->ptr());
	glUniformMatrix4fv(program->projLocation, 1, GL_TRUE, proj->ptr());

	float4 color = float4::one;
	if (colorOverLifetime) {
		float factor = colorFrame / colorLifetime;
		gradient->getColorAt(factor, color.ptr());
	}

	glUniform1i(program->diffuseMapLocation, 0);
	glUniform1i(program->hasDiffuseLocation, hasDiffuseMap);
	glUniform4fv(program->inputColorLocation, 1, color.ptr());

	glUniform1f(program->currentFrameLocation, currentFrame);

	glUniform1i(program->xTilesLocation, Xtiles);
	glUniform1i(program->yTilesLocation, Ytiles);

	glUniform1i(program->xFlipLocation, flipTexture[0]);
	glUniform1i(program->yFlipLocation, flipTexture[1]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, glTexture);

	//TODO: implement drawarrays
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

void ComponentBillboard::ResetColor() {
	colorFrame = 0.0f;
}
