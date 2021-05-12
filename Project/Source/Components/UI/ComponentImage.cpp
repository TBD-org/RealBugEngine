#include "ComponentImage.h"

#include "GameObject.h"
#include "Components/UI/ComponentTransform2D.h"
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
#include "FileSystem/TextureImporter.h"
#include "FileSystem/JsonValue.h"

#include "Utils/ImGuiUtils.h"
#include "Math/TransformOps.h"
#include "imgui.h"
#include "GL/glew.h"
#include "debugdraw.h"

#include "Utils/Leaks.h"

#define JSON_TAG_TEXTURE_TEXTUREID "TextureId"
#define JSON_TAG_COLOR "Color"
#define JSON_TAG_ALPHATRANSPARENCY "AlphaTransparency"
#define JSON_TAG_IS_FILL "IsFill"
#define JSON_TAG_FILL_VALUE "FillValue"

ComponentImage::~ComponentImage() {
	//TODO DECREASE REFERENCE COUNT OF SHADER AND TEXTURE, MAYBE IN A NEW COMPONENT::CLEANUP?
}

void ComponentImage::Init() {
	RebuildFillQuadVBO();
}

void ComponentImage::Update() {
}

void ComponentImage::OnEditorUpdate() {
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

	ImGui::ColorEdit4("Color##", color.ptr());

	ImGui::Checkbox("Alpha transparency", &alphaTransparency);

	ImGui::Checkbox("Is Fill", &isFill);

	if (ImGui::DragFloat("Fill", &fillVal, App->editor->dragSpeed2f, 0, 1)) {
		RebuildFillQuadVBO();
	}

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
				transform2D->SetSize(float2((float) width, (float) height));
			}
		}

		ImGui::Text("");
		ImGui::Separator();
		ImGui::TextColored(App->editor->titleColor, "Texture Preview");
		ImGui::TextWrapped("Size:");
		ImGui::SameLine();
		ImGui::TextWrapped("%d x %d", width, height);
		ImGui::Image((void*) textureResource->glTexture, ImVec2(200, 200));
		ImGui::Separator();
	}
}

void ComponentImage::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_TEXTURE_TEXTUREID] = textureID;
	JsonValue jColor = jComponent[JSON_TAG_COLOR];
	jColor[0] = color.x;
	jColor[1] = color.y;
	jColor[2] = color.z;
	jColor[3] = color.w;

	jComponent[JSON_TAG_ALPHATRANSPARENCY] = alphaTransparency;
	jComponent[JSON_TAG_IS_FILL] = isFill;
	jComponent[JSON_TAG_FILL_VALUE] = fillVal;
}

void ComponentImage::Load(JsonValue jComponent) {
	textureID = jComponent[JSON_TAG_TEXTURE_TEXTUREID];

	if (textureID != 0) {
		App->resources->IncreaseReferenceCount(textureID);
	}

	JsonValue jColor = jComponent[JSON_TAG_COLOR];
	color.Set(jColor[0], jColor[1], jColor[2], jColor[3]);

	alphaTransparency = jComponent[JSON_TAG_ALPHATRANSPARENCY];
	isFill = jComponent[JSON_TAG_IS_FILL];
	fillVal = jComponent[JSON_TAG_FILL_VALUE];
}

float4 ComponentImage::GetMainColor() const {

	float4 componentColor = App->userInterface->GetErrorColor();

	ComponentButton* button = GetOwner().GetComponent<ComponentButton>();
	if (button != nullptr) {
		componentColor = button->GetTintColor();
	}

	ComponentSlider* slider = GetOwner().GetComponent<ComponentSlider>();
	if (slider != nullptr) {
		componentColor = slider->GetTintColor();
	}

	ComponentToggle* toggle = GetOwner().GetComponent<ComponentToggle>();
	if (toggle != nullptr) {
		componentColor = toggle->GetTintColor();
	}
	
	return componentColor.Equals(App->userInterface->GetErrorColor()) ? color : componentColor;
}

void ComponentImage::Draw(ComponentTransform2D* transform) const {
	unsigned int program = App->programs->imageUI;

	if (alphaTransparency) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	if (isFill) {
		glBindBuffer(GL_ARRAY_BUFFER, fillQuadVBO);
	} else {
		glBindBuffer(GL_ARRAY_BUFFER, App->userInterface->GetQuadVBO());
	}
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) ((sizeof(float) * 6 * 3)));
	glUseProgram(program);

	float4x4 modelMatrix = transform->GetGlobalScaledMatrix();
	float4x4& proj = App->camera->GetProjectionMatrix();
	float4x4& view = App->camera->GetViewMatrix();

	if (App->userInterface->IsUsing2D()) {
		proj = float4x4::D3DOrthoProjLH(-1, 1, App->renderer->GetViewportSize().x, App->renderer->GetViewportSize().y); //near plane. far plane, screen width, screen height
		view = float4x4::identity;
	}

	ComponentCanvasRenderer* canvasRenderer = GetOwner().GetComponent<ComponentCanvasRenderer>();
	if (canvasRenderer != nullptr) {
		float factor = canvasRenderer->GetCanvasScreenFactor();
		view = view * float4x4::Scale(factor, factor, factor);
	}

	glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_TRUE, view.ptr());
	glUniformMatrix4fv(glGetUniformLocation(program, "proj"), 1, GL_TRUE, proj.ptr());
	glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_TRUE, modelMatrix.ptr());

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "diffuse"), 0);
	glUniform4fv(glGetUniformLocation(program, "inputColor"), 1, GetMainColor().ptr());

	ResourceTexture* textureResource = App->resources->GetResource<ResourceTexture>(textureID);
	if (textureResource != nullptr) {
		glBindTexture(GL_TEXTURE_2D, textureResource->glTexture);
		glUniform1i(glGetUniformLocation(program, "hasDiffuse"), 1);
	} else {
		glUniform1i(glGetUniformLocation(program, "hasDiffuse"), 0);
	}

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisable(GL_BLEND);
}

void ComponentImage::SetColor(float4 color_) {
	color = color_;
}

void ComponentImage::SetFillValue(float val) {
	if (val >= 1.0f) {
		fillVal = 1.0f;
	} else
		fillVal = val;
}

void ComponentImage::SetIsFill(bool b) {
	isFill = b;
}

void ComponentImage::RebuildFillQuadVBO() {
	float buffer_data[] = {
		-0.5f,
		-0.5f,
		0.0f, //  v0 pos

		0.5f,
		-0.5f,
		0.0f, // v1 pos

		-0.5f,
		-0.5f + fillVal,
		0.0f, //  v2 pos

		0.5f,
		-0.5f,
		0.0f, //  v3 pos

		0.5f,
		-0.5f + fillVal,
		0.0f, // v4 pos

		-0.5f,
		-0.5f + fillVal,
		0.0f, //  v5 pos

		0.0f,
		0.0f, //  v0 texcoord

		1.0f,
		0.0f, //  v1 texcoord

		0.0f ,
		1.0f * fillVal, //  v2 texcoord

		1.0f,
		0.0f, //  v3 texcoord

		1.0f ,
		1.0f * fillVal, //  v4 texcoord

		0.0f ,
		1.0f * fillVal //  v5 texcoord
	};
	glGenBuffers(1, &fillQuadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, fillQuadVBO); // set vbo active
	glBufferData(GL_ARRAY_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);
}