#include "ImporterCommon.h"

#include "Globals.h"
#include "Application.h"
#include "Resources/Resource.h"
#include "Modules/ModuleFiles.h"

#include "rapidjson/prettywriter.h"

#include "Utils/Leaks.h"

#define JSON_TAG_TYPE "Type"
#define JSON_TAG_NAME "Name"

bool ImporterCommon::SaveResourceMetaFile(Resource* resource) {
	// Create resource meta file
	rapidjson::Document resourceMetaDocument;
	JsonValue jResourceMeta(resourceMetaDocument, resourceMetaDocument);

	// Save meta information
	jResourceMeta[JSON_TAG_TYPE] = GetResourceTypeName(resource->GetType());
	jResourceMeta[JSON_TAG_NAME] = resource->GetName().c_str();
	resource->SaveResourceMeta(jResourceMeta);

	// Write resource meta document to buffer
	rapidjson::StringBuffer stringBuffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator, rapidjson::kWriteNanAndInfFlag> writer(stringBuffer);
	resourceMetaDocument.Accept(writer);

	// Save resource meta file
	const std::string& resourceMetaFilePath = resource->GetResourceFilePath() + META_EXTENSION;
	bool saved = App->files->Save(resourceMetaFilePath.c_str(), stringBuffer.GetString(), stringBuffer.GetSize());
	if (!saved) return false;

	return true;
}
