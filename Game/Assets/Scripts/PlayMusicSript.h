#pragma once

#include "Scripting/Script.h"

class PlayMusicSript : public Script
{
	GENERATE_BODY(PlayMusicSript);

public:

	void Start() override;
	void Update() override;
public:
	UID skullUID;
private:
	GameObject* skull = nullptr;
};
