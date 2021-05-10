#include "PanelHierarchy.h"

#include "Application.h"
#include "Utils/Logging.h"
#include "GameObject.h"
#include "FileSystem/PrefabImporter.h"
#include "Components/ComponentTransform.h"
#include "Components/ComponentBoundingBox2D.h"
#include "Components/UI/ComponentTransform2D.h"
#include "Components/UI/ComponentCanvas.h"
#include "Components/UI/ComponentCanvasRenderer.h"
#include "Components/UI/ComponentImage.h"
#include "Components/UI/ComponentSelectable.h"
#include "Components/UI/ComponentEventSystem.h"
#include "Components/UI/ComponentButton.h"
#include "Components/UI/ComponentSlider.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleUserInterface.h"
#include "Modules/ModuleResources.h"
#include "Resources/ResourcePrefab.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

#include "Utils/Leaks.h"

static ImVec4 grey = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
static ImVec4 white = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

PanelHierarchy::PanelHierarchy()
	: Panel("Hierarchy", true) {}

void PanelHierarchy::Update() {
	ImGui::SetNextWindowDockID(App->editor->dockLeftId, ImGuiCond_FirstUseEver);
	std::string windowName = std::string(ICON_FA_SITEMAP " ") + name;
	if (ImGui::Begin(windowName.c_str(), &enabled)) {
		GameObject* root = App->scene->scene->root;
		if (root != nullptr) {
			UpdateHierarchyNode(root);
		}
	}
	ImGui::End();
}

void PanelHierarchy::UpdateHierarchyNode(GameObject* gameObject) {
	const std::vector<GameObject*>& children = gameObject->GetChildren();

	char label[160];
	sprintf_s(label, "%s###%p", gameObject->name.c_str(), gameObject);

	ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
	ImGuiTreeNodeFlags flags = baseFlags;

	if (children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
	bool isSelected = App->editor->selectedGameObject == gameObject;
	if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

	//White for active gameobjects, gray for disabled objects, if a parent is not active, children are inherently not active
	ImGui::PushStyleColor(0, gameObject->IsActive() ? white : grey);

	bool open = ImGui::TreeNodeEx(label, flags);

	ImGui::PopStyleColor();

	ImGui::PushID(label);
	if (ImGui::BeginPopupContextItem("Options")) {
		App->editor->selectedGameObject = gameObject;
		Scene* scene = App->scene->scene;
		if (gameObject != scene->root) {
			if (ImGui::Selectable("Create Prefab")) {
				std::string path = std::string(PREFABS_PATH) + "/" + gameObject->name + PREFAB_EXTENSION;
				for (unsigned copyIndex = 0; App->files->Exists(path.c_str()); ++copyIndex) {
					path = std::string(PREFABS_PATH) + "/" + gameObject->name + " (" + std::to_string(copyIndex) + ")" + PREFAB_EXTENSION;
				}
				PrefabImporter::SavePrefab(path.c_str(), gameObject);
			}

			if (ImGui::Selectable("Delete")) {
				if (isSelected) App->editor->selectedGameObject = nullptr;
				App->scene->DestroyGameObjectDeferred(gameObject);
				if (App->userInterface->GetCurrentEventSystem()) {
					App->userInterface->GetCurrentEventSystem()->SetSelected(0);
				}
			}

			if (ImGui::Selectable("Duplicate")) {
				scene->DuplicateGameObject(gameObject, gameObject->GetParent());
			}

			ImGui::Separator();
		}

		if (ImGui::Selectable("Create Empty")) {
			CreateEmptyGameObject(gameObject);
		}
		if (ImGui::Selectable("Particle System")) {
			CreatePartycleSystemObject(gameObject);
		}
		// TODO: code duplicated in every CreateXX(gameObject). Generalisation could be done here. Also with PanelInspector->AddUIComponentsOptions()
		if (ImGui::BeginMenu("UI")) {
			if (ImGui::MenuItem("Event System")) {
				if (App->scene->scene->eventSystemComponents.Count() == 0) {
					CreateEventSystem(gameObject);
				} else {
					App->editor->modalToOpen = Modal::COMPONENT_EXISTS;
				}
			}

			if (ImGui::MenuItem("Canvas")) {
				CreateUICanvas(gameObject);
			}

			if (ImGui::MenuItem("Image")) {
				CreateUIImage(gameObject);
			}

			if (ImGui::MenuItem("Text")) {
				CreateUIText(gameObject);
			}

			if (ImGui::MenuItem("Button")) {
				CreateUIButton(gameObject);
			}

			if (ImGui::MenuItem("Slider")) {
				CreateUISlider(gameObject);
			}
			if (ImGui::MenuItem("Toggle")) {
				CreateUIToggle(gameObject);
			}

			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}
	ImGui::PopID();

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) {
		App->editor->selectedGameObject = gameObject;
	}

	if (ImGui::BeginDragDropSource()) {
		UID id = gameObject->GetID();
		ImGui::SetDragDropPayload("_HIERARCHY", &id, sizeof(UID));
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget()) {
		// Hierarchy movements
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_HIERARCHY")) {
			UID payloadGameObjectId = *(UID*) payload->Data;
			GameObject* payloadGameObject = App->scene->scene->GetGameObject(payloadGameObjectId);
			if (!gameObject->IsDescendantOf(payloadGameObject)) {
				// First of all, set the new Parent for the object
				payloadGameObject->SetParent(gameObject);

				ComponentTransform* transform = payloadGameObject->GetComponent<ComponentTransform>();
				float4x4 childGlobalMatrix = transform->GetGlobalMatrix();
				transform->SetGlobalTRS(childGlobalMatrix);
			}
		}

		// Prefabs
		std::string prafabPayloadType = std::string("_RESOURCE_") + GetResourceTypeName(ResourceType::PREFAB);
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(prafabPayloadType.c_str())) {
			UID prefabId = *(UID*) payload->Data;
			ResourcePrefab* prefab = App->resources->GetResource<ResourcePrefab>(prefabId);
			if (prefab != nullptr) {
				prefab->BuildPrefab(gameObject);
			}
		}
		ImGui::EndDragDropTarget();
	}

	if (open) {
		for (unsigned i = 0; i < children.size(); i++) {
			UpdateHierarchyNode(children[i]);
		}
		ImGui::TreePop();
	}
}

GameObject* PanelHierarchy::CreateEmptyGameObject(GameObject* gameObject) {
	GameObject* newGameObject = App->scene->scene->CreateGameObject(gameObject, GenerateUID(), "Game Object");
	ComponentTransform* transform = newGameObject->CreateComponent<ComponentTransform>();
	transform->SetPosition(float3(0, 0, 0));
	transform->SetRotation(Quat::identity);
	transform->SetScale(float3(1, 1, 1));
	newGameObject->InitComponents();
	return newGameObject;
}

GameObject* PanelHierarchy::CreateEventSystem(GameObject* gameObject) {
	if (App->userInterface->GetCurrentEventSystem() == nullptr) {
		GameObject* newGameObject = App->scene->scene->CreateGameObject(gameObject, GenerateUID(), "Event System");
		newGameObject->CreateComponent<ComponentTransform>();
		ComponentEventSystem* component = newGameObject->CreateComponent<ComponentEventSystem>();
		App->userInterface->SetCurrentEventSystem(component->GetID());
		newGameObject->InitComponents();
		return newGameObject;
	}

	return nullptr;
}

GameObject* PanelHierarchy::CreateUICanvas(GameObject* gameObject) {
	GameObject* newGameObject = App->scene->scene->CreateGameObject(gameObject, GenerateUID(), "Canvas");
	ComponentTransform* transform = newGameObject->CreateComponent<ComponentTransform>();
	ComponentTransform2D* transform2D = newGameObject->CreateComponent<ComponentTransform2D>();
	ComponentCanvas* canvas = newGameObject->CreateComponent<ComponentCanvas>();
	newGameObject->InitComponents();
	CreateEventSystem(App->scene->scene->root);
	return newGameObject;
}

GameObject* PanelHierarchy::CreateUIImage(GameObject* gameObject) {
	if (gameObject->HasComponentInAnyParent<ComponentCanvas>(gameObject) == nullptr) {
		gameObject = CreateUICanvas(gameObject);
	}

	GameObject* newGameObject = App->scene->scene->CreateGameObject(gameObject, GenerateUID(), "Image");
	ComponentTransform* transform = newGameObject->CreateComponent<ComponentTransform>();
	ComponentTransform2D* transform2D = newGameObject->CreateComponent<ComponentTransform2D>();
	ComponentCanvasRenderer* canvasRenderer = newGameObject->CreateComponent<ComponentCanvasRenderer>();
	ComponentImage* image = newGameObject->CreateComponent<ComponentImage>();
	newGameObject->InitComponents();

	return newGameObject;
}

GameObject* PanelHierarchy::CreateUIText(GameObject* gameObject) {
	if (gameObject->HasComponentInAnyParent<ComponentCanvas>(gameObject) == nullptr) {
		gameObject = CreateUICanvas(gameObject);
	}

	GameObject* newGameObject = App->scene->scene->CreateGameObject(gameObject, GenerateUID(), "Text");
	ComponentTransform* transform = newGameObject->CreateComponent<ComponentTransform>();
	ComponentTransform2D* transform2D = newGameObject->CreateComponent<ComponentTransform2D>();
	ComponentCanvasRenderer* canvasRenderer = newGameObject->CreateComponent<ComponentCanvasRenderer>();
	ComponentText* text = newGameObject->CreateComponent<ComponentText>();
	newGameObject->InitComponents();

	return newGameObject;
}

GameObject* PanelHierarchy::CreateUIButton(GameObject* gameObject) {
	if (gameObject->HasComponentInAnyParent<ComponentCanvas>(gameObject) == nullptr) {
		gameObject = CreateUICanvas(gameObject);
	}

	GameObject* newGameObject = App->scene->scene->CreateGameObject(gameObject, GenerateUID(), "Button");
	ComponentTransform* transform = newGameObject->CreateComponent<ComponentTransform>();
	ComponentTransform2D* transform2D = newGameObject->CreateComponent<ComponentTransform2D>();
	ComponentCanvasRenderer* canvasRenderer = newGameObject->CreateComponent<ComponentCanvasRenderer>();
	ComponentBoundingBox2D* boundingBox = newGameObject->CreateComponent<ComponentBoundingBox2D>();
	ComponentSelectable* selectable = newGameObject->CreateComponent<ComponentSelectable>();
	ComponentButton* button = newGameObject->CreateComponent<ComponentButton>();
	ComponentImage* image = newGameObject->CreateComponent<ComponentImage>();
	CreateEventSystem(App->scene->scene->root);

	selectable->SetSelectableType(button->GetType());
	newGameObject->InitComponents();

	return newGameObject;
}

GameObject* PanelHierarchy::CreatePartycleSystemObject(GameObject* gameObject) {
	GameObject* newGameObject = App->scene->scene->CreateGameObject(gameObject, GenerateUID(), "ParticleSystem");
	ComponentTransform* transform = newGameObject->CreateComponent<ComponentTransform>();
	ComponentParticleSystem* particle = newGameObject->CreateComponent<ComponentParticleSystem>();
	transform->SetPosition(float3(0, 0, 0));
	transform->SetRotation(Quat::identity);
	transform->SetScale(float3(1, 1, 1));
	newGameObject->InitComponents();

	return newGameObject;
}
GameObject* PanelHierarchy::CreateUIToggle(GameObject* gameObject) {
	if (gameObject->HasComponentInAnyParent<ComponentCanvas>(gameObject) == nullptr) {
		gameObject = CreateUICanvas(gameObject);
	}
	GameObject* newGameObject = App->scene->scene->CreateGameObject(gameObject, GenerateUID(), "Toggle");
	ComponentTransform* transform = newGameObject->CreateComponent<ComponentTransform>();
	ComponentTransform2D* transform2D = newGameObject->CreateComponent<ComponentTransform2D>();
	ComponentCanvasRenderer* canvasRenderer = newGameObject->CreateComponent<ComponentCanvasRenderer>();
	ComponentBoundingBox2D* boundingBox = newGameObject->CreateComponent<ComponentBoundingBox2D>();
	ComponentSelectable* selectable = newGameObject->CreateComponent<ComponentSelectable>();
	ComponentToggle* toggle = newGameObject->CreateComponent<ComponentToggle>();
	ComponentImage* image = newGameObject->CreateComponent<ComponentImage>();

	CreateEventSystem(App->scene->scene->root);

	//Child Image
	GameObject* newGameObjectChild = CreateUIImage(newGameObject);
	newGameObjectChild->InitComponents();
	newGameObjectChild->GetComponent<ComponentTransform2D>()->SetSize(transform2D->GetSize() / 2);
	newGameObjectChild->GetComponent<ComponentImage>()->SetColor(float4::zero);
	newGameObjectChild->name = "Checkmark";

	toggle->SetEnabledImageObj(newGameObjectChild->GetID());
	selectable->SetSelectableType(toggle->GetType());
	newGameObject->InitComponents();

	return newGameObject;
}

GameObject* PanelHierarchy::CreateUISlider(GameObject* gameObject) {
	if (gameObject->HasComponentInAnyParent<ComponentCanvas>(gameObject) == nullptr) {
		gameObject = CreateUICanvas(gameObject);
	}

	GameObject* newGameObject = App->scene->scene->CreateGameObject(gameObject, GenerateUID(), "Slider");
	ComponentTransform* transform = newGameObject->CreateComponent<ComponentTransform>();
	ComponentTransform2D* transform2D = newGameObject->CreateComponent<ComponentTransform2D>();
	ComponentCanvasRenderer* canvasRenderer = newGameObject->CreateComponent<ComponentCanvasRenderer>();
	ComponentBoundingBox2D* boundingBox = newGameObject->CreateComponent<ComponentBoundingBox2D>();
	ComponentSelectable* selectable = newGameObject->CreateComponent<ComponentSelectable>();
	ComponentSlider* slider = newGameObject->CreateComponent<ComponentSlider>();

	CreateEventSystem(App->scene->scene->root);

	GameObject* backgroundGameObject = App->scene->scene->CreateGameObject(newGameObject, GenerateUID(), "Background");
	ComponentTransform* backgroundTransform = backgroundGameObject->CreateComponent<ComponentTransform>();
	ComponentTransform2D* backgroundTransform2D = backgroundGameObject->CreateComponent<ComponentTransform2D>();
	ComponentCanvasRenderer* backgroundRenderer = backgroundGameObject->CreateComponent<ComponentCanvasRenderer>();
	ComponentImage* backgorundImage = backgroundGameObject->CreateComponent<ComponentImage>();

	GameObject* fillGameObject = App->scene->scene->CreateGameObject(newGameObject, GenerateUID(), "Fill");
	ComponentTransform* fillTransform = fillGameObject->CreateComponent<ComponentTransform>();
	ComponentTransform2D* fillTransform2D = fillGameObject->CreateComponent<ComponentTransform2D>();
	ComponentCanvasRenderer* fillRenderer = fillGameObject->CreateComponent<ComponentCanvasRenderer>();
	ComponentImage* fillImage = fillGameObject->CreateComponent<ComponentImage>();

	GameObject* handleGameObject = App->scene->scene->CreateGameObject(newGameObject, GenerateUID(), "Handle");
	ComponentTransform* handleTransform = handleGameObject->CreateComponent<ComponentTransform>();
	ComponentTransform2D* handleTransform2D = handleGameObject->CreateComponent<ComponentTransform2D>();
	ComponentCanvasRenderer* handleRenderer = handleGameObject->CreateComponent<ComponentCanvasRenderer>();
	ComponentImage* handleImage = handleGameObject->CreateComponent<ComponentImage>();


	selectable->SetSelectableType(slider->GetType());
	backgroundGameObject->InitComponents();
	fillGameObject->InitComponents();
	handleGameObject->InitComponents();

	newGameObject->InitComponents();

	return newGameObject;
}