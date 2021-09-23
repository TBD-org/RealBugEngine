#include "DroneWayPoints.h"
#include "GameObject.h"
#include "Components/ComponentTransform.h"
#include <GameplaySystems.h>
EXPOSE_MEMBERS(DroneWayPoints) {
    MEMBER(MemberType::STRING, initialSubString),
    MEMBER(MemberType::INT, totalWayPoint),
    MEMBER(MemberType::INT, initialWayPoint),
    MEMBER(MemberType::FLOAT, distanceOffset),
    MEMBER(MemberType::FLOAT, speed),
    MEMBER(MemberType::FLOAT, rotationSpeed)

};

GENERATE_BODY_IMPL(DroneWayPoints);

void DroneWayPoints::Start() {
	ownerTransform = GetOwner().GetComponent<ComponentTransform>();

    //Getting waypoints
    for (int i = 0; i < totalWayPoint; i++) {
        GameObject *wayPoint = GameplaySystems::GetGameObject( (initialSubString + std::to_string(i)).c_str());
        if (wayPoint) {
            wayPoints.push_back(wayPoint);
        }
    }
    //Checking for avoiding offset
    currentWayPoint = initialWayPoint;
    if (initialWayPoint > totalWayPoint) {
        currentWayPoint = 0;
    }
}

void DroneWayPoints::Update() {
    CheckWayPoint();
}


void DroneWayPoints::CheckWayPoint() 
{
    if (!ownerTransform || wayPoints.size() == 0){
        return;
    }

    if (wayPoints[currentWayPoint]) {
        ComponentTransform *destinyTransform = wayPoints[currentWayPoint]->GetComponent<ComponentTransform>();
        if (destinyTransform) {
            float distance =  ownerTransform->GetGlobalPosition().Distance(destinyTransform->GetGlobalPosition());
            if (distance <= distanceOffset) {
                currentWayPoint++;
                if (currentWayPoint > wayPoints.size() - 1) {
                    currentWayPoint = 0;
                }
                return;
            }
            
            Seek(ownerTransform->GetGlobalPosition(),destinyTransform->GetGlobalPosition());
        }

    }

}

void DroneWayPoints::Seek(const float3& currentPosition, const float3& newPosition)
{
    float3 direction = newPosition - currentPosition;
    float3 velocity = direction.Normalized() * speed;
    ownerTransform->SetGlobalPosition(currentPosition + (velocity * Time::GetDeltaTime()));

    Quat targetRotation = Quat::LookAt(float3(0, 0, 1), velocity.Normalized(), float3(0, 1, 0), float3(0, 1, 0));
    Quat rotation = Quat::Slerp(ownerTransform->GetGlobalRotation(), targetRotation, Min(Time::GetDeltaTime() / Max(rotationSpeed, 0.000001f), 1.0f));
	ownerTransform->SetGlobalRotation(rotation);
}
