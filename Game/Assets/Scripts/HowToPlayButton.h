#pragma once

#include "Scripting/Script.h"

class HowToPlayButton : public Script
{
	GENERATE_BODY(HowToPlayButton);

public:

	void Start() override;
	void Update() override;
	void OnButtonClick() override;

	UID sceneUID = 0;
};
