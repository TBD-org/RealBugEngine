#pragma once

#include "Resources/Resource.h"

#include "Geometry/Triangle.h"
#include "Math/float4x4.h"
#include "Math/float3.h"
#include "Math/float2.h"
#include <string>
#include <vector>

class ResourceMesh : public Resource {
public:
	struct Bone {
		float4x4 transform;
		std::string boneName;
	};

	struct Attach {
		unsigned numBones = 0u;
		unsigned bones[4] = {0u, 0u, 0u, 0u};
		float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	};

	struct Vertex {
		float3 position = {0.0f, 0.0f, 0.0f};
		float3 normal = {0.0f, 0.0f, 0.0f};
		float3 tangent = {0.0f, 0.0f, 0.0f};
		float2 uv = {0.0f, 0.0f};
		unsigned bones[4] = {0u, 0u, 0u, 0u};
		float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	};

public:
	REGISTER_RESOURCE(ResourceMesh, ResourceType::MESH);

	void Load() override;
	void FinishLoading() override;
	void Unload() override;

	std::vector<Triangle> ExtractTriangles(const float4x4& modelMatrix) const;

public:
	unsigned vbo = 0;
	unsigned ebo = 0;
	unsigned vao = 0;

	std::vector<Bone> bones;
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;
};