#include "AdvertScript.h"
#include "GameplaySystems.h"


EXPOSE_MEMBERS(AdvertScript) {
    MEMBER(MemberType::FLOAT, speed),
        MEMBER(MemberType::FLOAT, startTime),
        MEMBER(MemberType::FLOAT, changeTime),
};

GENERATE_BODY_IMPL(AdvertScript);

void AdvertScript::Start() {
    restTime = changeTime;
}

void AdvertScript::Update() {
    //Go Down
    if (startTime > 0) {
        startTime -= Time::GetDeltaTime();
    }
    else {
        if (isChanging) {
            if (firstAdvert) {
                if (offset.y < 0.5) {
                    offset += float2(0, speed) * Time::GetDeltaTime();
                    GetOwner().GetComponent<ComponentMeshRenderer>()->SetTextureOffset(offset);
                }
                else {
                    offset.y = 0.5;
                    isChanging = false;
                }
            }
            else {
                if ((offset.y > 0)) {
                    offset -= float2(0, speed) * Time::GetDeltaTime();
                    GetOwner().GetComponent<ComponentMeshRenderer>()->SetTextureOffset(offset);
                }
                else {
                    offset.y = 0.0;
                    isChanging = false;
                }
            }
        }
        else {
            if (restTime > 0) {
                restTime -= Time::GetDeltaTime();
            }
            else {
                isChanging = true;
                restTime = changeTime;
                firstAdvert = !firstAdvert;
            }
        }
    }
}