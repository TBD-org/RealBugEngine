#include "FullScreenSetter.h"
#include "GameplaySystems.h"
#include "GameObject.h"
#include "Components/UI/ComponentToggle.h"

EXPOSE_MEMBERS(FullScreenSetter) {

};

GENERATE_BODY_IMPL(FullScreenSetter);

void FullScreenSetter::Start() {
	ComponentToggle* toggle = GetOwner().GetComponent< ComponentToggle>();
	if (toggle) {
		if (toggle->IsActive()) {
			toggle->SetChecked(Screen::GetWindowMode() == WindowMode::FULLSCREEN);
		}
	}
}

void FullScreenSetter::Update() {

}

void FullScreenSetter::OnToggled(bool toggled_) {
	Screen::SetWindowMode(toggled_ ? WindowMode::FULLSCREEN : WindowMode::WINDOWED);
}
