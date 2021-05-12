#pragma once

#include "Modules/ModuleFiles.h"

#include <string>
#include <vector>

namespace FileDialog {
	void CreateContext();
	void DestroyContext();

	void Init(const std::string& title, bool editMode = false, AllowedExtensionsFlag ext = AllowedExtensionsFlag::ALL, const char* specificPath = nullptr);
	bool OpenDialog(const std::string& title, std::string& selectedPath);
	bool OverrideAlertDialog(const std::string& file);
	bool IsDirectory(const char* path);
	bool Exists(const char* filePath);
	void Copy(const char* path, const char* newPath);

	std::vector<std::string> GetDrives();
	std::vector<std::string> GetFilesInFolder(const char* folderPath, AllowedExtensionsFlag extFilter = AllowedExtensionsFlag::ALL);
	std::vector<std::string> GetFileExtensions(AllowedExtensionsFlag ext);

	std::string GetFileNameAndExtension(const char* filePath);
	std::string GetFileName(const char* filePath);
	std::string GetFileExtension(const char* filePath);
	std::string GetFileFolder(const char* filePath);
	std::string GetAbsolutePath(const char* filePath);

}; // namespace FileDialog
