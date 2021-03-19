#pragma once
#include "GameObject.h"
struct Event {
public:
	union {
		struct {
			GameObject* ptr;
		} objPtr;
		struct
		{
			int x, y;
		} point2d;
	};

	enum class EventType {
		GameObject_Destroyed,
		Pressed_Play,
		Pressed_Pause,
		Pressed_Resume,
		Pressed_Step,
		Pressed_Stop,
		File_Dropped

	} type;

	static const char* TypeToString(EventType v) {
		switch (v) {
		case EventType::GameObject_Destroyed:
			return "GameObject destroyed";
		case EventType::File_Dropped:
			return "File dropped";
		case EventType::Pressed_Play:
			return "Pressed play";
		case EventType::Pressed_Pause:
			return "Pressed pause";
		case EventType::Pressed_Resume:
			return "Pressed resume";
		case EventType::Pressed_Step:
			return "Pressed step";
		case EventType::Pressed_Stop:
			return "Pressed stop";
		default:
			return "Unkown event type";
		}
	}

	Event(EventType aType)
		: type(aType) {
	}
	Event(EventType aType, float2 aFloat2)
		: type(aType) {
		point2d.x = aFloat2.x;
		point2d.y = aFloat2.y;
	}

	Event(EventType aType, GameObject* obj)
		: type(aType) {
		objPtr.ptr = obj;
	}

	~Event() {
	}
};
