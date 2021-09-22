#include "PanelConfiguration.h"

#include "Application.h"
#include "Utils/Logging.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleHardwareInfo.h"
#include "Modules/ModuleWindow.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleRender.h"
#include "Modules/ModuleCamera.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModulePhysics.h"
#include "Modules/ModuleConfiguration.h"
#include "Resources/ResourceScene.h"
#include "Resources/ResourceNavMesh.h"
#include "Resources/ResourceTexture.h"
#include "Scene.h"
#include "Utils/ImGuiUtils.h"


#include "GL/glew.h"
#include "imgui.h"
#include "IconsForkAwesome.h"

#include "Utils/Leaks.h"

PanelConfiguration::PanelConfiguration()
	: Panel("Configuration", true) {}

void PanelConfiguration::Update() {
	ImGui::SetNextWindowDockID(App->editor->dockLeftId, ImGuiCond_FirstUseEver);
	std::string windowName = std::string(ICON_FK_COGS " ") + name;
	if (ImGui::Begin(windowName.c_str(), &enabled)) {
		// Save/Load buttons
		ImGui::TextColored(App->editor->titleColor, "Configuration");
		if (ImGui::Button("Reload")) {
			App->configuration->LoadConfiguration();
		}
		ImGui::SameLine();
		if (ImGui::Button("Save")) {
			App->configuration->SaveConfiguration();
		}

		ImGui::Separator();

		// Application
		if (ImGui::CollapsingHeader("Application")) {
			if (ImGui::InputText("App name", App->appName, IM_ARRAYSIZE(App->appName))) {
				App->window->SetTitle(App->appName);
			}
			ImGui::InputText("Organization", App->organization, IM_ARRAYSIZE(App->organization));
			ImGui::TextColored(App->editor->titleColor, "Start scene");
			ImGui::ResourceSlot<ResourceScene>("Scene", &App->scene->startSceneId);
		}

		// Time
		if (ImGui::CollapsingHeader("Time")) {
			ImGui::SliderInt("Max FPS", &App->time->maxFps, 1, 240);
			ImGui::Checkbox("Limit framerate", &App->time->limitFramerate);
			if (ImGui::Checkbox("VSync", &App->time->vsync)) {
				App->renderer->SetVSync(App->time->vsync);
			}
			ImGui::SliderInt("Step delta time (MS)", &App->time->stepDeltaTimeMs, 1, 1000);
			ImGui::SliderFloat("TimeScale", &App->time->timeScale, 0.f, 4.f);

			// FPS Graph
			char title[25];
			sprintf_s(title, 25, "Framerate %.1f", logger->fpsLog[logger->fpsLogIndex]);
			ImGui::PlotHistogram("##framerate", &logger->fpsLog[0], FPS_LOG_SIZE, logger->fpsLogIndex, title, 0.0f, 100.0f, ImVec2(310, 100));
			sprintf_s(title, 25, "Milliseconds %0.1f", logger->msLog[logger->fpsLogIndex]);
			ImGui::PlotHistogram("##milliseconds", &logger->msLog[0], FPS_LOG_SIZE, logger->fpsLogIndex, title, 0.0f, 40.0f, ImVec2(310, 100));
		}

		// Physics
		if (ImGui::CollapsingHeader("Physics")) {
			if (ImGui::DragFloat("Gravity", &App->physics->gravity, App->editor->dragSpeed3f, -100.f, 100.f)) {
				App->physics->SetGravity(App->physics->gravity);
			}
		}

		// Hardware
		if (ImGui::CollapsingHeader("Hardware")) {
			ImGui::Text("GLEW version:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, App->hardware->glewVersion);
			ImGui::Text("SDL version:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, App->hardware->sdlVersion);
			ImGui::Text("Assimp version:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, App->hardware->assimpVersion);
			ImGui::Text("DeviL version:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, App->hardware->devilVersion);
			ImGui::Text("Bullet Physics version:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, App->hardware->bulletVersion);

			ImGui::Separator();

			ImGui::Text("CPUs:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, "%i (Cache: %i kb)", App->hardware->cpuCount, App->hardware->cacheSizeKb);
			ImGui::Text("System RAM:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, "%.1f Gb", App->hardware->ramGb);
			ImGui::Text("Caps:");
			const char* items[] = {"3DNow", "ARMSIMD", "AVX", "AVX2", "AVX512F", "AltiVec", "MMX", "NEON", "RDTSC", "SSE", "SSE2", "SSE3", "SSE41", "SSE42"};
			for (int i = 0; i < IM_ARRAYSIZE(items); ++i) {
				if (App->hardware->caps[i]) {
					ImGui::SameLine();
					ImGui::TextColored(App->editor->textColor, items[i]);
				}

				// Line break to avoid too many items in the same line
				if (i == 6) {
					ImGui::Text("");
				}
			}

			ImGui::Separator();

			ImGui::Text("GPU Vendor:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, "%s", App->hardware->gpuVendor);
			ImGui::Text("GPU Renderer:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, "%s", App->hardware->gpuRenderer);
			ImGui::Text("GPU OpenGL Version:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, "%s", App->hardware->gpuOpenglVersion);
			ImGui::Text("VRAM Budget:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, "%.1f Mb", App->hardware->vramBudgetMb);
			ImGui::Text("VRAM Usage:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, "%.1f Mb", App->hardware->vramUsageMb);
			ImGui::Text("VRAM Available:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, "%.1f Mb", App->hardware->vramAvailableMb);
			ImGui::Text("VRAM Reserved:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, "%.1f Mb", App->hardware->vramReservedMb);
		}

		// Window
		if (ImGui::CollapsingHeader("Window")) {
			// Window mode combo box
			const char* items[] = {"Windowed", "Fullscreen", "Fullscreen desktop"};
			const char* itemCurrent = items[int(App->window->GetWindowMode())];
			if (ImGui::BeginCombo("Window mode", itemCurrent)) {
				for (int n = 0; n < IM_ARRAYSIZE(items); ++n) {
					bool isSelected = (itemCurrent == items[n]);
					if (ImGui::Selectable(items[n], isSelected)) {
						App->window->SetWindowMode(WindowMode(n));
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			float brightness = App->window->GetBrightness();
			if (ImGui::SliderFloat("Brightness", &brightness, 0.25f, 1.0f)) {
				App->window->SetBrightness(brightness);
			}

			if (App->window->GetWindowMode() == WindowMode::WINDOWED) {
				bool resizable = App->window->GetResizable();
				if (ImGui::Checkbox("Resizable", &resizable)) {
					App->window->SetResizable(resizable);
				}
				if (resizable) {
					bool sizeChanged = false;
					bool sizeChanging = false;
					ImGui::SliderInt("Width", &windowWidth, 640, 4096);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						sizeChanged = true;
					}
					if (ImGui::IsItemActive()) {
						sizeChanging = true;
					}
					ImGui::SliderInt("Height", &windowHeight, 480, 2160);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						sizeChanged = true;
					}
					if (ImGui::IsItemActive()) {
						sizeChanging = true;
					}

					if (sizeChanged) {
						App->window->SetSize(windowWidth, windowHeight);
					} else if (!sizeChanging) {
						windowWidth = App->window->GetWidth();
						windowHeight = App->window->GetHeight();
					}
				}
			} else {
				int currentDisplayModeIndex = App->window->GetCurrentDisplayMode();
				const SDL_DisplayMode& currentDisplayMode = App->window->GetDisplayMode(currentDisplayModeIndex);
				char currentDisplayModeLabel[40];
				sprintf_s(currentDisplayModeLabel, " %i bpp\t%i x %i @ %iHz", SDL_BITSPERPIXEL(currentDisplayMode.format), currentDisplayMode.w, currentDisplayMode.h, currentDisplayMode.refresh_rate);

				if (ImGui::BeginCombo("Display Modes", currentDisplayModeLabel)) {
					for (unsigned displayModeIndex = 0; displayModeIndex < App->window->GetNumDisplayModes(); ++displayModeIndex) {
						const SDL_DisplayMode& displayMode = App->window->GetDisplayMode(currentDisplayModeIndex);
						bool isSelected = (currentDisplayModeIndex == displayModeIndex);
						char displayModeLabel[40];
						sprintf_s(displayModeLabel, " %i bpp\t%i x %i @ %iHz", SDL_BITSPERPIXEL(displayMode.format), displayMode.w, displayMode.h, displayMode.refresh_rate);

						if (ImGui::Selectable(displayModeLabel, isSelected)) {
							App->window->SetCurrentDisplayMode(displayModeIndex);
						}
					}
					ImGui::EndCombo();
				}
			}
		}

		// Render
		if (ImGui::CollapsingHeader("Render")) {
			ImGui::TextColored(App->editor->titleColor, "Background Settings");
			ImGui::ColorEdit3("Background", App->renderer->clearColor.ptr());

			ImGui::Separator();

			ImGui::TextColored(App->editor->titleColor, "SSAO Settings");
			ImGui::Checkbox("Activate SSAO", &App->renderer->ssaoActive);
			ImGui::DragFloat("Range", &App->renderer->ssaoRange, 0.01f, 0.0f, 100.0f);
			ImGui::DragFloat("Bias", &App->renderer->ssaoBias, 0.0001f, 0.0f, 10.0f, "%.5f");
			ImGui::DragFloat("Power", &App->renderer->ssaoPower, 0.01f, 0.0f, 100.0f);
			ImGui::DragFloat("Direct Lighting Strength", &App->renderer->ssaoDirectLightingStrength, 0.0f, 0.0f, 1.0f);

			ImGui::Separator();

			ImGui::TextColored(App->editor->titleColor, "Bloom Settings");
			ImGui::Checkbox("Activate Bloom", &App->renderer->bloomActive);
			ImGui::DragFloat("Bloom Threshold", &App->renderer->bloomThreshold, 0.1f);
			ImGui::DragFloat("Intensity", &App->renderer->bloomIntensity, 0.1f);
			if (ImGui::DragFloat("Size Multiplier", &App->renderer->bloomSizeMultiplier, 0.01f, 0.0f, 10.0f)) {
				App->renderer->ComputeBloomGaussianKernel();
			};
			ImGui::Text("Shape");
			ImGui::SliderFloat("Very Large weight", &App->renderer->bloomVeryLargeWeight, 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat("Large weight", &App->renderer->bloomLargeWeight, 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat("Medium weight", &App->renderer->bloomMediumWeight, 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat("Small weight", &App->renderer->bloomSmallWeight, 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat("Very Small weight", &App->renderer->bloomVerySmallWeight, 0.0f, 1.0f, "%.2f");

			ImGui::Separator();

			ImGui::TextColored(App->editor->titleColor, "MSAA Settings");
			if (ImGui::Checkbox("Activate MSAA", &App->renderer->msaaActive)) {
				App->renderer->UpdateFramebuffers();
			}
			const char* items[] = {"x2", "x4", "x8"};
			const char* itemCurrent = items[static_cast<int>(App->renderer->msaaSampleType)];
			if (ImGui::BeginCombo("Samples Number", itemCurrent)) {
				for (int n = 0; n < IM_ARRAYSIZE(items); ++n) {
					bool isSelected = (itemCurrent == items[n]);
					if (ImGui::Selectable(items[n], isSelected)) {
						App->renderer->msaaSampleType = static_cast<MSAA_SAMPLES_TYPE>(n);
						App->renderer->UpdateFramebuffers();
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Separator();
			ImGui::Checkbox("Activate Chromatic Aberration", &App->renderer->chromaticAberrationActive);
			ImGui::DragFloat("Chromatic Aberration Strength", &App->renderer->chromaticAberrationStrength, 0.1f);
		}

		// Scene
		if (ImGui::CollapsingHeader("Scene")) {
			ImGui::TextColored(App->editor->titleColor, "Ambient color");
			ImGui::ColorEdit3("Ambient Color", App->renderer->ambientColor.ptr());

			Scene* scene = App->scene->GetCurrentScene();
			ImGui::TextColored(App->editor->titleColor, "Quadtree");
			ImGui::InputFloat2("Min Point", scene->quadtreeBounds.minPoint.ptr());
			ImGui::InputFloat2("Max Point", scene->quadtreeBounds.maxPoint.ptr());
			ImGui::InputScalar("Max Depth", ImGuiDataType_U32, &scene->quadtreeMaxDepth);
			ImGui::InputScalar("Elements Per Node", ImGuiDataType_U32, &scene->quadtreeElementsPerNode);
			if (ImGui::Button("Clear Quadtree")) {
				scene->ClearQuadtree();
			}
			ImGui::SameLine();
			if (ImGui::Button("Rebuild Quadtree")) {
				scene->RebuildQuadtree();
			}

			ImGui::Separator();

			ImGui::TextColored(App->editor->titleColor, "NavMesh");
			ImGui::ResourceSlot<ResourceNavMesh>("Nav Mesh", &scene->navMeshId);

			ImGui::Separator();
			ImGui::ResourceSlot<ResourceTexture>("Cursor Texture", &scene->cursorId);

			int widthCursor = scene->widthCursor;			
			if (ImGui::DragInt("Width Cursor", &widthCursor, 1, 10, 100)) {
				scene->widthCursor = widthCursor;
			}
			int heightCursor = scene->heightCursor;
			if (ImGui::DragInt("Height Cursor", &heightCursor, 1, 10, 100)) {
				scene->heightCursor = heightCursor;
			}
		}
	}
	ImGui::End();
}
