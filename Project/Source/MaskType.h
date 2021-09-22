#pragma once

enum class MaskType {
	NONE = 0,
	ENEMY = 1 << 1,
	PLAYER = 1 << 2
};

struct Mask {
	int bitMask = static_cast<int>(MaskType::NONE);

	// Only to Show in PanelInspector
	const char* maskNames[2] = {"ENEMY", "PLAYER"};
	bool maskValues[2] = {false, false};
};

const char* GetMaskTypeName(MaskType type);
MaskType GetMaskTypeFromName(const char* name);
