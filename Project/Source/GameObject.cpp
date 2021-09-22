#include "GameObject.h"

#include "Globals.h"
#include "Components/ComponentType.h"
#include "Components/UI/ComponentCanvas.h"
#include "Components/UI/ComponentCanvasRenderer.h"
#include "Components/UI/ComponentImage.h"
#include "Components/UI/ComponentTransform2D.h"
#include "Components/UI/ComponentBoundingBox2D.h"
#include "Components/UI/ComponentEventSystem.h"
#include "Components/UI/ComponentSelectable.h"
#include "Importers/ModelImporter.h"
#include "Modules/ModuleInput.h"
#include "Modules/ModuleScene.h"
#include "Utils/Logging.h"

#include "Math/myassert.h"
#include "rapidjson/document.h"

#include "Utils/Leaks.h"

#define JSON_TAG_ID "Id"
#define JSON_TAG_NAME "Name"
#define JSON_TAG_ACTIVE "Active"
#define JSON_TAG_STATIC "Static"
#define JSON_TAG_ACTIVEINHIERARCHY "ActiveHierarchy"
#define JSON_TAG_ROOT_BONE_ID "RootBoneId"
#define JSON_TAG_ROOT_BONE_NAME "RootBoneName"
#define JSON_TAG_TYPE "Type"
#define JSON_TAG_COMPONENTS "Components"
#define JSON_TAG_CHILDREN "Children"
#define JSON_TAG_MASK "Mask"

void GameObject::Init() {
	for (Component* component : components) {
		component->Init();
	}

	for (GameObject* child : children) {
		child->Init();
	}
}

void GameObject::Start() {
	for (Component* component : components) {
		component->Start();
	}

	for (GameObject* child : children) {
		child->Start();
	}
}

void GameObject::Update() {
	if (IsActive()) {
		for (Component* component : components) {
			component->Update();
		}

		for (GameObject* child : children) {
			child->Update();
		}
	}
}

void GameObject::DrawGizmos() {
	for (Component* component : components) {
		component->DrawGizmos();
	}

	for (GameObject* child : children) {
		child->DrawGizmos();
	}
}

void GameObject::Enable() {
	active = true;
	EnableInHierarchy();
}

void GameObject::Disable() {
	DisableInHierarchy();
	active = false;
}

bool GameObject::IsActive() const {
	return active && activeInHierarchy;
}

bool GameObject::IsActiveInternal() const {
	return active;
}

void GameObject::SetStatic(bool value) {
	isStatic = value;
}

bool GameObject::IsStatic() const {
	return isStatic;
}

UID GameObject::GetID() const {
	return id;
}

const std::vector<Component*>& GameObject::GetComponents() const {
	return components;
}

void GameObject::RemoveComponent(Component* component) {
	for (auto it = components.begin(); it != components.end(); ++it) {
		if (*it == component) {
			scene->RemoveComponentByTypeAndId((*it)->GetType(), (*it)->GetID());
			components.erase(it);
			break;
		}
	}
}

void GameObject::RemoveAllComponents() {
	while (!components.empty()) {
		Component* component = components.back();
		scene->RemoveComponentByTypeAndId(component->GetType(), component->GetID());
		components.pop_back();
	}
}

void GameObject::SetParent(GameObject* gameObject) {
	if (parent != nullptr) {
		bool found = false;
		for (auto it = parent->children.begin(); it != parent->children.end(); ++it) {
			if (*it == this) {
				found = true;
				parent->children.erase(it);
				break;
			}
		}
		assert(found);
	}
	parent = gameObject;
	if (gameObject != nullptr) {
		gameObject->children.push_back(this);
	}

	// To invalidate hierarchy in UIElements
	ComponentTransform2D* parentTransform2D = this->GetComponent<ComponentTransform2D>();
	if (parentTransform2D != nullptr) {
		parentTransform2D->InvalidateHierarchy();
	}
}

GameObject* GameObject::GetParent() const {
	return parent;
}

void GameObject::SetRootBone(GameObject* gameObject) {
	rootBoneHierarchy = gameObject;
}

GameObject* GameObject::GetRootBone() const {
	return rootBoneHierarchy;
}

void GameObject::AddMask(MaskType mask_) {
	if ((mask.bitMask & static_cast<int>(mask_)) != 0) {
		LOG("Mask already added");
		return;
	}

	switch (mask_) {
	case MaskType::ENEMY:
		mask.bitMask |= static_cast<int>(mask_);
		break;
	case MaskType::PLAYER:
		mask.bitMask |= static_cast<int>(mask_);
		break;
	default:
		LOG("The solicitated mask doesn't exist");
		break;
	}
}

void GameObject::DeleteMask(MaskType mask_) {
	if ((mask.bitMask & static_cast<int>(mask_)) == 0) {
		LOG("Mask already deleted");
		return;
	}

	switch (mask_) {
	case MaskType::ENEMY:
		mask.bitMask ^= static_cast<int>(mask_);
		break;
	case MaskType::PLAYER:
		mask.bitMask ^= static_cast<int>(mask_);
		break;
	default:
		LOG("The solicitated mask doesn't exist");
		break;
	}
}

Mask& GameObject::GetMask() {
	return mask;
}

void GameObject::AddChild(GameObject* gameObject) {
	gameObject->SetParent(this);
}

void GameObject::RemoveChild(GameObject* gameObject) {
	gameObject->SetParent(nullptr);
}

const std::vector<GameObject*>& GameObject::GetChildren() const {
	return children;
}

GameObject* GameObject::GetChild(unsigned index) const {
	return index < children.size() ? children[index] : nullptr;
}

GameObject* GameObject::GetChild(const char* name) const {
	GameObject* ret = nullptr;
	for (std::vector<GameObject*>::const_iterator it = children.begin(); it != children.end() && ret == nullptr; ++it) {
		if ((*it)->name == name) {
			ret = *it;
		}
	}
	return ret;
}

bool GameObject::IsDescendantOf(GameObject* gameObject) {
	if (GetParent() == nullptr) return false;
	if (GetParent() == gameObject) return true;
	return GetParent()->IsDescendantOf(gameObject);
}

GameObject* GameObject::FindDescendant(const std::string& name_) const {
	for (GameObject* child : children) {
		if (child->name == name_) {
			return child;
		} else {
			GameObject* gameObject = child->FindDescendant(name_);
			if (gameObject != nullptr) return gameObject;
		}
	}

	return nullptr;
}

bool GameObject::HasChildren() const {
	return !children.empty();
}

void GameObject::Save(JsonValue jGameObject) const {
	jGameObject[JSON_TAG_ID] = id;
	jGameObject[JSON_TAG_NAME] = name.c_str();
	jGameObject[JSON_TAG_ACTIVE] = active;
	jGameObject[JSON_TAG_STATIC] = isStatic;
	jGameObject[JSON_TAG_ACTIVEINHIERARCHY] = activeInHierarchy;
	jGameObject[JSON_TAG_ROOT_BONE_ID] = rootBoneHierarchy != nullptr ? rootBoneHierarchy->id : 0;
	jGameObject[JSON_TAG_MASK] = mask.bitMask;

	JsonValue jComponents = jGameObject[JSON_TAG_COMPONENTS];
	for (unsigned i = 0; i < components.size(); ++i) {
		JsonValue jComponent = jComponents[i];
		Component* component = components[i];

		jComponent[JSON_TAG_TYPE] = GetComponentTypeName(component->GetType());
		jComponent[JSON_TAG_ID] = component->GetID();
		jComponent[JSON_TAG_ACTIVE] = component->IsActiveInternal();
		component->Save(jComponent);
	}

	JsonValue jChildren = jGameObject[JSON_TAG_CHILDREN];
	for (unsigned i = 0; i < children.size(); ++i) {
		JsonValue jChild = jChildren[i];
		GameObject* child = children[i];
		child->Save(jChild);
	}
}

void GameObject::Load(JsonValue jGameObject) {
	UID newId = jGameObject[JSON_TAG_ID];
	scene->gameObjects.ChangeKey(id, newId);
	id = newId;
	name = jGameObject[JSON_TAG_NAME];
	active = jGameObject[JSON_TAG_ACTIVE];
	isStatic = jGameObject[JSON_TAG_STATIC];
	activeInHierarchy = jGameObject[JSON_TAG_ACTIVEINHIERARCHY];
	mask.bitMask = jGameObject[JSON_TAG_MASK];

	for (unsigned i = 0; i < ARRAY_LENGTH(mask.maskNames); ++i) {
		MaskType type = GetMaskTypeFromName(mask.maskNames[i]);
		if ((mask.bitMask & static_cast<int>(type)) != 0) {
			mask.maskValues[i] = true;
		}
	}

	JsonValue jComponents = jGameObject[JSON_TAG_COMPONENTS];
	for (unsigned i = 0; i < jComponents.Size(); ++i) {
		JsonValue jComponent = jComponents[i];

		std::string typeName = jComponent[JSON_TAG_TYPE];
		UID componentId = jComponent[JSON_TAG_ID];
		bool active = jComponent[JSON_TAG_ACTIVE];

		ComponentType type = GetComponentTypeFromName(typeName.c_str());
		Component* component = scene->CreateComponentByTypeAndId(this, type, componentId);
		if (active) {
			component->Enable();
		} else {
			component->Disable();
		}
		components.push_back(component);
		component->Load(jComponent);

		// Save in the Scene the GameObject with the directional light
		if (type == ComponentType::LIGHT) {
			ComponentLight* light = static_cast<ComponentLight*>(component);
			if (light && light->lightType == LightType::DIRECTIONAL) {
				scene->directionalLight = this;
			}
		} 
	}

	JsonValue jChildren = jGameObject[JSON_TAG_CHILDREN];
	for (unsigned i = 0; i < jChildren.Size(); ++i) {
		JsonValue jChild = jChildren[i];
		std::string childName = jChild[JSON_TAG_NAME];

		GameObject* child = scene->gameObjects.Obtain(0);
		child->scene = scene;
		child->SetParent(this);
		child->Load(jChild);
	}

	UID rootBoneId = jGameObject[JSON_TAG_ROOT_BONE_ID];
	rootBoneHierarchy = scene->GetGameObject(rootBoneId);
	// Recache bones to unordered map
	if (rootBoneHierarchy) {
		std::unordered_map<std::string, GameObject*> temporalBonesMap;
		temporalBonesMap[rootBoneHierarchy->name] = rootBoneHierarchy;
		ModelImporter::CacheBones(rootBoneHierarchy, temporalBonesMap);
		ModelImporter::SaveBones(this, temporalBonesMap);
	}
}

void GameObject::SavePrefab(JsonValue jGameObject) {
	jGameObject[JSON_TAG_NAME] = name.c_str();
	jGameObject[JSON_TAG_ACTIVE] = active;
	jGameObject[JSON_TAG_ROOT_BONE_NAME] = rootBoneHierarchy ? rootBoneHierarchy->name.c_str() : "";
	jGameObject[JSON_TAG_MASK] = mask.bitMask;

	JsonValue jComponents = jGameObject[JSON_TAG_COMPONENTS];
	for (unsigned i = 0; i < components.size(); ++i) {
		Component* component = components[i];
		JsonValue jComponent = jComponents[i];

		jComponent[JSON_TAG_TYPE] = GetComponentTypeName(component->GetType());
		jComponent[JSON_TAG_ACTIVE] = component->IsActive();
		component->Save(jComponent);
	}

	JsonValue jChildren = jGameObject[JSON_TAG_CHILDREN];
	for (unsigned i = 0; i < children.size(); ++i) {
		JsonValue jChild = jChildren[i];
		GameObject* child = children[i];

		child->SavePrefab(jChild);
	}
}

void GameObject::LoadPrefab(JsonValue jGameObject) {
	name = jGameObject[JSON_TAG_NAME];
	active = jGameObject[JSON_TAG_ACTIVE];
	mask.bitMask = jGameObject[JSON_TAG_MASK];

	for (unsigned i = 0; i < ARRAY_LENGTH(mask.maskNames); ++i) {
		MaskType type = GetMaskTypeFromName(mask.maskNames[i]);
		if ((mask.bitMask & static_cast<int>(type)) != 0) {
			mask.maskValues[i] = true;
		}
	}

	JsonValue jComponents = jGameObject[JSON_TAG_COMPONENTS];
	for (unsigned i = 0; i < jComponents.Size(); ++i) {
		JsonValue jComponent = jComponents[i];

		std::string typeName = jComponent[JSON_TAG_TYPE];
		UID componentId = GenerateUID();
		bool active = jComponent[JSON_TAG_ACTIVE];

		ComponentType type = GetComponentTypeFromName(typeName.c_str());
		Component* component = scene->CreateComponentByTypeAndId(this, type, componentId);
		components.push_back(component);
		component->Load(jComponent);
	}

	JsonValue jChildren = jGameObject[JSON_TAG_CHILDREN];
	for (unsigned i = 0; i < jChildren.Size(); ++i) {
		JsonValue jChild = jChildren[i];
		std::string childName = jChild[JSON_TAG_NAME];

		UID childId = GenerateUID();
		GameObject* child = scene->gameObjects.Obtain(childId);
		child->scene = scene;
		child->id = childId;
		child->SetParent(this);
		child->LoadPrefab(jChild);
	}

	std::string rootBoneName = jGameObject[JSON_TAG_ROOT_BONE_NAME];
	if (rootBoneName != "") {
		rootBoneHierarchy = (rootBoneName == this->name) ? this : FindDescendant(rootBoneName);
		// Recache bones to unordered map
		std::unordered_map<std::string, GameObject*> temporalBonesMap;
		temporalBonesMap[rootBoneHierarchy->name] = rootBoneHierarchy;
		ModelImporter::CacheBones(rootBoneHierarchy, temporalBonesMap);
		ModelImporter::SaveBones(this, temporalBonesMap);
	}
}

void GameObject::EnableInHierarchy() {
	if (parent != nullptr && !parent->IsActive()) {
		return;
	}

	activeInHierarchy = true;
	for (GameObject* child : children) {
		child->EnableInHierarchy();
	}
	for (Component* component : components) {
		if (component->IsActive()) {
			component->OnEnable();
		}
	}
}

void GameObject::DisableInHierarchy() {
	if (parent != nullptr && !parent->IsActive()) {
		return;
	}

	for (Component* component : components) {
		if (component->IsActive()) {
			component->OnDisable();
		}
	}
	for (GameObject* child : children) {
		child->DisableInHierarchy();
	}
	activeInHierarchy = false;
}
