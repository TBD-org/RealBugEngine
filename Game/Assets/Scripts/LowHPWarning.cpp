#include "LowHPWarning.h"
#include "GameObject.h"
#include "Components/UI/ComponentImage.h"
#include "Components/ComponentAudioSource.h"

#include "GameplaySystems.h"

EXPOSE_MEMBERS(LowHPWarning) {
    MEMBER(MemberType::FLOAT, effectTime),
    MEMBER(MemberType::FLOAT, fadeOutTime),
    MEMBER(MemberType::GAME_OBJECT_UID, effectUID),
    MEMBER(MemberType::GAME_OBJECT_UID, audioUID)
};

GENERATE_BODY_IMPL(LowHPWarning);

void LowHPWarning::Start() {
    effect = GameplaySystems::GetGameObject(effectUID);
    if (effect) {
        vignette = effect->GetComponent<ComponentImage>();
        if (vignette) {
            color = vignette->GetColor();
        }
        effect->Disable();
    }
    
    GameObject* audioGO = GameplaySystems::GetGameObject(audioUID);
    if (audioGO) audio = audioGO->GetComponent<ComponentAudioSource>();
}

void LowHPWarning::Update() {
    if (!effect || !vignette) return;
    if (playing) {
        bool doFadeOut = effectTime - effectCurrentTime <= fadeOutTime ? true : false;
        if (effectCurrentTime >= effectTime) {
            vignette->SetColor(float4(color.x, color.y, color.z, color.w));
            effectCurrentTime = 0.0f;
            fadeOutCurrentTime = 0.0f;
        }
        else {
            if (doFadeOut) {
                if (fadeOutCurrentTime <= fadeOutTime) {
                    float delta = fadeOutCurrentTime / fadeOutTime;
                    float fadeValue = Lerp(color.w, 0, delta);
                    vignette->SetColor(float4(color.x, color.y, color.z, fadeValue));
                    fadeOutCurrentTime += Time::GetDeltaTime();
                }
            }
        }
        effectCurrentTime += Time::GetDeltaTime(); 
    }
}

void LowHPWarning::Play() {
    if (!effect || !vignette) return;
    playing = true;
    effect->Enable();
    if (audio) audio->Play();
}

void LowHPWarning::Stop() {
    if (!effect || !vignette) return;
    playing = false;
    effectCurrentTime = 0.0f;
    fadeOutCurrentTime = 0.0f;
    effect->Disable();
    if (audio) audio->Stop();
}
