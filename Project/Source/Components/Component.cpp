#include "Component.h"

#include "Utils/JsonValue.h"
#include "GameObject.h"

#include "Utils/Leaks.h"

Component::Component(ComponentType type_, GameObject* owner_, UID id_, bool active_)
	: type(type_)
	, owner(owner_)
	, id(id_)
	, active(active_) {}

Component::~Component() {}

void Component::Init() {}

void Component::Start() {}

void Component::Update() {}

void Component::DrawGizmos() {}

void Component::OnEditorUpdate() {}

void Component::Save(JsonValue jComponent) const {}

void Component::Load(JsonValue jComponent) {}

bool Component::CanBeRemoved() const {
	return true;
}

void Component::Enable() {
	active = true;
	OnEnable();
}

void Component::Disable() {
	active = false;
	OnDisable();
}

ComponentType Component::GetType() const {
	return type;
}

GameObject& Component::GetOwner() const {
	return *owner;
}

UID Component::GetID() const {
	return id;
}

bool Component::IsActive() const {
	return active && GetOwner().IsActive();
}

bool Component::IsActiveInternal() const {
	return active;
}
