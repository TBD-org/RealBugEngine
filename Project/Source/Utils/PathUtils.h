#pragma once

#include <string>

namespace PathUtils {
	std::string GetFileNameAndExtension(const char* filePath);
	std::string GetFileName(const char* filePath);
	std::string GetFileExtension(const char* filePath);
	std::string GetFileFolder(const char* filePath);
};
