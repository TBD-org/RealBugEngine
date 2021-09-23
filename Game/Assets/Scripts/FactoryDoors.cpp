#include "FactoryDoors.h"

#include "GameObject.h"
#include "GameplaySystems.h"

EXPOSE_MEMBERS(FactoryDoors) {};

GENERATE_BODY_IMPL(FactoryDoors);

void FactoryDoors::Start() {
	animation = GetOwner().GetComponent<ComponentAnimation>();
}

void FactoryDoors::Update() {
	
}

void FactoryDoors::Open() {
	if (animation) animation->SendTrigger("ClosedOpening");
}