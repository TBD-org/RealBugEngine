#pragma once

#include "Module.h"
#include "Utils/UID.h"
#include "Math/float2.h"
#include "Math/float4.h"

#define SCENE_SCREEN_FACTOR 0.01f

class GameObject;
class ComponentEventSystem;
class ResourceFont;

struct Character;

class ModuleUserInterface : public Module {
public:
	bool Init() override;						   // Inits the module
	bool Start() override;						   // Adds event listeners: SCREEN_RESIZED, MOUSE_CLICKED, MOUSE_RELEASED
	UpdateStatus Update() override;				   // Calls PointerEnter and PointerExit of Components if eventSystem isn't null
	bool CleanUp() override;					   // Deletes generic image VBO (used in ComponentImage)
	void ReceiveEvent(TesseractEvent& e) override; // Treats the events that is listening to.

	Character GetCharacter(UID font, char c);																	// Returns the Character that matches the given one in the given font or null otherwise.
	void GetCharactersInString(UID font, const std::string& sentence, std::vector<Character>& charsInSentence); // Fills the given vector with the glyphs of the given font to form the given sentence.
	void Render();																								// Calls every ComponentCanvasRenderer Render function if the parent is active in hierarchy
	void RecursiveRender(const GameObject* obj);
	void SetCurrentEventSystem(UID id_);		   // Sets the new event system
	ComponentEventSystem* GetCurrentEventSystem(); // Returns the Module's Event System. If the UID is 0, returns nullptr

	unsigned int GetQuadVBO();	// Returns the generic VBO that is used in ComponentImages
	void ViewportResized();	// Calls ComponentCanvas, ComponentTransform2D, ComponentText components to be updated
	bool IsUsing2D() const;

public:
	bool view2DInternal = false;

	float4 GetErrorColor(); // Gets the representation of the color

private:
	void CreateQuadVBO(); // Creates a vbo made by two triangles centered that form a Quad

private:
	UID currentEvSys = 0;						// Module's Event System UID
	unsigned int quadVBO = 0;					// VBO of the ComponentImage generic Quad
	float4 errorColor = float4(-1, -1, -1, -1); // Representation of error in color (not a color to display)
};
