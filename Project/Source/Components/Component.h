#pragma once

#include "Components/ComponentType.h"
#include "FileSystem/JsonValue.h"
#include "Utils/UID.h"

class GameObject;

#if defined(TESSERACT_ENGINE_API)
/* do nothing. */
#elif defined(_MSC_VER)
#define TESSERACT_ENGINE_API __declspec(dllexport)
#endif

class Component {
public:
	Component(ComponentType type, GameObject* owner, UID id, bool active);
	virtual ~Component();

	// ------- Core Functions ------ //
	virtual void Init();								// Performs the initialisations of the Component that are detached from the constructora. Called from owner->InitComponents().
	virtual void Update();								// Updates the Component at each frame. Called on owner->Update()
	virtual void DrawGizmos();							// Draws the visual representation of the component in the screen (if exists, I.E. Light direction or camera frustum).
	virtual void OnEditorUpdate();						// Draw the ImGui elements & info of the Component in the Inspector. Called from PanelInspector->Update()
	virtual void Save(JsonValue jComponent) const;		// Operations to serialise this Component when saving the scene. Called from owner->Save().
	virtual void Load(JsonValue jComponent);			// Operations to initialise this Component when a scene is loaded. Called from owner->Load().
	virtual bool CanBeRemoved() const;					// Used in inspectorPanel to check if there are any dependencies that "forbid" the component from being removed

	// ---- Visibility Setters ----- //
	TESSERACT_ENGINE_API void Enable();
	TESSERACT_ENGINE_API void Disable();

	// ---- Virtual Enable/Disable callbacks ----//
	virtual void OnEnable() {}
	virtual void OnDisable() {}

	// ---------- Getters ---------- //
	TESSERACT_ENGINE_API ComponentType GetType() const;
	TESSERACT_ENGINE_API GameObject& GetOwner() const;
	TESSERACT_ENGINE_API UID GetID() const;
	TESSERACT_ENGINE_API bool IsActive() const;
	bool IsActiveInternal() const;

protected:
	ComponentType type = ComponentType::UNKNOWN; // See ComponentType.h for a list of all available types.
	bool active = true;							 // Visibility of the Component. If active is false the GameObject behaves as if this Component doesn't exist.

private:
	UID id = 0;					 // Unique identifier for the component
	GameObject* owner = nullptr; // References the GameObject this Component applies its functionality to. Its 'parent'.
};
