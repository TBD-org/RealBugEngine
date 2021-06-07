#pragma once

#include "Utils/UID.h"
#include "Creator.h"
#include "Member.h"
#include "TesseractEvent.h"

#include <vector>

#define GENERATE_BODY(classname)                            \
public:                                                     \
	classname(GameObject* owner_) : Script(owner_) {}       \
	const std::vector<Member>& GetMembers() const override; \
                                                            \
private:                                                    \
	static const CreatorImplementation<classname> creator;

#define GENERATE_BODY_IMPL(classname)                                      \
	const CreatorImplementation<classname> classname::creator(#classname); \
                                                                           \
	const std::vector<Member>& classname::GetMembers() const {             \
		return _members;                                                   \
	}

class Script {
public:
	Script(GameObject* owner_)
		: owner(owner_) {}

	virtual ~Script() = default;

	virtual const std::vector<Member>& GetMembers() const = 0;

	GameObject& GetOwner() const {
		return *owner;
	}

	virtual void ReceiveEvent(TesseractEvent& e) {}

	virtual void Update() = 0;
	virtual void Start() = 0;
	virtual void OnButtonClick() {}
	virtual void OnToggled(bool val) {}
	virtual void OnValueChanged() {}
	virtual void OnAnimationFinished() {}
	virtual void OnAnimationSecondaryFinished() {}
	virtual void OnCollision(GameObject& collidedWith, float3 contactPoint, float3 normalCollision) {}

private:
	GameObject* owner = nullptr;
};
