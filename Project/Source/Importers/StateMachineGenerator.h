#pragma once
#include "Utils/UID.h"

#include <unordered_map>

namespace StateMachineGenerator {
	bool GenerateStateMachine(const char* filePath);
	void SaveToFile(const char* filePath, std::unordered_map<UID, std::string>& listClips);
}; // namespace StateMachinGenerator
	