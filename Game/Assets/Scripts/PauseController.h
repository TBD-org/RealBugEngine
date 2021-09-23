#pragma once

#include "Scripting/Script.h"
#include "Components/ComponentAudioSource.h"

class PauseController : public Script
{
	GENERATE_BODY(PauseController);

	enum class AudioPause {
		OPEN,
		CLOSE,
		MUSIC,
		TOTAL
	};

public:

	void Start() override;
	void Update() override;

	static void SetIsPause(bool _isPause);

private:
	inline static bool isPause = false;
	bool hasEntered = false;

	//Audio
	ComponentAudioSource* audios[static_cast<int>(AudioPause::TOTAL)] = { nullptr };
	ComponentAudioSource* levelMusic = nullptr;

};

