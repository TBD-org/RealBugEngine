#pragma once

#include "Scripting/Script.h"

class GameObject;
class ComponentTransform2D;
class ComponentImage;

class SceneTransition : public Script {
	GENERATE_BODY(SceneTransition);

public:
	enum class TransitionMove {
		LEFT_TO_RIGHT,
		RIGHT_TO_LEFT,
		TOP_TO_BOTTOM,
		BOTTOM_TO_TOP,
		FADE_IN,
		FADE_OUT,
	};

	void Start() override;
	void Update() override;

	void StartTransition(bool isExit_ = false); // To iniciate the transition
	void UpdateObjectToResolution(); // To update the size and the position of the image which made the transition

public:

	UID sceneUID = 0;
	UID transitionUID = 0;

	int transitionMove = static_cast<int>(TransitionMove::LEFT_TO_RIGHT);
	float speedTransition = 700.0f;
	int levelNum = 0;

private:
	GameObject* transitionGO = nullptr;
	ComponentTransform2D* transform2D = nullptr;
	ComponentImage* image2D = nullptr;

	float2 actualResolution = float2(0.f, 0.f);

	bool startTransition = false;
	bool finishedTransition = false;
	bool isExit = false;
};

