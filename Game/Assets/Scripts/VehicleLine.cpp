#include "VehicleLine.h"

#include "GameObject.h"
#include "GameplaySystems.h"
#include "Resources/ResourcePrefab.h"

#include <math.h>

EXPOSE_MEMBERS(VehicleLine) {
    MEMBER(MemberType::PREFAB_RESOURCE_UID, vehicle1),
    MEMBER(MemberType::PREFAB_RESOURCE_UID, vehicle2),
    MEMBER(MemberType::PREFAB_RESOURCE_UID, vehicle3),
    MEMBER(MemberType::PREFAB_RESOURCE_UID, vehicle4),
    MEMBER(MemberType::PREFAB_RESOURCE_UID, vehicle5),
    MEMBER(MemberType::PREFAB_RESOURCE_UID, vehicle6),
    MEMBER(MemberType::PREFAB_RESOURCE_UID, vehicle7),
    MEMBER(MemberType::PREFAB_RESOURCE_UID, vehicle8),
    MEMBER(MemberType::PREFAB_RESOURCE_UID, vehicle9),
    MEMBER(MemberType::FLOAT, speed),
    MEMBER(MemberType::FLOAT, timeBetweenSpawns)
};

GENERATE_BODY_IMPL(VehicleLine);

void VehicleLine::Start() {

    vehiclesUID[0] = vehicle1;
    vehiclesUID[1] = vehicle2;
    vehiclesUID[2] = vehicle3;
    vehiclesUID[3] = vehicle4;
    vehiclesUID[4] = vehicle5;
    vehiclesUID[5] = vehicle6;
    vehiclesUID[6] = vehicle7;
    vehiclesUID[7] = vehicle8;
    vehiclesUID[8] = vehicle9;
	
}

void VehicleLine::Update() {

    if (timeToSpawn <= 0) {
        while (actualVehicle == previousVehicle) {
            actualVehicle = rand() % 9;
        }
        ResourcePrefab* prefabVehicle = GameplaySystems::GetResource<ResourcePrefab>(vehiclesUID[actualVehicle]);
        GameObject* goVehicle = nullptr;
        if (prefabVehicle) goVehicle = GameplaySystems::GetGameObject(prefabVehicle->BuildPrefab(&GetOwner()));
        if(goVehicle) vehicles.push_back(goVehicle);
        timeToSpawn = timeBetweenSpawns;
        previousVehicle = actualVehicle;
    } 
    else {
        timeToSpawn -= Time::GetDeltaTime();
    }
    UpdateVehicles();
	
}

void VehicleLine::UpdateVehicles()
{
    int vehiclesToClear = 0;
    for (GameObject* vehicle : vehicles) {
        ComponentTransform* transformVehicle = vehicle->GetComponent<ComponentTransform>();
        if (!transformVehicle) return;
        float3 position = transformVehicle->GetGlobalPosition();
        float3 direction = transformVehicle->GetGlobalRotation() * float3(1, 0, 0);
        float3 velocity = direction.Normalized() * speed;
        position += velocity * Time::GetDeltaTime();
        transformVehicle->SetGlobalPosition(position);
        if (VehicleOutsideMap(transformVehicle->GetGlobalPosition())){
            GameplaySystems::DestroyGameObject(vehicle);
            ++vehiclesToClear;
        }
    }
    for (int i = 0; i < vehiclesToClear; ++i) {
        vehicles.erase(vehicles.begin());
    }
    
}

bool VehicleLine::VehicleOutsideMap(const float3 posVehicle)
{
    if (posVehicle.x < -600 || posVehicle.x > 600) return true;
    if (posVehicle.y < -600 || posVehicle.y > 600) return true;
    if (posVehicle.z < -600 || posVehicle.z > 600) return true;
    return false;
}
