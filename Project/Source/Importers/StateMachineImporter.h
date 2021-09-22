#pragma once
#include "Utils/JsonValue.h"

namespace StateMachineImporter {
	bool ImportStateMachine(const char* filePath, JsonValue jMeta);
} // namespace StateMachineImporter
