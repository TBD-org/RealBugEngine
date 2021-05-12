#include "ComponentText.h"

#include "GameObject.h"
#include "Application.h"
#include "ComponentTransform2D.h"
#include "Modules/ModulePrograms.h"
#include "Modules/ModuleCamera.h"
#include "Modules/ModuleRender.h"
#include "Modules/ModuleUserInterface.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleEditor.h"
#include "Resources/ResourceTexture.h"
#include "Resources/ResourceFont.h"
#include "FileSystem/JsonValue.h"

#include "GL/glew.h"
#include "Math/TransformOps.h"
#include "Utils/Logging.h"
#include "Utils/ImGuiUtils.h"
#include "imgui_stdlib.h"

#include "Utils/Leaks.h"

#define JSON_TAG_TEXT_FONTID "FontID"
#define JSON_TAG_TEXT_FONTSIZE "FontSize"
#define JSON_TAG_TEXT_LINEHEIGHT "LineHeight"
#define JSON_TAG_TEXT_VALUE "Value"
#define JSON_TAG_TEXT_ALIGNMENT "Alignment"
#define JSON_TAG_COLOR "Color"

ComponentText::~ComponentText() {
	//TODO DECREASE REFERENCE COUNT OF SHADER AND FONT, MAYBE IN A NEW COMPONENT::CLEANUP?
}

void ComponentText::Init() {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	Invalidate();
}

void ComponentText::OnEditorUpdate() {
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

	ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;

	bool mustRecalculateVertices = false;

	if (ImGui::InputTextMultiline("Text input", &text, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 8), flags)) {
		SetText(text);
	}
	UID oldFontID = fontID;
	ImGui::ResourceSlot<ResourceFont>("Font", &fontID);
	if (oldFontID != fontID) {
		mustRecalculateVertices = true;
	}

	if (ImGui::DragFloat("Font Size", &fontSize, 2.0f, 0, FLT_MAX)) {
		mustRecalculateVertices = true;
	}
	if (ImGui::DragFloat("Line height", &lineHeight, 2.0f, 0, FLT_MAX)) {
		mustRecalculateVertices = true;
	}

	mustRecalculateVertices |= ImGui::RadioButton("Left", &textAlignment, 0);
	ImGui::SameLine();
	mustRecalculateVertices |= ImGui::RadioButton("Center", &textAlignment, 1);
	ImGui::SameLine();
	mustRecalculateVertices |= ImGui::RadioButton("Right", &textAlignment, 2);

	ImGui::ColorEdit4("Color##", color.ptr());

	if (mustRecalculateVertices) {
		Invalidate();
	}
}

void ComponentText::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_TEXT_FONTID] = fontID;
	jComponent[JSON_TAG_TEXT_FONTSIZE] = fontSize;
	jComponent[JSON_TAG_TEXT_LINEHEIGHT] = lineHeight;
	jComponent[JSON_TAG_TEXT_ALIGNMENT] = textAlignment;

	jComponent[JSON_TAG_TEXT_VALUE] = text.c_str();

	JsonValue jColor = jComponent[JSON_TAG_COLOR];
	jColor[0] = color.x;
	jColor[1] = color.y;
	jColor[2] = color.z;
	jColor[3] = color.w;
}

void ComponentText::Load(JsonValue jComponent) {
	fontID = jComponent[JSON_TAG_TEXT_FONTID];
	App->resources->IncreaseReferenceCount(fontID);

	fontSize = jComponent[JSON_TAG_TEXT_FONTSIZE];

	lineHeight = jComponent[JSON_TAG_TEXT_LINEHEIGHT];

	textAlignment = jComponent[JSON_TAG_TEXT_ALIGNMENT];

	text = jComponent[JSON_TAG_TEXT_VALUE];

	JsonValue jColor = jComponent[JSON_TAG_COLOR];
	color.Set(jColor[0], jColor[1], jColor[2], jColor[3]);
}

void ComponentText::Draw(ComponentTransform2D* transform) {
	if (fontID == 0) {
		return;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	unsigned int program = App->programs->textUI;

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(vao);

	glUseProgram(program);

	float4x4 model = transform->GetGlobalMatrix();
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
	glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_TRUE, proj.ptr());
	glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_TRUE, model.ptr());
	glUniform4fv(glGetUniformLocation(program, "textColor"), 1, color.ptr());

	RecalculateVertices();

	for (size_t i = 0; i < text.size(); ++i) {
		if (text.at(i) != '\n') {
			Character character = App->userInterface->GetCharacter(fontID, text.at(i));

			// render glyph texture over quad
			glBindTexture(GL_TEXTURE_2D, character.textureID);
			// update content of VBO memory
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verticesText[i]), &verticesText[i].front());
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			// render quad
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_BLEND);
}

void ComponentText::SetText(const std::string& newText) {
	text = newText;
	Invalidate();
}

void ComponentText::SetFontSize(float newfontSize) {
	fontSize = newfontSize;
	Invalidate();
}

void ComponentText::SetFontColor(const float4& newColor) {
	color = newColor;
}

float4 ComponentText::GetFontColor() const {
	return color;
}

void ComponentText::RecalculateVertices() {
	if (!dirty) {
		return;
	}

	if (fontID == 0) {
		return;
	}

	verticesText.resize(text.size());

	ComponentTransform2D* transform = GetOwner().GetComponent<ComponentTransform2D>();

	float x = -transform->GetSize().x / 2.0f;
	float y = 0;

	float dy = 0; // additional y shifting
	int j = 0;	  // index of row

	// FontSize / size of imported font. 48 is due to FontImporter default PixelSize
	float scale = (fontSize / 48);

	for (size_t i = 0; i < text.size(); ++i) {
		Character character = App->userInterface->GetCharacter(fontID, text.at(i));

		float xpos = x + character.bearing.x * scale;
		float ypos = y - (character.size.y - character.bearing.y) * scale;

		float w = character.size.x * scale;
		float h = character.size.y * scale;

		switch (textAlignment) {
		case TextAlignment::LEFT: {
			// Default branch, could be deleted
			break;
		}
		case TextAlignment::CENTER: {
			xpos += (transform->GetSize().x / 2.0f - SubstringWidth(&text.c_str()[j], scale) / 2.0f);
			break;
		}
		case TextAlignment::RIGHT: {
			xpos += transform->GetSize().x - SubstringWidth(&text.c_str()[j], scale);
			break;
		}
		}

		if (text.at(i) == '\n') {
			dy += lineHeight;					// shifts to next line
			x = -transform->GetSize().x / 2.0f; // reset to initial position
			j = i + 1;							// updated j variable in order to get the substringwidth of the following line in the next iteration
		}

		// clang-format off
		verticesText[i] = {
			xpos, ypos + h - dy, 0.0f, 0.0f,
			xpos, ypos - dy, 0.0f, 1.0f,
			xpos + w, ypos - dy, 1.0f, 1.0f,
			xpos, ypos + h - dy, 0.0f, 0.0f,
			xpos + w, ypos - dy, 1.0f, 1.0f,
			xpos + w, ypos + h - dy, 1.0f, 0.0f
		};
		// clang-format on

		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		if (text.at(i) != '\n') {
			x += (character.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64). Divides / 64
		}
	}

	dirty = false;
}

void ComponentText::Invalidate() {
	dirty = true;
}

float ComponentText::SubstringWidth(const char* substring, float scale) {
	float subWidth = 0.f;

	for (int i = 0; substring[i] != '\0' && substring[i] != '\n'; ++i) {
		Character c = App->userInterface->GetCharacter(fontID, substring[i]);
		subWidth += (c.advance >> 6) * scale;
	}

	return subWidth;
}