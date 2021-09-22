#include "PathUtils.h"

#include <algorithm>
#include "Math/MathFunc.h"

std::string PathUtils::GetFileNameAndExtension(const char* filePath) {
	const char* lastSlash = strrchr(filePath, '/');
	const char* lastBackslash = strrchr(filePath, '\\');
	const char* lastSeparator = Max(lastSlash, lastBackslash);

	if (lastSeparator == nullptr) {
		return filePath;
	}

	const char* fileNameAndExtension = lastSeparator + 1;
	return fileNameAndExtension;
}

std::string PathUtils::GetFileName(const char* filePath) {
	const char* lastSlash = strrchr(filePath, '/');
	const char* lastBackslash = strrchr(filePath, '\\');
	const char* lastSeparator = Max(lastSlash, lastBackslash);

	const char* fileName = lastSeparator == nullptr ? filePath : lastSeparator + 1;
	const char* lastDot = strrchr(fileName, '.');

	if (lastDot == nullptr || lastDot == fileName) {
		return fileName;
	}

	return std::string(fileName).substr(0, lastDot - fileName);
}

std::string PathUtils::GetFileExtension(const char* filePath) {
	const char* lastSlash = strrchr(filePath, '/');
	const char* lastBackslash = strrchr(filePath, '\\');
	const char* lastSeparator = Max(lastSlash, lastBackslash);
	const char* lastDot = strrchr(filePath, '.');

	if (lastDot == nullptr || lastDot == filePath || lastDot < lastSeparator || lastDot == lastSeparator + 1) {
		return std::string();
	}

	std::string extension = std::string(lastDot);
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	return extension;
}

std::string PathUtils::GetFileFolder(const char* filePath) {
	const char* lastSlash = strrchr(filePath, '/');
	const char* lastBackslash = strrchr(filePath, '\\');
	const char* lastSeparator = Max(lastSlash, lastBackslash);

	if (lastSeparator == nullptr) {
		return std::string();
	}

	return std::string(filePath).substr(0, lastSeparator - filePath);
}