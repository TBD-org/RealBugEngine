#pragma once

#pragma warning(disable : 4251)

#include "Utils/UID.h"
#include "Utils/JsonValue.h"
#include "MaskType.h"
#include "Scene.h"

#include <string>
#include <vector>

class Component;

template<typename T>
class ComponentView {
public:
	class Iterator {
	public:
		Iterator(const ComponentView<T>& view__, std::vector<Component*>::const_iterator it__)
			: view(view__)
			, it(it__) {}

		const Iterator& operator++() {
			++it;
			while (*this != view.end() && (*it)->GetType() != T::staticType) {
				++it;
			}
			return *this;
		}

		bool operator!=(const Iterator& other) const {
			return it != other.it;
		}

		T& operator*() const {
			return (T&) **it;
		}

	private:
		const ComponentView<T>& view;
		std::vector<Component*>::const_iterator it;
	};

public:
	ComponentView(const std::vector<Component*>& components__)
		: components(components__) {}

	typename Iterator begin() const {
		std::vector<Component*>::const_iterator it = components.begin();
		while (it != components.end() && (*it)->GetType() != T::staticType) {
			++it;
		}
		return Iterator(*this, it);
	}

	typename Iterator end() const {
		return Iterator(*this, components.end());
	}

private:
	const std::vector<Component*>& components;
};

class TESSERACT_ENGINE_API GameObject {
public:
	void Init();
	void Start();
	void Update();
	void DrawGizmos();

	void Enable();
	void Disable();
	bool IsActive() const;
	bool IsActiveInHierarchy() const;
	bool IsActiveInternal() const;
	void SetStatic(bool value);
	bool IsStatic() const;

	UID GetID() const;

	template<class T> T* CreateComponent();
	template<class T> bool HasComponent() const;
	template<class T> T* GetComponent() const;
	template<class T> ComponentView<T> GetComponents() const;
	template<class T> GameObject* HasComponentInAnyParent(GameObject* current) const; // Finds in the current object or in any parent of this Object the T Component. Returns the GameObject if found, else nullptr
	const std::vector<Component*>& GetComponents() const;
	void RemoveComponent(Component* component);
	void RemoveAllComponents();

	void SetParent(GameObject* gameObject);
	GameObject* GetParent() const;

	void SetRootBone(GameObject* gameObject);
	GameObject* GetRootBone() const;

	void AddMask(MaskType mask_);
	void DeleteMask(MaskType mask_);
	Mask& GetMask();

	void AddChild(GameObject* gameObject);
	void RemoveChild(GameObject* gameObject);
	const std::vector<GameObject*>& GetChildren() const;
	GameObject* GetChild(unsigned index) const;
	GameObject* GetChild(const char* name) const;
	bool IsDescendantOf(GameObject* gameObject);
	GameObject* FindDescendant(const std::string& name) const;
	bool HasChildren() const;

	void Save(JsonValue jGameObject) const;
	void Load(JsonValue jGameObject);

	void SavePrefab(JsonValue jGameObject);
	void LoadPrefab(JsonValue jGameObject);

public:
	UID id = 0;
	std::string name = "";

	Scene* scene = nullptr;
	bool isInQuadtree = false;

	bool flag = false; // Auxiliary variable to help with iterating on the Quadtree

	std::vector<Component*> components;

private:
	void EnableInHierarchy();
	void DisableInHierarchy();

private:
	bool active = true;
	bool isStatic = false; // Used for NavMesh creation atm
	bool activeInHierarchy = true;
	Mask mask;
	UID prefabId = 0;
	GameObject* parent = nullptr;
	GameObject* rootBoneHierarchy = nullptr;
	std::vector<GameObject*> children;
};

template<class T>
inline T* GameObject::CreateComponent() {
	if (!T::allowMultipleComponents && HasComponent<T>()) return nullptr;
	T* component = (T*) scene->CreateComponentByTypeAndId(this, T::staticType, GenerateUID());
	components.push_back(component);
	return component;
}

template<class T>
inline bool GameObject::HasComponent() const {
	for (Component* component : components) {
		if (component->GetType() == T::staticType) {
			return true;
		}
	}

	return false;
}

template<class T>
inline T* GameObject::GetComponent() const {
	for (Component* component : components) {
		if (component->GetType() == T::staticType) {
			return (T*) component;
		}
	}
	return nullptr;
}

template<class T>
inline ComponentView<T> GameObject::GetComponents() const {
	return ComponentView<T>(components);
}

template<class T>
inline GameObject* GameObject::HasComponentInAnyParent(GameObject* current) const {
	T* component = current->GetComponent<T>();
	if (component != nullptr) {
		return current;
	} else {
		if (current->GetParent() != nullptr) {
			return HasComponentInAnyParent<T>(current->GetParent());
		}
	}

	return nullptr;
}
