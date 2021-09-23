#include "Teleporter.h"

#include "GameplaySystems.h"
#include "Math/float2.h"
#include "GameObject.h"
#include "Components/ComponentTransform.h"

EXPOSE_MEMBERS(Teleporter) {
	MEMBER(MemberType::GAME_OBJECT_UID, targetObjectUID),
		MEMBER(MemberType::FLOAT3, targetPosition)
};

GENERATE_BODY_IMPL(Teleporter);

void Teleporter::Start() {

}

void Teleporter::Update() {
	if (Input::GetKeyCode(Input::KEYCODE::KEY_T)) {
		GameObject* object = GameplaySystems::GetGameObject(targetObjectUID);
		if (object) {
			object->GetComponent<ComponentTransform>()->SetPosition(targetPosition);
		}
	}
}