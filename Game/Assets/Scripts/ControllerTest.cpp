#include "ControllerTest.h"

#include "GameplaySystems.h"
#include "Components/ComponentTransform.h"

EXPOSE_MEMBERS(ControllerTest) {
    // Add members here to expose them to the engine. Example:
    // MEMBER(MemberType::BOOL, exampleMember1),
    // MEMBER(MemberType::PREFAB_RESOURCE_UID, exampleMember2),
    MEMBER(MemberType::GAME_OBJECT_UID, kinematicUID)
};

GENERATE_BODY_IMPL(ControllerTest);

void ControllerTest::Start() {
    kinematic = GameplaySystems::GetGameObject(kinematicUID);
	if (kinematic) transform = kinematic->GetComponent<ComponentTransform>();
}

void ControllerTest::Update() {
	if (Input::GetKeyCode(Input::KEYCODE::KEY_I)) {
		transform->SetGlobalPosition(transform->GetGlobalPosition() + float3(0.f, 0.f, 1.f) * 4.f * Time::GetDeltaTime());
		Debug::Log("GG!");
	}

	if (Input::GetKeyCode(Input::KEYCODE::KEY_K)) {
		transform->SetGlobalPosition(transform->GetGlobalPosition() + float3(0.f, 0.f, -1.f) * 4.f * Time::GetDeltaTime());
	}

	if (Input::GetKeyCode(Input::KEYCODE::KEY_J)) {
		transform->SetGlobalPosition(transform->GetGlobalPosition() + float3(1.f, 0.f, 0.f) * 4.f * Time::GetDeltaTime());
	}

	if (Input::GetKeyCode(Input::KEYCODE::KEY_L)) {
		transform->SetGlobalPosition(transform->GetGlobalPosition() + float3(-1.f, 0.f, 0.f) * 4.f * Time::GetDeltaTime());
	}
}