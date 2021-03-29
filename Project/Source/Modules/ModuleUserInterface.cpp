#include "ModuleUserInterface.h"

#include "Application.h"
#include "ModuleFiles.h"
#include "ModuleEventSystem.h"
#include "Utils/FileDialog.h"

#include "GameObject.h"
#include "Components/ComponentCanvas.h"

#include <Components/UI/ComponentSelectable.h>
#include "Components/ComponentEventSystem.h"
#include "Components/ComponentBoundingBox2D.h"
#include "Event.h"

#include "UI/Interfaces/IPointerEnterHandler.h"

#include "Utils/Logging.h"
#include "Utils/Leaks.h"

bool ModuleUserInterface::Init() {
	App->eventSystem->AddObserverToEvent(Event::EventType::MOUSE_UPDATE, this);
	App->eventSystem->AddObserverToEvent(Event::EventType::MOUSE_CLICKED, this);
	return true;
}

void ModuleUserInterface::AddFont(std::string fontPath) {
	//Right now we have FileDialog, it may change in the future
	//FileDialog::GetFileName(fontPath.c_str());

	//OLD VERSION, TO DO USE NEW SYSTEM, AS STATED IN THE PREVIOUS COMMENT

	//std::string fontName = App->files->GetFileName(fontPath.c_str());
	//std::unordered_map<std::string, std::unordered_map<char, Character>>::const_iterator existsKey = fonts.find(fontName);

	//if (existsKey == fonts.end()) {
	//	std::unordered_map<char, Character> characters;
	//	FontImporter::LoadFont(fontPath, characters);
	//	fonts.insert(std::pair<std::string, std::unordered_map<char, Character>>(fontName, characters));
	//}
}

Character ModuleUserInterface::GetCharacter(std::string font, char c) {
	std::unordered_map<std::string, std::unordered_map<char, Character>>::const_iterator existsKey = fonts.find(font);

	if (existsKey == fonts.end()) {
		return Character();
	}
	return fonts[font][c];
}

void ModuleUserInterface::GetCharactersInString(std::string font, std::string sentence, std::vector<Character>& charsInSentence) {
	std::unordered_map<std::string, std::unordered_map<char, Character>>::const_iterator existsKey = fonts.find(font);

	if (existsKey == fonts.end()) {
		return;
	}

	for (std::string::const_iterator i = sentence.begin(); i != sentence.end(); ++i) {
		charsInSentence.push_back(fonts[font][*i]);
	}
}

void ModuleUserInterface::Render() {
	//GameObject* canvasRenderer = canvas->GetChildren()[0];
	if (canvas != nullptr) {
		//canvas->GetChildren()[0]->GetComponent<ComponentCanvas>()->Render();
		ComponentCanvas* canvasComponent = canvas->GetComponent<ComponentCanvas>();
		if (canvasComponent) {
			canvasComponent->Render();
		}
	}
}

void ModuleUserInterface::StartUI() {
}

void ModuleUserInterface::EndUI() {
}

GameObject* ModuleUserInterface::GetCanvas() const {
	return canvas;
}

void ModuleUserInterface::ReceiveEvent(const Event& e) {
	float2 mousePos = float2(e.point2d.x, e.point2d.y);
	switch (e.type) {
	case Event::EventType::MOUSE_UPDATE:
		if (ComponentEventSystem::currentEvSys) {
			for (ComponentSelectable* selectable : ComponentEventSystem::currentEvSys->m_Selectables) {
				//TODO GET GAMEOBJECT REF

				//ComponentBoundingBox2D* bb = selectable->GetOwner().GetComponent<ComponentBoundingBox2D>();
				//
				//if (!selectable->IsHovered()) {
				//	if (bb->GetWorldAABB().Contains(mousePos)) {
				//		selectable->OnPointerEnter();
				//	}
				//} else {
				//	if (!bb->GetWorldAABB().Contains(mousePos)) {
				//		selectable->OnPointerExit();
				//	}
				//}

				ComponentBoundingBox2D* bb = selectable->GetOwner().GetComponent<ComponentBoundingBox2D>();

				if (!selectable->IsHovered()) {
					if (bb->GetWorldAABB().Contains(mousePos)) {
						selectable->OnPointerEnter();
					}
				} else {
					if (!bb->GetWorldAABB().Contains(mousePos)) {
						selectable->OnPointerExit();
					}
				}
			}
		}
		break;

	case Event::EventType::MOUSE_CLICKED:
		LOG("Clicked Mouse");
		break;
	default:
		break;
	}
}