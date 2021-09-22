#include "ModuleEditor.h"

#include "Globals.h"
#include "Application.h"
#include "GameObject.h"
#include "Scene.h"
#include "Modules/ModuleWindow.h"
#include "Modules/ModuleRender.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleProject.h"
#include "Modules/ModuleEvents.h"
#include "Modules/ModuleNavigation.h"
#include "Modules/ModuleTime.h"
#include "Importers/SceneImporter.h"
#include "Importers/PrefabImporter.h"
#include "Importers/MaterialImporter.h"
#include "Importers/NavMeshImporter.h"
#include "Navigation/NavMesh.h"
#include "Utils/FileDialog.h"
#include "TesseractEvent.h"

#include "ImGuizmo.h"
#include "imgui_internal.h"
#include "imgui_utils.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "IconsFontAwesome5.h"
#include "IconsForkAwesome.h"
#include "GL/glew.h"
#include "SDL_video.h"
#include "Brofiler.h"
#include "Math/float2.h"

#include "Utils/Leaks.h"

static const ImWchar iconsRangesFa[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
static const ImWchar iconsRangesFk[] = {ICON_MIN_FK, ICON_MAX_FK, 0};

static void ApplyCustomStyle() {
	ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;

	colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
	colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
	colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
	colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
	colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.373f, 0.000f, 1.000f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
	colors[ImGuiCol_UrlText] = ImVec4(0.000f, 0.451f, 1.0f, 1.0f);

	style->ChildRounding = 4.0f;
	style->FrameBorderSize = 1.0f;
	style->FrameRounding = 2.0f;
	style->GrabMinSize = 7.0f;
	style->PopupRounding = 2.0f;
	style->ScrollbarRounding = 12.0f;
	style->ScrollbarSize = 13.0f;
	style->TabBorderSize = 1.0f;
	style->TabRounding = 0.0f;
	style->WindowRounding = 0.0f;

	// Merge in icons from Font Awesome and Fork Awesome
	ImGui::GetIO().Fonts->AddFontDefault();
	ImFontConfig iconsConfig;
	iconsConfig.MergeMode = true;
	iconsConfig.PixelSnapH = true;

	ImGui::GetIO().Fonts->AddFontFromFileTTF("Fonts/" FONT_ICON_FILE_NAME_FAS, 12.0f, &iconsConfig, iconsRangesFa);
	ImGui::GetIO().Fonts->AddFontFromFileTTF("Fonts/" FONT_ICON_FILE_NAME_FK, 12.0f, &iconsConfig, iconsRangesFk);
}

bool ModuleEditor::Init() {
	ImGui::CreateContext();
	FileDialog::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		io.ConfigViewportsNoAutoMerge = false;
		io.ConfigViewportsNoTaskBarIcon = true;
	}

	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
		io.ConfigDockingTransparentPayload = true;
	}

	ApplyCustomStyle();

	return true;
}

bool ModuleEditor::Start() {
	ImGui_ImplSDL2_InitForOpenGL(App->window->window, App->renderer->context);
	ImGui_ImplOpenGL3_Init(GLSL_VERSION);

	// Define Color Style
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 1.0f, 0.10f));
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4) ImColor(0.54f, 0.43f, 0.12f, 0.97f));

	panels.push_back(&panelScene);
	panels.push_back(&panelConsole);
	panels.push_back(&panelProject);
	panels.push_back(&panelConfiguration);
	panels.push_back(&panelHierarchy);
	panels.push_back(&panelInspector);
	panels.push_back(&panelAbout);
	panels.push_back(&panelControlEditor);
	panels.push_back(&panelNavigation);
	panels.push_back(&panelResource);
	panels.push_back(&panelGameControllerDebug);
	panels.push_back(&panelImportOptions);
	panels.push_back(&panelAudioMixer);

	return true;
}

UpdateStatus ModuleEditor::PreUpdate() {
	BROFILER_CATEGORY("ModuleEditor - PreUpdate", Profiler::Color::Azure)

#if GAME
	return UpdateStatus::CONTINUE;
#else

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(App->window->window);
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	return UpdateStatus::CONTINUE;
#endif
}

UpdateStatus ModuleEditor::Update() {
	BROFILER_CATEGORY("ModuleEditor - Update", Profiler::Color::Azure)

	ImGui::CaptureMouseFromApp(true);
	ImGui::CaptureKeyboardFromApp(true);

#if GAME
	return UpdateStatus::CONTINUE;
#endif

	// Main menu bar
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("New Scene")) {
			modalToOpen = Modal::NEW_SCENE;
		}
		if (ImGui::MenuItem("Load Scene")) {
			modalToOpen = Modal::LOAD_SCENE;
		}
		if (ImGui::MenuItem("Save Scene", nullptr, false, !App->time->HasGameStarted())) {
			modalToOpen = Modal::SAVE_SCENE;
		}
		if (ImGui::MenuItem("Quit")) {
			modalToOpen = Modal::QUIT;
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Assets")) {
		if (ImGui::BeginMenu("Create")) {
			if (ImGui::MenuItem("Material")) {
				modalToOpen = Modal::CREATE_MATERIAL;
			}
			if (ImGui::MenuItem("Script")) {
				modalToOpen = Modal::CREATE_SCRIPT;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("View")) {
		ImGui::MenuItem(panelScene.name, "", &panelScene.enabled);
		ImGui::MenuItem(panelConsole.name, "", &panelConsole.enabled);
		ImGui::MenuItem(panelProject.name, "", &panelProject.enabled);
		ImGui::MenuItem(panelInspector.name, "", &panelInspector.enabled);
		ImGui::MenuItem(panelHierarchy.name, "", &panelHierarchy.enabled);
		ImGui::MenuItem(panelConfiguration.name, "", &panelConfiguration.enabled);
		ImGui::MenuItem(panelControlEditor.name, "", &panelControlEditor.enabled);
		ImGui::MenuItem(panelNavigation.name, "", &panelNavigation.enabled);
		ImGui::MenuItem(panelResource.name, "", &panelResource.enabled);
		ImGui::MenuItem(panelGameControllerDebug.name, "", &panelGameControllerDebug.enabled);
		ImGui::MenuItem(panelImportOptions.name, "", &panelImportOptions.enabled);
		ImGui::MenuItem(panelAudioMixer.name, "", &panelAudioMixer.enabled);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Help")) {
		if (ImGui::MenuItem("Repository")) {
			App->RequestBrowser("https://github.com/TBD-org/TBD-Engine/wiki");
		}
		if (ImGui::MenuItem("Download latest")) {
			App->RequestBrowser("https://github.com/TBD-org/TBD-Engine/releases");
		}
		if (ImGui::MenuItem("Report a bug")) {
			App->RequestBrowser("https://github.com/TBD-org/TBD-Engine/issues");
		}
		ImGui::MenuItem(panelAbout.name, "", &panelAbout.enabled);
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();

	// Modals
	switch (modalToOpen) {
	case Modal::NEW_PROJECT:
		ImGui::OpenPopup("New project");
		break;
	case Modal::NEW_SCENE:
		ImGui::OpenPopup("New scene");
		break;
	case Modal::LOAD_SCENE:
		FileDialog::Init("Load scene", false, (AllowedExtensionsFlag::SCENE), SCENES_PATH);
		break;
	case Modal::SAVE_SCENE:
		FileDialog::Init("Save scene", true, (AllowedExtensionsFlag::SCENE), SCENES_PATH);
		break;
	case Modal::SAVE_PREFAB:
		FileDialog::Init("Save prefab", true, (AllowedExtensionsFlag::PREFAB), PREFABS_PATH);
		break;
	case Modal::QUIT:
		ImGui::OpenPopup("Quit");
		break;
	case Modal::COMPONENT_EXISTS:
		ImGui::OpenPopup("Already existing Component");
		break;
	case Modal::CANT_REMOVE_COMPONENT:
		ImGui::OpenPopup("Unable to remove component");
		break;
	case Modal::CREATE_MATERIAL:
		ImGui::OpenPopup("Name the material");
		break;
	case Modal::CREATE_SCRIPT:
		ImGui::OpenPopup("Name the script");
		break;
	case Modal::CREATE_NAVMESH:
		ImGui::OpenPopup("Name the NavMesh");
		break;
	}
	modalToOpen = Modal::NONE;

	ImGui::SetNextWindowSize(ImVec2(260, 100), ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("New scene", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
		ImGui::Text("Do you wish to create a new scene?");
		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::SameLine(ImGui::GetWindowWidth() - 140);
		if (ImGui::Button("New scene")) {
			App->scene->CreateEmptyScene();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine(ImGui::GetWindowWidth() - 60);
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	std::string selectedPath;

	if (FileDialog::OpenDialog("Load scene", selectedPath)) {
		std::string filePath = FileDialog::GetRelativePath(selectedPath.c_str());
		App->scene->LoadScene(filePath.c_str());
		ImGui::CloseCurrentPopup();
	}

	if (FileDialog::OpenDialog("Save scene", selectedPath)) {
		std::string filePath = FileDialog::GetRelativePath(selectedPath.c_str());
		App->scene->SaveScene(filePath.c_str());
		ImGui::CloseCurrentPopup();
	}

	if (FileDialog::OpenDialog("Save prefab", selectedPath)) {
		std::string filePath = FileDialog::GetRelativePath(selectedPath.c_str());
		PrefabImporter::SavePrefab(filePath.c_str(), selectedGameObject);
		ImGui::CloseCurrentPopup();
	}

	ImGui::SetNextWindowSize(ImVec2(260, 100), ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Name the material", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
		static char name[FILENAME_MAX] = "New mat";
		ImGui::InputText("Name##matName", name, IM_ARRAYSIZE(name));
		ImGui::NewLine();
		ImGui::SameLine(ImGui::GetWindowWidth() - 120);
		if (ImGui::Button("Save", ImVec2(50, 20))) {
			std::string path = MATERIALS_PATH "/" + std::string(name) + MATERIAL_EXTENSION;
			MaterialImporter::CreateAndSaveMaterial(path.c_str());
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine(ImGui::GetWindowWidth() - 60);
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::SetNextWindowSize(ImVec2(260, 100), ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Name the script", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
		static char name[FILENAME_MAX] = "New script";
		ImGui::InputText("Name##scriptName", name, IM_ARRAYSIZE(name));
		ImGui::NewLine();
		ImGui::SameLine(ImGui::GetWindowWidth() - 120);
		if (ImGui::Button("Save", ImVec2(50, 20))) {
			App->project->CreateScript(std::string(name));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine(ImGui::GetWindowWidth() - 60);
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::SetNextWindowSize(ImVec2(260, 100), ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Name the NavMesh", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
		static char name[FILENAME_MAX] = "New NavMesh";
		ImGui::InputText("Name##navMeshName", name, IM_ARRAYSIZE(name));
		ImGui::NewLine();
		ImGui::SameLine(ImGui::GetWindowWidth() - 120);
		if (ImGui::Button("Save", ImVec2(50, 20))) {
			NavMesh* navMesh = App->navigation->GetCurrentNavMesh();
			std::string path = std::string(NAVMESH_PATH) + "/" + name + NAVMESH_EXTENSION;
			NavMeshImporter::ExportNavMesh(*navMesh, path.c_str());
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine(ImGui::GetWindowWidth() - 60);
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::SetNextWindowSize(ImVec2(260, 100), ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Quit", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
		ImGui::Text("Do you really want to quit?");
		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::SameLine(ImGui::GetWindowWidth() - 120);
		if (ImGui::Button("Quit", ImVec2(50, 20))) {
			return UpdateStatus::STOP;
		}
		ImGui::SameLine(ImGui::GetWindowWidth() - 60);
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// ALREADY EXISTING COMPONENT MODAL
	ImGui::SetNextWindowSize(ImVec2(400, 120), ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Already existing Component", nullptr, ImGuiWindowFlags_NoResize)) {
		ImGui::PushTextWrapPos();
		ImGui::Text("A Component of this type already exists in this GameObject.\nMultiple instances of this type of Component are not allowed in the same GameObject.");
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// CANT REMOVE COMPONENT MODAL
	ImGui::SetNextWindowSize(ImVec2(400, 120), ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Unable to remove component", nullptr, ImGuiWindowFlags_NoResize)) {
		ImGui::PushTextWrapPos();
		ImGui::Text("This Component cannot be removed.There are other Components in this Game Object that require its functionality.");
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// Docking
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGuiID dockSpaceId = ImGui::GetID("DockSpace");

	if (!ImGui::DockBuilderGetNode(dockSpaceId)) {
		ImGui::DockBuilderAddNode(dockSpaceId);
		ImGui::DockBuilderSetNodeSize(dockSpaceId, viewport->WorkSize);

		dockMainId = dockSpaceId;
		dockUpId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Up, 0.2f, nullptr, &dockMainId);
		ImGui::DockBuilderSetNodeSize(dockUpId, ImVec2(viewport->WorkSize.x, 40));
		dockRightId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, 0.2f, nullptr, &dockMainId);
		dockDownId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Down, 0.3f, nullptr, &dockMainId);
		dockLeftId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Left, 0.25f, nullptr, &dockMainId);
	}

	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	ImGuiWindowFlags dockSpaceWindowFlags = 0;
	dockSpaceWindowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
	dockSpaceWindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", nullptr, dockSpaceWindowFlags);
	ImGui::PopStyleVar(3);
	ImGui::DockSpace(dockSpaceId);
	ImGui::End();

	// Panels
	for (Panel* panel : panels) {
		if (panel->enabled) {
			panel->Update();
		}
	}

	return UpdateStatus::CONTINUE;
}

UpdateStatus ModuleEditor::PostUpdate() {
	BROFILER_CATEGORY("ModuleEditor - PostUpdate", Profiler::Color::Azure)

#if GAME
	return UpdateStatus::CONTINUE;
#endif //  GAME

	// Deleting Components
	if (panelInspector.GetComponentToDelete()) {
		if (panelInspector.GetComponentToDelete()->CanBeRemoved()) {
			selectedGameObject->RemoveComponent(panelInspector.GetComponentToDelete());
		} else {
			App->editor->modalToOpen = Modal::CANT_REMOVE_COMPONENT;
		}
		panelInspector.SetComponentToDelete(nullptr);
	}

	// Draw to default frame buffer (main window)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Render main window
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Handle and render other windows
	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault();
	SDL_GL_MakeCurrent(App->window->window, App->renderer->context);

	return UpdateStatus::CONTINUE;
}

bool ModuleEditor::CleanUp() {
	panels.clear();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	FileDialog::DestroyContext();
	ImGui::DestroyContext();

	return true;
}

void ModuleEditor::OnMouseClicked() {
	TesseractEvent mouseEvent = TesseractEvent(TesseractEventType::MOUSE_CLICKED);
	App->events->AddEvent(mouseEvent);
}

void ModuleEditor::OnMouseReleased() {
	TesseractEvent mouseEvent = TesseractEvent(TesseractEventType::MOUSE_RELEASED);
	App->events->AddEvent(mouseEvent);
}

void ModuleEditor::HelpMarker(const char* text) {
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(text);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}