#include "PanelImportOptions.h"

#include "Resources/Resource.h"
#include "Application.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleUserInterface.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleResources.h"
#include "Importers/TextureImporter.h"
#include "Utils/FileDialog.h"
#include "Utils/Logging.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"
#include "IconsForkAwesome.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/prettywriter.h"

#include "Utils/Leaks.h"

static bool UpdateMetaFile(const char* metaFilePath, ImportOptions& importOptions) {
	rapidjson::Document document;

	if (App->files->Exists(metaFilePath)) {
		// Read meta file
		Buffer<char> buffer = App->files->Load(metaFilePath);
		if (buffer.Size() == 0) {
			LOG("Error reading meta file %s", metaFilePath);
			return false;
		}

		// Parse document from file
		document.Parse<rapidjson::kParseNanAndInfFlag>(buffer.Data());
		if (document.HasParseError()) {
			LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document.GetParseError()), document.GetErrorOffset());
			return false;
		}
	}

	JsonValue jMeta(document, document);
	importOptions.Save(jMeta);

	// Write document to buffer
	rapidjson::StringBuffer stringBuffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator, rapidjson::kWriteNanAndInfFlag> writer(stringBuffer);
	document.Accept(writer);

	// Save to file
	bool saved = App->files->Save(metaFilePath, stringBuffer.GetString(), stringBuffer.GetSize());
	if (!saved) {
		LOG("Error saving meta file %s", metaFilePath);
		return false;
	}

	return true;
}

PanelImportOptions::PanelImportOptions()
	: Panel("Import Options", true) {}

void PanelImportOptions::Update() {
	ImGui::SetNextWindowDockID(App->editor->dockRightId, ImGuiCond_FirstUseEver);
	std::string windowName = std::string(ICON_FA_BOX " ") + name;
	if (ImGui::Begin(windowName.c_str(), &enabled)) {
		const std::string& selected = App->editor->selectedAsset;
		if (selected != "") {
			ImGui::TextUnformatted("Path:");
			ImGui::SameLine();
			ImGui::TextColored(App->editor->textColor, "%s", selected.c_str());

			ImGui::Separator();

			ImportOptions* importOptions = App->resources->GetImportOptions<ImportOptions>(selected.c_str());

			if (importOptions) {
				importOptions->ShowImportOptions();
			}

			ImGui::Separator();

			if (ImGui::Button("Save")) {
				std::string metaFilePath = selected + META_EXTENSION;
				if (importOptions) {
					UpdateMetaFile(metaFilePath.c_str(), *importOptions);
				}
				App->resources->assetsToReimport.push(selected);
			}
		}
	}
	ImGui::End();
}