#pragma once

#include "Resource.h"

#include "Math/float2.h"
#include <unordered_map>
#include <string>

struct Character {
	unsigned int textureID = 0;
	float2 size = float2::zero;
	float2 bearing = float2::zero;
	unsigned int advance = 0;
};

class ResourceFont : public Resource {
public:
	REGISTER_RESOURCE(ResourceFont, ResourceType::FONT);

	void FinishLoading() override;
	void Unload() override;

public:
	std::unordered_map<char, Character> characters; //Should character be a pointer? It's not a built-in type
	std::string name = "";
};
