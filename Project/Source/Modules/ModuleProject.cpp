#include "ModuleProject.h"

#include "fmt/format.h"

#include "Application.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleEvents.h"
#include "Modules/ModuleTime.h"
#include "Components/ComponentScript.h"
#include "Scripting/PropertyMap.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
#include "Utils/FileDialog.h"
#include "Utils/PathUtils.h"
#include "Scene.h"

#include <Windows.h>
#include <shellapi.h>
#include <ObjIdl.h>

// from https://msdn.microsoft.com/en-us/library/yf86a8ts.aspx
#pragma warning(disable : 4278)
#pragma warning(disable : 4146)
#include "Utils/dte80a.tlh"
#pragma warning(default : 4146)
#pragma warning(default : 4278)

#include "Utils/Leaks.h"

EnvDTE::Process* FindVSProcess(DWORD targetPID) {
	HRESULT hr;

	static const wchar_t* progID = L"VisualStudio.DTE.16.0";

	CLSID clsID;
	CLSIDFromProgID(progID, &clsID);

	IUnknown* unknown;
	hr = GetActiveObject(clsID, 0, &unknown);
	if (FAILED(hr)) {
		return nullptr;
	}

	EnvDTE::_DTE* interface_;

	hr = unknown->QueryInterface(&interface_);
	if (FAILED(hr)) {
		return nullptr;
	}

	EnvDTE::Debugger* debugger;
	hr = interface_->get_Debugger(&debugger);
	if (FAILED(hr)) {
		return nullptr;
	}

	EnvDTE::Processes* processes;
	hr = debugger->get_LocalProcesses(&processes);
	if (FAILED(hr)) {
		return nullptr;
	}

	long Count = 0;
	hr = processes->get_Count(&Count);
	if (FAILED(hr)) {
		return nullptr;
	}

	EnvDTE::Process* result = nullptr;

	for (int i = 0; i < Count; ++i) {
		EnvDTE::Process* process;

		hr = processes->Item(_variant_t(i), &process);
		if (FAILED(hr)) continue;

		long processID;
		hr = process->get_ProcessID(&processID);
		if (FAILED(hr)) {
			return nullptr;
		}

		if (processID == targetPID) {
			result = process;
		}
	}

	return result;
}

void AttachVS() {
	DWORD targetPID = GetCurrentProcessId();
	EnvDTE::Process* process = FindVSProcess(targetPID);
	if (process) {
		process->Attach();
	}
}

void DetachVS(bool waitForBreakOrEnd) {
	DWORD targetPID = GetCurrentProcessId();
	EnvDTE::Process* process = FindVSProcess(targetPID);
	if (process) {
		process->Detach(variant_t(false));
	}
}

template<class T>
static T struct_cast(void* ptr, LONG offset = 0) {
	return reinterpret_cast<T>(reinterpret_cast<intptr_t>(ptr) + offset);
}

struct RSDSHeader {
	DWORD signature;
	GUID guid;
	long version;
	char filename[1];
};

static std::string GetLastErrorStdStr() {
	DWORD error = GetLastError();
	if (error) {
		LPVOID lpMsgBuf;
		DWORD bufLen = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		if (bufLen) {
			LPCSTR lpMsgStr = (LPCSTR) lpMsgBuf;
			std::string result(lpMsgStr, lpMsgStr + bufLen);

			std::string::size_type pos = result.find("\r\n");
			if (pos != std::string::npos) {
				result.replace(result.begin() + pos, result.end(), "\n\0");
			}
			LocalFree(lpMsgBuf);

			return result;
		}
	}
	return std::string();
}

static bool DebugDirRelativeVirtualAddress(PIMAGE_OPTIONAL_HEADER optionalHeader, DWORD& debugDirRva, DWORD& debugDirSize) {
	if (optionalHeader->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
		// Represents the optional header format.
		PIMAGE_OPTIONAL_HEADER64 optionalHeader64 = struct_cast<PIMAGE_OPTIONAL_HEADER64>(optionalHeader);
		debugDirRva = optionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
		debugDirSize = optionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
	} else {
		// Represents the optional header format.
		PIMAGE_OPTIONAL_HEADER32 optionalHeader32 = struct_cast<PIMAGE_OPTIONAL_HEADER32>(optionalHeader);
		debugDirRva = optionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
		debugDirSize = optionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
	}

	if (debugDirRva == 0 && debugDirSize == 0) {
		return true;
	} else if (debugDirRva == 0 || debugDirSize == 0) {
		return false;
	}

	return true;
}

static bool FileOffsetRelativeVirtualAdress(PIMAGE_NT_HEADERS ntHeaders, DWORD rva, DWORD& fileOffset) {
	bool found = false;

	// Represents the image section header format.
	IMAGE_SECTION_HEADER* sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
	for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections && !found; ++i, ++sectionHeader) {
		if (sectionHeader->VirtualAddress <= rva) {
			if (sectionHeader->VirtualAddress + sectionHeader->Misc.VirtualSize > rva) {
				found = true;
				fileOffset = rva - sectionHeader->VirtualAddress + sectionHeader->PointerToRawData;
			}
		}
	}

	if (!found) {
		return false;
	}

	return true;
}

static char* PDBFind(LPBYTE imageBase, PIMAGE_DEBUG_DIRECTORY debugDir) {
	assert(debugDir && imageBase);
	LPBYTE debugInfo = const_cast<LPBYTE>(imageBase + debugDir->PointerToRawData);
	const DWORD debugInfoSize = debugDir->SizeOfData;

	if (debugInfo == 0 || debugInfoSize == 0) {
		return nullptr;
	}

	if (IsBadReadPtr(debugInfo, debugInfoSize)) {
		return nullptr;
	}

	if (debugInfoSize < sizeof(DWORD)) {
		return nullptr;
	}

	if (debugDir->Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
		auto signature = *(DWORD*) debugInfo;
		if (signature == 'SDSR') {
			auto* info = (RSDSHeader*) (debugInfo);
			if (IsBadReadPtr(debugInfo, sizeof(RSDSHeader))) {
				return nullptr;
			}
			if (IsBadStringPtrA((const char*) info->filename, UINT_MAX)) {
				return nullptr;
			}
			return info->filename;
		}
	}

	return nullptr;
}

static bool PDBReplace(const std::string& filename, const std::string& namePDB) {
	HANDLE fp = nullptr;
	HANDLE filemap = nullptr;
	LPVOID mem = nullptr;
	bool result = false;

	DEFER {
		if (mem != nullptr) {
			UnmapViewOfFile(mem);
		}

		if (filemap != nullptr) {
			CloseHandle(filemap);
		}

		if ((fp != nullptr) && (fp != INVALID_HANDLE_VALUE)) {
			CloseHandle(fp);
		}
	};

	fp = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if ((fp == INVALID_HANDLE_VALUE) || (fp == nullptr)) {
		return false;
	}

	// Creates or opens a named or unnamed file mapping object for a specified file.
	filemap = CreateFileMapping(fp, nullptr, PAGE_READWRITE, 0, 0, nullptr);
	if (filemap == nullptr) {
		return false;
	}

	// Maps a view of a file mapping into the address space of a calling process.
	mem = MapViewOfFile(filemap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (mem == nullptr) {
		return false;
	}

	//  Represents the DOS header format.
	PIMAGE_DOS_HEADER dosHeader = struct_cast<PIMAGE_DOS_HEADER>(mem);
	if (dosHeader == 0) {
		return false;
	}

	// Verifies that the calling process has read access to the specified range of memory.
	if (IsBadReadPtr(dosHeader, sizeof(IMAGE_DOS_HEADER))) {
		return false;
	}

	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		return false;
	}

	// Represents the PE header format.
	PIMAGE_NT_HEADERS ntHeaders = struct_cast<PIMAGE_NT_HEADERS>(dosHeader, dosHeader->e_lfanew);
	if (ntHeaders == 0) {
		return false;
	}

	// Verifies that the calling process has read access to the specified range of memory.
	if (IsBadReadPtr(ntHeaders, sizeof(ntHeaders->Signature))) {
		return false;
	}

	// Check if signature of the NT header is the correct one
	if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
		return false;
	}

	// Verifies that the calling process has read access to the specified range of memory.
	if (IsBadReadPtr(&ntHeaders->FileHeader, sizeof(IMAGE_FILE_HEADER))) {
		return false;
	}

	if (IsBadReadPtr(&ntHeaders->OptionalHeader, ntHeaders->FileHeader.SizeOfOptionalHeader)) {
		return false;
	}

	if (ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC && ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
		return false;
	}

	// Retrieve the contents of PE file sections
	auto sectionHeaders = IMAGE_FIRST_SECTION(ntHeaders);
	if (IsBadReadPtr(sectionHeaders, ntHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER))) {
		return false;
	}

	DWORD debugDirVRA = 0;
	DWORD debugDirSize = 0;
	if (!DebugDirRelativeVirtualAddress(&ntHeaders->OptionalHeader, debugDirVRA, debugDirSize)) {
		return false;
	}

	if (debugDirVRA == 0 || debugDirSize == 0) {
		return false;
	}

	DWORD offset = 0;
	if (!FileOffsetRelativeVirtualAdress(ntHeaders, debugDirVRA, offset)) {
		return false;
	}

	// Represents the debug directory format.
	PIMAGE_DEBUG_DIRECTORY debugDir = struct_cast<PIMAGE_DEBUG_DIRECTORY>(mem, offset);
	if (debugDir == 0) {
		return false;
	}

	if (IsBadReadPtr(debugDir, debugDirSize)) {
		return false;
	}

	if (debugDirSize < sizeof(IMAGE_DEBUG_DIRECTORY)) {
		return false;
	}

	int numEntries = debugDirSize / sizeof(IMAGE_DEBUG_DIRECTORY);
	if (numEntries == 0) {
		return false;
	}

	for (int i = 1; i <= numEntries; ++i, ++debugDir) {
		char* pdb = PDBFind((LPBYTE) mem, debugDir);
		if (pdb) {
			size_t len = strlen(pdb);
			if (len >= strlen(namePDB.c_str())) {
				memcpy_s(pdb, len, namePDB.c_str(), namePDB.length());
				pdb[namePDB.length()] = 0;
				result = true;
			}
		}
	}

	return result;
}

namespace Tesseract {
	std::istream& getline(std::istream& is, std::string& t) {
		// Clear to add the new value
		t.clear();

		std::istream::sentry se(is, true);
		std::streambuf* sb = is.rdbuf();

		for (;;) {
			int c = sb->sbumpc();
			switch (c) {
			case '\n':
				return is;
			case '\r':
				if (sb->sgetc() == '\n')
					sb->sbumpc();
				return is;
			case std::streambuf::traits_type::eof():
				// Also handle the case when the last line has no line ending
				if (t.empty())
					is.setstate(std::ios::eofbit);
				return is;
			default:
				t += (char) c;
				break;
			}
		}
	}
} // namespace Tesseract

bool ModuleProject::Init() {
	Factory::CreateContext();

	gameState = new PropertyMap();

#if GAME
	UnloadGameCodeDLL();
	if (!LoadGameCodeDLL("Penteract.dll")) {
		LOG("%s", GetLastErrorStdStr().c_str());
	}
#else
	LoadProject("Penteract/Penteract.sln");
#endif
	return true;
}

UpdateStatus ModuleProject::Update() {
	if (App->time->HasGameStarted()) {
		for (ComponentScript& script : App->scene->GetCurrentScene()->scriptComponents) {
			if (script.IsActive()) {
				Script* scriptInstance = script.GetScriptInstance();
				if (scriptInstance != nullptr) {
					scriptInstance->Update();
				}
			}
		}
	}

	return UpdateStatus::CONTINUE;
}

bool ModuleProject::CleanUp() {
	UnloadGameCodeDLL();
	Factory::DestroyContext();
	RELEASE(gameState);
	return true;
}

void ModuleProject::ReceiveEvent(TesseractEvent& e) {
	if (App->time->HasGameStarted()) {
		for (ComponentScript& script : App->scene->GetCurrentScene()->scriptComponents) {
			if (script.IsActive()) {
				Script* scriptInstance = script.GetScriptInstance();
				if (scriptInstance != nullptr) {
					scriptInstance->ReceiveEvent(e);
				}
			}
		}
	}
}

void ModuleProject::LoadProject(const char* path) {
	projectPath = PathUtils::GetFileFolder(path);
	projectName = PathUtils::GetFileNameAndExtension(path);
	App->files->AddSearchPath(projectPath.c_str());

	if (!App->files->Exists(projectName.c_str())) {
		CreateNewProject(projectPath.c_str(), "");
	}
}

void ModuleProject::CreateScript(std::string& name) {
	Buffer<char> bufferHeader = App->files->Load("Templates/Header");
	Buffer<char> bufferSource = App->files->Load("Templates/Source");

	std::string header = bufferHeader.Data();
	std::string source = bufferSource.Data();

	std::string assetsPath = "Assets/Scripts/";
	std::string realPath = projectPath + "/Source/";

	std::string result;

	result = fmt::format(header, name, "{", "}");
	App->files->Save((assetsPath + name + ".h").c_str(), result.data(), result.size());

	result = fmt::format(source, name, "{", "}");
	App->files->Save((assetsPath + name + ".cpp").c_str(), result.data(), result.size());
}

void ModuleProject::CreateNewProject(const char* name, const char* path) {
	std::string fullPath = std::string(path) + name;
	std::string batchPath = fullPath + "/Batches";

	App->files->CreateFolder(fullPath.c_str());
	App->files->CreateFolder(batchPath.c_str());

	std::string UIDProject("{");
	UIDProject += GenerateUID128();
	UIDProject += "}";

	CreateMSVCSolution((fullPath + "/" + name + ".sln").c_str(), name, UIDProject.c_str());
	CreateMSVCProject((fullPath + "/" + name + ".vcxproj").c_str(), name, UIDProject.c_str());

	CreateBatches();
}

void ModuleProject::CreateMSVCSolution(const char* path, const char* name, const char* UIDProject) {
	Buffer<char> buffer = App->files->Load("Templates/MSVCSolution");

	std::string solution = buffer.Data();

	std::string UIDSolution("{");
	UIDSolution += GenerateUID128();
	UIDSolution += "}";

	std::string result = fmt::format(solution, name, UIDProject, UIDSolution);

	App->files->Save(path, result.data(), result.size());
}

void ModuleProject::CreateMSVCProject(const char* path, const char* name, const char* UIDProject) {
	Buffer<char> buffer = App->files->Load("Templates/MSVCProject");

	std::string project = buffer.Data();
	std::string enginePath = PathUtils::GetFileFolder(FileDialog::GetAbsolutePath("").c_str());

#ifdef _DEBUG
	std::string result = fmt::format(project, name, UIDProject, "../../Project/Source/", "../../Project/Libs/MathGeoLib", "../../Project/Libs/SDL/include", "../../Project/Libs/rapidjson/include", "../../Project/Libs/OpenAL-soft/include", "../../Project/Libs/Bullet/include", "../../Project/Libs/recastnavigation-1.5.1", "../../Project/Libs/imgui", "../../Project/Libs/libav/include", enginePath);
#else
	std::string result = fmt::format(project, name, UIDProject, "../Engine/Source/", "../Engine/Libs/MathGeoLib", "../Engine/Libs/SDL/include", "../Engine/Libs/rapidjson/include", "../Engine/Libs/OpenAL-soft/include", "../Engine/Libs/Bullet/include", "../Engine/Libs/recastnavigation-1.5.1", "../Engine/Libs/imgui", "../Engine/Libs/libav/include", enginePath);
#endif

	App->files->Save(path, result.data(), result.size());
}

void ModuleProject::CreateBatches() {
	Buffer<char> buffer;
	std::string file;
	std::string result;

	// Debug (.exe)
	buffer = App->files->Load("Templates/BuildDebug");
	file = buffer.Data();
	result = fmt::format(file, projectName);
	App->files->Save((projectPath + "/Batches/BuildDebug.bat").c_str(), result.data(), result.size());

	// Debug Editor (.dll)
	buffer = App->files->Load("Templates/BuildDebugEditor");
	file = buffer.Data();
	result = fmt::format(file, projectName);
	App->files->Save((projectPath + "/Batches/BuildDebugEditor.bat").c_str(), result.data(), result.size());

	// Release (.exe)
	buffer = App->files->Load("Templates/BuildRelease");
	file = buffer.Data();
	result = fmt::format(file, projectName);
	App->files->Save((projectPath + "/Batches/BuildRelease.bat").c_str(), result.data(), result.size());

	// Release Editor (.dll)
	buffer = App->files->Load("Templates/BuildReleaseEditor");
	file = buffer.Data();
	result = fmt::format(file, projectName);
	App->files->Save((projectPath + "/Batches/BuildReleaseEditor.bat").c_str(), result.data(), result.size());
}

void ModuleProject::CompileProject(Configuration config) {
	if (IsDebuggerPresent()) {
		DetachVS(true);
	}

	UnloadGameCodeDLL();

	std::string batchDir = projectPath + "/Batches";

	SHELLEXECUTEINFO sei = {0};
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.hwnd = NULL;
	sei.lpVerb = "open";
	sei.lpParameters = NULL;
	sei.lpDirectory = batchDir.c_str();
	sei.nShow = NULL;
	sei.hInstApp = NULL;

	switch (config) {
	case Configuration::RELEASE:
		sei.lpFile = "BuildRelease.bat";
		break;
	case Configuration::RELEASE_EDITOR:
		sei.lpFile = "BuildReleaseEditor.bat";
		break;
	case Configuration::DEBUG:
		sei.lpFile = "BuildDebug.bat";
		break;
	case Configuration::DEBUG_EDITOR:
		sei.lpFile = "BuildDebugEditor.bat";
		break;
	default:
		sei.lpFile = "";
		break;
	}

	ShellExecuteEx(&sei);
	WaitForSingleObject(sei.hProcess, INFINITE);
	CloseHandle(sei.hProcess);

	std::string buildPath = projectPath;
	std::string name = PathUtils::GetFileName(projectName.c_str());

	switch (config) {
	case Configuration::RELEASE:
		buildPath += "/Build/Release/";
		break;
	case Configuration::RELEASE_EDITOR:
		buildPath += "/Build/ReleaseEditor/";
		break;
	case Configuration::DEBUG:
		buildPath += "/Build/Debug/";
		break;
	case Configuration::DEBUG_EDITOR:
		buildPath += "/Build/DebugEditor/";
		break;
	}

	std::string dllPath = buildPath + name + ".dll";
	std::string pdbPath = buildPath + name + ".pdb";
	std::string dllPathAux = "";

	std::string auxName = App->files->GetFilePath(pdbPath.c_str(), true);

	// GetFilePath returns "" if the file is not found
	if (auxName != "") {
		std::size_t found = auxName.find_first_of("/");
		while (found != std::string::npos) {
			auxName[found] = '\\';
			found = auxName.find_first_of("/", found + 1);
		}
		auxName[auxName.size() - 5] = '_';

		dllPathAux = buildPath + PathUtils::GetFileName(auxName.c_str()) + ".dll";

		if (!CopyFileA(dllPath.c_str(), dllPathAux.c_str(), FALSE)) {
			std::string error = GetLastErrorStdStr().c_str();
			LOG("Move fails dll %s", error.c_str());
		}
		PDBReplace(dllPathAux, auxName);

		std::string realPDB = buildPath + name + ".pdb";

		if (!CopyFileA(realPDB.c_str(), auxName.c_str(), FALSE)) {
			std::string error = GetLastErrorStdStr().c_str();
			LOG("Move fails pdb %s", error.c_str());
		}
	}

	std::string logFile = buildPath + name + ".log";
	Buffer<char> buffer = App->files->Load(logFile.c_str());
	std::string logContent = "";

	if (buffer.Size() > 0) {
		logContent = buffer.Data();
	}

	std::istringstream f(logContent);
	std::string line;
	LOG("---------------------GAME COMPILATION---------------------");
	while (Tesseract::getline(f, line)) {
		LOG("%s", line.c_str());
	}
	LOG("----------------------------------------------------------");

	if (!LoadGameCodeDLL(dllPathAux.c_str())) {
		LOG("DLL NOT LOADED: %s", GetLastErrorStdStr().c_str());
	}

	if (!IsDebuggerPresent()) {
		AttachVS();
	}

	App->events->AddEvent(TesseractEventType::COMPILATION_FINISHED);
}

bool ModuleProject::IsGameLoaded() const {
	return gameCodeDLL != nullptr;
}

PropertyMap* ModuleProject::GetGameState() const {
	return gameState;
}

bool ModuleProject::LoadGameCodeDLL(const char* path) {
	if (gameCodeDLL) {
		LOG("DLL already loaded.");
		return false;
	}

	gameCodeDLL = LoadLibrary(path);

	return gameCodeDLL ? true : false;
}

bool ModuleProject::UnloadGameCodeDLL() {
	Scene* scene = App->scene->GetCurrentScene();
	if (scene != nullptr) {
		for (ComponentScript& scriptComponent : scene->scriptComponents) {
			scriptComponent.ReleaseScriptInstance();
		}
	}

	if (!gameCodeDLL) {
		LOG("No DLL to unload.");
		return false;
	}

	FreeLibrary(gameCodeDLL);

	gameCodeDLL = nullptr;

	return true;
}