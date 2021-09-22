#include "Trail.h"

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
#include "Importers/TextureImporter.h"
#include "Utils/JsonValue.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/Logging.h"

#include "Math/float3x3.h"
#include "Math/TransformOps.h"
#include "imgui.h"
#include "GL/glew.h"
#include "debugdraw.h"
#include "rapidjson/rapidjson.h"
#include "imgui_color_gradient.h"
#include "Math/float3.h"
#include <string>
#include "Utils/Leaks.h"

void Trail::Init() {
	glGenBuffers(1, &quadVBO);
	EditTextureCoords();
}

bool Trail::CalculateDistance(float3 A, float3 B) {
	if (B.DistanceSq(A) > vertexDistance * vertexDistance) return true;
	return false;
}
void Trail::Update(float3 mPosition) {
	if (!isRendering) return;
	float3 currentDirection = (currentPosition - previousPosition).Normalized();
	float3 right = currentDirection.Perpendicular(float3::unitY, -float3::unitZ);
	float3 vectorUp = Cross(currentDirection, right);

	if (isStarted) {
		if (CalculateDistance(previousPosition, mPosition)) {
			previousPositionUp = currentPositionUp;
			previousPositionDown = currentPositionDown;
			previousPosition = currentPosition;

			currentPosition = mPosition;
			currentPositionUp = (vectorUp * width) + currentPosition;
			currentPositionDown = (-vectorUp * width) + currentPosition;

			if (quadsCreated >= trailQuads) {
				for (int j = 0; j < trailQuads - 1; j++) {
					quads[j] = quads[j + 1];
				}
				quadsCreated--;
			}

			Quad* currentQuad = &quads[quadsCreated];
			SpawnQuad(currentQuad);

			InsertVertex(currentQuad, previousPositionDown);
			InsertTextureCoords(currentQuad);
			InsertVertex(currentQuad, currentPositionDown);
			InsertTextureCoords(currentQuad);
			InsertVertex(currentQuad, previousPositionUp);
			InsertTextureCoords(currentQuad);

			InsertVertex(currentQuad, currentPositionDown);
			InsertTextureCoords(currentQuad);
			InsertVertex(currentQuad, currentPositionUp);
			InsertTextureCoords(currentQuad);
			InsertVertex(currentQuad, previousPositionUp);
			InsertTextureCoords(currentQuad);
		} else {
			if (quadsCreated > 0) {
				currentPosition = mPosition;
				currentPositionUp = (vectorUp * width) + currentPosition;
				currentPositionDown = (-vectorUp * width) + currentPosition;

				Quad* currentQuad = &quads[quadsCreated-1];

				UpdateVertex(currentQuad, currentPositionDown, 5);
				UpdateVertex(currentQuad, currentPositionDown, 15);
				UpdateVertex(currentQuad, currentPositionUp, 20);
			}
		}
	} else {
		isStarted = true;
		currentPosition = mPosition;
		currentPositionUp = vectorUp * width + currentPosition;
		currentPositionDown = -vectorUp * width + currentPosition;
		return;
	}
	UpdateQuads();
}

void Trail::UpdateQuads() {
	for (int i = 0; i < quadsCreated; i++) {
		UpdateLife(&quads[i]);

		if (quads[i].life <= 0) {
			quadsCreated--;

			for (int j = 0; j < trailQuads - 1; j++) {
				for (int x = 0; x < 30; x++) {
					quads[j].quadInfo[x] = quads[j + 1].quadInfo[x];
				}
			}
		}
	}
}

void Trail::UpdateLife(Quad* currentQuad) {
	if (App->time->IsGameRunning()) {
		currentQuad->life -= App->time->GetDeltaTime();
	} else {
		currentQuad->life -= App->time->GetRealTimeDeltaTime();
	}
}

void Trail::OnEditorUpdate() {
	if (ImGui::Button("Play")) Play();
	if (ImGui::Button("Stop")) Stop();
	if (ImGui::Checkbox("Render", &isRendering)) {
		isStarted = false;
	}
	ImGui::DragFloat("Width", &width, App->editor->dragSpeed2f, 0, inf);
	if (ImGui::DragInt("Trail Quads", &trailQuads, 1.0f, 1, 50, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		if (nTextures > trailQuads) nTextures = trailQuads;
		DeleteQuads();
	}
	ImGui::DragFloat("Quad Life", &quadLife, App->editor->dragSpeed2f, 1, inf);
	ImGui::DragFloat("Vertex Distance", &vertexDistance, App->editor->dragSpeed2f, 0, inf);
	ImGui::NewLine();
	ImGui::Checkbox("Color Over Trail", &colorOverTrail);
	if (colorOverTrail) {
		ImGui::GradientEditor(gradient, draggingGradient, selectedGradient);
	}

	ImGui::NewLine();
	ImGui::ResourceSlot<ResourceTexture>("texture", &textureID);
	ImGui::Text("Flip: ");
	ImGui::SameLine();
	ImGui::Checkbox("X", &flipTexture[0]);
	ImGui::SameLine();
	ImGui::Checkbox("Y", &flipTexture[1]);

	if (ImGui::DragInt("Texture Repeats", &nTextures, 1.0f, 1, trailQuads, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		DeleteQuads();
		EditTextureCoords();
	}

	ImGui::NewLine();
	ImGui::Indent();
	if (ImGui::CollapsingHeader("Texture Preview")) {
		ResourceTexture* textureResource = App->resources->GetResource<ResourceTexture>(textureID);
		if (textureResource != nullptr) {
			int width;
			int height;
			glGetTextureLevelParameteriv(textureResource->glTexture, 0, GL_TEXTURE_WIDTH, &width);
			glGetTextureLevelParameteriv(textureResource->glTexture, 0, GL_TEXTURE_HEIGHT, &height);

			ImGui::TextWrapped("Size:");
			ImGui::SameLine();
			ImGui::TextWrapped("%d x %d", width, height);
			ImGui::Image((void*) textureResource->glTexture, ImVec2(200, 200));
		}
	}
	ImGui::Unindent();
}

void Trail::Draw() {
	ProgramTrail* trailProgram = App->programs->trail;
	if (!trailProgram) return;
	unsigned glTexture = 0;

	ResourceTexture* texture = App->resources->GetResource<ResourceTexture>(textureID);
	glTexture = texture ? texture->glTexture : 0;
	int hasDiffuseMap = texture ? 1 : 0;

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glDisable(GL_CULL_FACE);

	for (int i = 0; i < quadsCreated; i++) {
		Quad* currentQuad = &quads[i];

		glBindBuffer(GL_ARRAY_BUFFER, quadVBO); // set vbo active
		glBufferData(GL_ARRAY_BUFFER, sizeof(currentQuad->quadInfo), currentQuad->quadInfo, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*) 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*) (sizeof(float) * 3));
		glUseProgram(trailProgram->program);

		Frustum* frustum = App->camera->GetActiveCamera()->GetFrustum();
		float4x4* proj = &App->camera->GetProjectionMatrix();
		float4x4* view = &App->camera->GetViewMatrix();
		float4x4 model = float4x4::identity;

		glActiveTexture(GL_TEXTURE0);

		glUniformMatrix4fv(trailProgram->viewLocation, 1, GL_TRUE, view->ptr());
		glUniformMatrix4fv(trailProgram->projLocation, 1, GL_TRUE, proj->ptr());
		glUniformMatrix4fv(trailProgram->modelLocation, 1, GL_TRUE, model.ptr());

		float4 color = float4::one;
		if (colorOverTrail) {
			float factor = 1 - currentQuad->life / quadLife; // Life decreasing
			gradient->getColorAt(factor, color.ptr());
		}

		glUniform1i(trailProgram->diffuseMap, 0);
		glUniform1i(trailProgram->hasDiffuseLocation, hasDiffuseMap);
		glUniform4fv(trailProgram->inputColorLocation, 1, color.ptr());

		glUniform1i(trailProgram->xFlipLocation, flipTexture[0] ? 1 : 0);
		glUniform1i(trailProgram->yFlipLocation, flipTexture[1] ? 1 : 0);

		glBindTexture(GL_TEXTURE_2D, glTexture);

		glDrawArrays(GL_TRIANGLES, 0, currentQuad->index);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

void Trail::InsertVertex(Quad* currentQuad, float3 vertex) {
	currentQuad->quadInfo[currentQuad->index++] = vertex.x;
	currentQuad->quadInfo[currentQuad->index++] = vertex.y;
	currentQuad->quadInfo[currentQuad->index++] = vertex.z;
}

void Trail::InsertTextureCoords(Quad* currentQuad) {
	if (nTextures == 1) {
		textureCreated = 0;
		for (int i = 0; i < quadsCreated; i++) {
			for (int j = 0; j < 6; j++) {
				quads[i].quadInfo[(j * 5) + 3] = textureCords[textureCreated++];
				quads[i].quadInfo[(j * 5) + 4] = textureCords[textureCreated++];
			}
		}
		currentQuad->quadInfo[currentQuad->index++] = textureCords[textureCreated++];
		currentQuad->quadInfo[currentQuad->index++] = textureCords[textureCreated++];
	} else {
		if (textureCreated == nRepeats) textureCreated = 0;
		currentQuad->quadInfo[currentQuad->index++] = textureCords[textureCreated++];
		currentQuad->quadInfo[currentQuad->index++] = textureCords[textureCreated++];
	}
}

void Trail::SpawnQuad(Quad* currentQuad) {
	currentQuad->index = 0;
	currentQuad->life = quadLife;
	quadsCreated++;
}

void Trail::UpdateVertex(Quad* currentQuad, float3 vertex, int index) {
	currentQuad->quadInfo[index++] = vertex.x;
	currentQuad->quadInfo[index++] = vertex.y;
	currentQuad->quadInfo[index++] = vertex.z;
}

void Trail::DeleteQuads() {
	for (int i = 0; i < maxQuads; i++) {
		quads[i].life = 0;
		quadsCreated--;
	}

	quadsCreated = 0;
	maxVertices = 30 * trailQuads;
	isStarted = false;
	textureCreated = 0;
}

void Trail::EditTextureCoords() {
	if (nTextures == 0) return;

	int nLine = 0;
	float factor = nTextures / (float) trailQuads;
	nRepeats = (float) (trailQuads / nTextures) * 12;
	for (int textureEdited = 0; textureEdited < nRepeats;) {
		///vertice1
		textureCords[textureEdited++] = nLine * factor;
		textureCords[textureEdited++] = 0.0f;

		textureCords[textureEdited++] = (nLine + 1) * factor;
		textureCords[textureEdited++] = 0.0f;

		textureCords[textureEdited++] = nLine * factor;
		textureCords[textureEdited++] = 1.0f;

		///vertice2
		textureCords[textureEdited++] = (nLine + 1) * factor;
		textureCords[textureEdited++] = 0.0f;

		textureCords[textureEdited++] = (nLine + 1) * factor;
		textureCords[textureEdited++] = 1.0f;

		textureCords[textureEdited++] = nLine * factor;
		textureCords[textureEdited++] = 1.0f;

		nLine++;
	}
}

TESSERACT_ENGINE_API void Trail::Play() {
	isRendering = true;
}

TESSERACT_ENGINE_API void Trail::Stop() {
	isRendering = false;
	isStarted = false;
	DeleteQuads();
}

TESSERACT_ENGINE_API void Trail::SetWidth(float w) {
	width = w;
}