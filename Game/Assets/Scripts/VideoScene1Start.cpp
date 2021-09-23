#include "VideoScene1Start.h"

#include "GameObject.h"
#include "GameplaySystems.h"
#include "PlayerController.h"
#include "GameController.h"
#include "CanvasFader.h"
#include "Components/UI/ComponentVideo.h"
#include "Components/UI/ComponentCanvas.h"

EXPOSE_MEMBERS(VideoScene1Start) {
    MEMBER(MemberType::GAME_OBJECT_UID, canvasFaderUID)
};

GENERATE_BODY_IMPL(VideoScene1Start);

void VideoScene1Start::Start() {
    componentVideo = GetOwner().GetComponent<ComponentVideo>();
    parent = GetOwner().GetParent();

    componentVideo->SetVideoFrameSize(Screen::GetResolution().x, Screen::GetResolution().y);
    if (componentVideo) {
        componentVideo->Play();
        GameController::SetVideoActive(true);
    }
    Time::PauseGame();
}

void VideoScene1Start::Update() {

    if ((componentVideo->HasVideoFinished() && componentVideo->IsActive()) || Input::GetKeyCodeDown(Input::KEYCODE::KEY_ESCAPE)) {
        Time::ResumeGame();
        componentVideo->Stop();
        GameController::SetVideoActive(false);
        parent->Disable();

        if (canvasFaderUID > 0) {
            GameObject* canvasFader = GameplaySystems::GetGameObject(canvasFaderUID);
            if (canvasFader) {
                CanvasFader* faderScript = GET_SCRIPT(canvasFader, CanvasFader);
                if (faderScript) {
                    faderScript->FadeIn();
                }
            }
        }
    }
}