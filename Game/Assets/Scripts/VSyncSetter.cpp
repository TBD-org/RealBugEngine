#include "VSyncSetter.h"

#include "GameplaySystems.h"
#include "Components/UI/ComponentToggle.h"

EXPOSE_MEMBERS(VSyncSetter) {
};

GENERATE_BODY_IMPL(VSyncSetter);

void VSyncSetter::Start() {
	ComponentToggle* toggle = GetOwner().GetComponent<ComponentToggle>();
	if (toggle) {
		toggle->SetChecked(Screen::IsVSyncActive());
	}
}

void VSyncSetter::Update() {
	
}

void VSyncSetter::OnToggled(bool toggled) {
	Screen::SetVSync(toggled);
}
