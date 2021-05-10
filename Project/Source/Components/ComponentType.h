#pragma once

/* Creating a new component type:
*    1. Add a new ComponentType for the new component
*    2. Add REGISTER_COMPONENT to the .h of the new component
*    3. Add the new component to the GetComponentTypeName and GetComponentTypeFromName functions in ComponentType.cpp
*    4. Create a new PoolMap for the new component in Scene.h
*    5. Initialize the PoolMap in Scene.cpp's Init()
*    6. Add the new component to the functions in Scene.cpp
*    7. Implement DuplicateComponent for the new component
*    8. Add the "Add Component" functionality in PanelInspector.cpp (ln. 62)
*/

// REGISTER_COMPONENT builds the data structures common to all Components.
// This includes the Constructor.
#define REGISTER_COMPONENT(componentClass, componentType, allowMultiple) \
	static const ComponentType staticType = componentType;               \
	static const bool allowMultipleComponents = allowMultiple;           \
	componentClass(GameObject* owner, UID id, bool active) : Component(staticType, owner, id, active) {}

enum class ComponentType {
	UNKNOWN,
	TRANSFORM,
	MESH_RENDERER,
	BOUNDING_BOX,
	CAMERA,
	LIGHT,
	CANVAS,
	CANVASRENDERER,
	IMAGE,
	TRANSFORM2D,
	BUTTON,
	EVENT_SYSTEM,
	BOUNDING_BOX_2D,
	TOGGLE,
	SELECTABLE,
	SLIDER,
	SKYBOX,
	ANIMATION,
	TEXT,
	SCRIPT,
	PARTICLE,
	AUDIO_SOURCE,
	AUDIO_LISTENER
};

const char* GetComponentTypeName(ComponentType type);
ComponentType GetComponentTypeFromName(const char* name);