#include "MaskType.h"

#include "Utils/Logging.h"

#include <cassert>

const char* GetMaskTypeName(MaskType type) {
	switch (type) {
	case MaskType::NONE:
		return "NONE";
	case MaskType::ENEMY:
		return "ENEMY";
	case MaskType::PLAYER:
		return "PLAYER";
	default:
		LOG("The mask selected is not registered.");
		assert(false);
		return nullptr;
	}

	return nullptr;
}

MaskType GetMaskTypeFromName(const char* name) {
	if (strcmp(name, "NONE") == 0) {
		return MaskType::NONE;
	} else if (strcmp(name, "ENEMY") == 0) {
		return MaskType::ENEMY;
	} else if (strcmp(name, "PLAYER") == 0) {
		return MaskType::PLAYER;
	} else {
		assert(false);
		return MaskType::NONE;
	}
}
