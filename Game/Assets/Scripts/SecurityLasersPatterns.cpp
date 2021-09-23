#include "SecurityLasersPatterns.h"

#include "GameplaySystems.h"
#include "LasersGenerator.h"

#include <algorithm>
#include <random>

std::random_device rd;
std::mt19937 g(rd());

EXPOSE_MEMBERS(SecurityLasersPatterns) {
    MEMBER(MemberType::FLOAT, laserActiveDuration),
    MEMBER(MemberType::FLOAT, intervalDuration),
    MEMBER(MemberType::GAME_OBJECT_UID, generator1UID),
    MEMBER(MemberType::GAME_OBJECT_UID, generator2UID),
    MEMBER(MemberType::GAME_OBJECT_UID, generator3UID),
    MEMBER(MemberType::GAME_OBJECT_UID, generator4UID)
};

GENERATE_BODY_IMPL(SecurityLasersPatterns);

void SecurityLasersPatterns::Start() {
    GameObject* generator1 = GameplaySystems::GetGameObject(generator1UID);
    GameObject* generator2 = GameplaySystems::GetGameObject(generator2UID);
    GameObject* generator3 = GameplaySystems::GetGameObject(generator3UID);
    GameObject* generator4 = GameplaySystems::GetGameObject(generator4UID);

    float chargingTime = 0.f;
    if (generator1) {
        LasersGenerator* script = GET_SCRIPT(generator1, LasersGenerator);
        if (script) {
            generators.push_back(script);
            chargingTime = script->chargingDuration;
        }
    }
    if (generator2) {
        LasersGenerator* script = GET_SCRIPT(generator2, LasersGenerator);
        if (script) generators.push_back(script);
    }
    if (generator3) {
        LasersGenerator* script = GET_SCRIPT(generator3, LasersGenerator);
        if (script) generators.push_back(script);
    }
    if (generator4) {
        LasersGenerator* script = GET_SCRIPT(generator4, LasersGenerator);
        if (script) generators.push_back(script);
    }

    permutationDuration = intervalDuration * (generators.size()-1) + chargingTime + laserActiveDuration;
    permutationTimer = permutationDuration;
}

void SecurityLasersPatterns::Update() {
    permutationTimer += Time::GetDeltaTime();
    if (permutationTimer > permutationDuration) {
        // Create a random permutation
        permutationTimer = 0;
        std::shuffle(generators.begin(), generators.end(), g);

        // Set the times for this round
        for (int i = 0; i < generators.size(); ++i) {
            generators[i]->coolDownOnTimer = 0.f;
            generators[i]->coolDownOffTimer = permutationDuration - intervalDuration*i;
            generators[i]->coolDownOn = laserActiveDuration;
            generators[i]->coolDownOff = permutationDuration;
            generators[i]->Init();
        }
    }
}