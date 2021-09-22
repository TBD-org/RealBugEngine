#pragma once

#include "Utils/JsonValue.h"

/* Creating import options for a new asset type:
*    1. Extend the ImportOptions class
*    2. Add a new case to the LoadImportOptions function in ModuleResources.cpp
*    3. If the import options are also needed when loading the resources:
*        - Add the options as resource members and override LoadResourceMeta and SaveResourceMeta
*        - In the importer, before creating the resource, get the import options using GetImportOptions from ModuleResources.h
*        - In the importer, just after the resource has been created, initialize the members to the import options values
*/

class ImportOptions {
public:
	virtual ~ImportOptions();

	virtual void ShowImportOptions();
	virtual void Load(JsonValue jMeta);
	virtual void Save(JsonValue jMeta);
};
