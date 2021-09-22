#include "PanelAudioMixer.h"

#include "Components/ComponentAudioListener.h"
#include "Application.h"
#include "Modules/ModuleEditor.h"
#include "Modules/ModuleScene.h"
#include "Utils/Pool.h"
#include "Scene.h"

#include "imgui.h"
#include "IconsForkAwesome.h"
#include <string>

#include "Utils/Leaks.h"

PanelAudioMixer::PanelAudioMixer()
	: Panel("Audio Mixer", false) {}

void PanelAudioMixer::Update() {
	ImGui::SetNextWindowSize(ImVec2(400.0f, 400.0f), ImGuiCond_FirstUseEver);
	std::string windowName = std::string(ICON_FK_MUSIC " ") + name;
	if (ImGui::Begin(windowName.c_str(), &enabled, ImGuiWindowFlags_AlwaysAutoResize)) {
		ComponentAudioListener* listener = nullptr;
		if (App->scene->GetCurrentScene()->audioListenerComponents.Count() == 0) {
			ImGui::End();
			return;
		}
		Pool<ComponentAudioListener>::Iterator audioListener = App->scene->GetCurrentScene()->audioListenerComponents.begin();
		float gain = (*audioListener).GetAudioVolume();

		ImGui::TextColored(App->editor->titleColor, "Output Volume");
		if (ImGui::SliderFloat("##out_volume", &gain, 0.f, 1.f)) {
			(*audioListener).SetAudioVolume(gain);
		}
	}
	ImGui::End();
}
