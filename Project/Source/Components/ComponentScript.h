#pragma once

#include "Component.h"
#include "Scripting/Member.h"
#include "Scripting/Script.h"
#include "Utils/UID.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

class ComponentScript : public Component {
public:
	REGISTER_COMPONENT(ComponentScript, ComponentType::SCRIPT, true);
	~ComponentScript();

	void Init() override;
	void Start() override;

	void OnEditorUpdate() override;
	void Save(JsonValue jComponent) const override;
	void Load(JsonValue jComponent) override;

	void CreateScriptInstance();
	void ReleaseScriptInstance();
	TESSERACT_ENGINE_API Script* GetScriptInstance() const;
	TESSERACT_ENGINE_API const char* GetScriptName() const;

private:
	std::unordered_map<std::string, std::pair<MemberType, MEMBER_VARIANT>> changedValues;

	UID scriptId = 0;
	std::unique_ptr<Script> scriptInstance = nullptr;
};