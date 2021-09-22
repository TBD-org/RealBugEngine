#include "ModuleUserInterface.h"

#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleEvents.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModuleInput.h"
#include "Modules/ModuleScene.h"
#include "Components/UI/ComponentCanvas.h"
#include "Components/UI/ComponentCanvasRenderer.h"
#include "Components/UI/ComponentSelectable.h"
#include "Components/UI/ComponentEventSystem.h"
#include "Components/UI/ComponentBoundingBox2D.h"
#include "Resources/ResourceFont.h"
#include "UI/Interfaces/IPointerEnterHandler.h"
#include "UI/Interfaces/IPointerExitHandler.h"
#include "UI/Interfaces/IMouseClickHandler.h"
#include "Scene.h"
#include "TesseractEvent.h"

#include "Geometry/LineSegment.h"
#include "GL/glew.h"

#include "Utils/Leaks.h"

bool ModuleUserInterface::Init() {
#if GAME
	view2DInternal = true;
#endif
	return true;
}

bool ModuleUserInterface::Start() {
	CreateQuadVBO();
	App->events->AddObserverToEvent(TesseractEventType::SCREEN_RESIZED, this);
	App->events->AddObserverToEvent(TesseractEventType::MOUSE_CLICKED, this);
	App->events->AddObserverToEvent(TesseractEventType::MOUSE_RELEASED, this);
	App->events->AddObserverToEvent(TesseractEventType::PRESSED_PLAY, this);
	ViewportResized();
	return true;
}

UpdateStatus ModuleUserInterface::Update() {
	float2 mousePos = App->input->GetMousePosition(true);

	if (currentEvSys != 0) {
		for (ComponentSelectable& selectable : App->scene->GetCurrentScene()->selectableComponents) {
			ComponentBoundingBox2D* bb = selectable.GetOwner().GetComponent<ComponentBoundingBox2D>();

			if (bb) {
				if (!selectable.IsHovered()) {
					if (bb->GetWorldAABB().Contains(mousePos)) {
						selectable.OnPointerEnter();
					}
				} else {
					if (!bb->GetWorldAABB().Contains(mousePos)) {
						selectable.OnPointerExit();
					}
				}
			}
		}
		ComponentEventSystem* currEvSys = App->userInterface->GetCurrentEventSystem();
		if (currEvSys) {
			ComponentSelectable* currentlySelected = GetCurrentEventSystem()->GetCurrentSelected();
			if (currentlySelected) {
				ManageInputsOnSelected(currentlySelected);
			}
		}
	}

	return UpdateStatus::CONTINUE;
}

void ModuleUserInterface::ManageInputsOnSelected(ComponentSelectable* currentlySelected) {
	bool pressingOnSelected = App->input->GetKey(SDL_SCANCODE_RETURN) == KeyState::KS_REPEAT
							  || App->input->GetKey(SDL_SCANCODE_RETURN) == KeyState::KS_DOWN
							  || (App->input->GetPlayerController(0)
								  && (App->input->GetPlayerController(0)->GetButtonState(SDL_CONTROLLER_BUTTON_A) == KeyState::KS_REPEAT
									  || App->input->GetPlayerController(0)->GetButtonState(SDL_CONTROLLER_BUTTON_A) == KeyState::KS_DOWN));

	ComponentSlider* slider = currentlySelected->GetOwner().GetComponent<ComponentSlider>();

	if (!slider) {
		if (pressingOnSelected) {
			if (currentlySelected->IsClicked()) {
				if (!wasPressConfirmed) {
					currentlySelected->TryToClickOn(false);
					wasPressConfirmed = true;
					//CONFIRM PRESS
				}
			} else {
				currentlySelected->TryToClickOn(true);
				//SETCLICKEDINTERNAL
			}
		} else {
			wasPressConfirmed = false;
		}

		//Sliders are handled separately, all other UI components can be managed through this code
	} else {
		//TODO MANAGE BEING HANDLED BOOl

		if (pressingOnSelected) {
			if (!wasPressConfirmed) {
				wasPressConfirmed = true;
				handlingSlider = slider->beingHandled = !slider->beingHandled;
			}
		} else {
			wasPressConfirmed = false;
		}

		if (!slider->beingHandled) return;

		float directionToMoveSlider = 0;
		if (App->input->GetPlayerController(0)) {
			directionToMoveSlider = App->input->GetPlayerController(0)->GetAxisNormalized(SDL_CONTROLLER_AXIS_LEFTX);
			if (directionToMoveSlider == 0) {
				if (App->input->GetPlayerController(0)->GetButtonState(SDL_CONTROLLER_BUTTON_DPAD_LEFT) == KeyState::KS_DOWN || App->input->GetPlayerController(0)->GetButtonState(SDL_CONTROLLER_BUTTON_DPAD_LEFT) == KeyState::KS_REPEAT) {
					directionToMoveSlider = -1.0f;
				} else if (App->input->GetPlayerController(0)->GetButtonState(SDL_CONTROLLER_BUTTON_DPAD_RIGHT) == KeyState::KS_DOWN || App->input->GetPlayerController(0)->GetButtonState(SDL_CONTROLLER_BUTTON_DPAD_RIGHT) == KeyState::KS_REPEAT) {
					directionToMoveSlider = 1.0f;
				}
			}
		}

		if (directionToMoveSlider == 0) {
			if (App->input->GetKey(SDL_SCANCODE_LEFT) == KeyState::KS_DOWN) {
				directionToMoveSlider = -1.0f;
			} else if (App->input->GetKey(SDL_SCANCODE_RIGHT) == KeyState::KS_DOWN) {
				directionToMoveSlider = 1.0f;
			}
		}
		if (directionToMoveSlider != 0) {
			slider->ModifyValue(directionToMoveSlider);
		}
	}
}

//This is done like this because receiving a WINDOW RESIZE event from SDL takes one frame to take effect, so instead of directly calling OnViewportResized,
//The behaviour implemented sets viewportWasResized to True, and the next PostUpdate iteration, when SDL event has taken effect, OnViewportResized is called
UpdateStatus ModuleUserInterface::PostUpdate() {
	if (viewportWasResized) {
		viewportWasResized = false;
		OnViewportResized();
	}
	return UpdateStatus::CONTINUE;
}

bool ModuleUserInterface::CleanUp() {
	glDeleteBuffers(1, &quadVBO);
	return true;
}

void ModuleUserInterface::ReceiveEvent(TesseractEvent& e) {
	ComponentEventSystem* eventSystem = GetCurrentEventSystem();
	switch (e.type) {
	case TesseractEventType::SCREEN_RESIZED:
		ViewportResized();
		break;

	case TesseractEventType::MOUSE_CLICKED:
		if (eventSystem != nullptr) {
			ComponentSelectable* lastHoveredSelectable = eventSystem->GetCurrentlyHovered();
			if (lastHoveredSelectable != nullptr) {
				if (lastHoveredSelectable->IsInteractable()) {
					eventSystem->SetClickedGameObject(&lastHoveredSelectable->GetOwner());
					lastHoveredSelectable->TryToClickOn(true);
				}
			} else {
				//Set selected to null
				eventSystem->SetSelected(nullptr);
			}
		}
		break;

	case TesseractEventType::MOUSE_RELEASED:
		if (eventSystem != nullptr) {
			ComponentSelectable* lastHoveredSelectable = eventSystem->GetCurrentlyHovered();
			if (lastHoveredSelectable != nullptr) {
				if (&lastHoveredSelectable->GetOwner() == eventSystem->GetClickedGameObject()) {
					lastHoveredSelectable->TryToClickOn(false);
					eventSystem->SetSelected(nullptr);
				}
			}
		}
		break;
	case TesseractEventType::PRESSED_PLAY:
		//This is done to prevent the fact that (in editor) disabling internal 2D and pressing play generates a visual error
		ViewportResized();
		break;
	default:
		break;
	}
}

void ModuleUserInterface::RecursiveRender(const GameObject* obj) {
	ComponentCanvasRenderer* renderer = obj->GetComponent<ComponentCanvasRenderer>();

	if (obj->IsActive()) {
		if (renderer && renderer->IsActive()) {
			renderer->Render(obj);
		}

		for (const GameObject* child : obj->GetChildren()) {
			RecursiveRender(child);
		}
	}
}

void ModuleUserInterface::Render() {
	Scene* scene = App->scene->GetCurrentScene();
	if (scene != nullptr) {
		RecursiveRender(scene->root);
	}
}

unsigned int ModuleUserInterface::GetQuadVBO() {
	return quadVBO;
}

void ModuleUserInterface::CreateQuadVBO() {
	float buffer_data[] = {
		-0.5f,
		-0.5f,
		0.0f, //  v0 pos

		0.5f,
		-0.5f,
		0.0f, // v1 pos

		-0.5f,
		0.5f,
		0.0f, //  v2 pos

		0.5f,
		-0.5f,
		0.0f, //  v3 pos

		0.5f,
		0.5f,
		0.0f, // v4 pos

		-0.5f,
		0.5f,
		0.0f, //  v5 pos

		0.0f,
		0.0f, //  v0 texcoord

		1.0f,
		0.0f, //  v1 texcoord

		0.0f,
		1.0f, //  v2 texcoord

		1.0f,
		0.0f, //  v3 texcoord

		1.0f,
		1.0f, //  v4 texcoord

		0.0f,
		1.0f //  v5 texcoord
	};

	glGenBuffers(1, &quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO); // set vbo active
	glBufferData(GL_ARRAY_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);
}

void ModuleUserInterface::SetCurrentEventSystem(UID id_) {
	currentEvSys = id_;
}

ComponentEventSystem* ModuleUserInterface::GetCurrentEventSystem() {
	if (currentEvSys == 0) {
		if (App->scene->GetCurrentScene()->eventSystemComponents.Count() > 0) {
			currentEvSys = (*App->scene->GetCurrentScene()->eventSystemComponents.begin()).GetID();
		}
	}

	return currentEvSys == 0 ? nullptr : (ComponentEventSystem*) App->scene->GetCurrentScene()->GetComponentByTypeAndId(ComponentType::EVENT_SYSTEM, currentEvSys);
}

void ModuleUserInterface::ViewportResized() {
	viewportWasResized = true;
}

bool ModuleUserInterface::IsUsing2D() const {
	return view2DInternal || App->time->HasGameStarted();
}

float4 ModuleUserInterface::GetErrorColor() {
	return errorColor;
}

void ModuleUserInterface::OnViewportResized() {
	for (ComponentCanvas& canvas : App->scene->GetCurrentScene()->canvasComponents) {
		canvas.Invalidate();
	}

	for (ComponentTransform2D& transform : App->scene->GetCurrentScene()->transform2DComponents) {
		transform.Invalidate();
	}

	for (ComponentText& text : App->scene->GetCurrentScene()->textComponents) {
		text.Invalidate();
	}
}