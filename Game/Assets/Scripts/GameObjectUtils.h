#include "GameObject.h"

GameObject* SearchReferenceInHierarchy(GameObject* root, std::string name) {
	if (root == nullptr) return nullptr;

	if (root->name == name)	return root;

	GameObject* reference = nullptr;
	for (GameObject* child : root->GetChildren()) {
		reference = SearchReferenceInHierarchy(child, name);
		if (reference != nullptr) return reference;
	}

	return nullptr;
}