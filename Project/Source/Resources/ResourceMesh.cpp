#include "ResourceMesh.h"

#include "Globals.h"
#include "Application.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
#include "Utils/MSTimer.h"
#include "Modules/ModuleFiles.h"

#include "GL/glew.h"
#include "Math/Quat.h"

#include "Utils/Leaks.h"

const int positionsSizeQuantity = 3;
const int normalSizeQuantity = 3;
const int tangentSizeQuantity = 3;
const int uvSizeQuantity = 2;
const int bonesIDSizeQuantity = 4;
const int weightSizeQuantity = 4;

const unsigned positionSize = sizeof(float) * positionsSizeQuantity;
const unsigned normalSize = sizeof(float) * normalSizeQuantity;
const unsigned tangentSize = sizeof(float) * tangentSizeQuantity;
const unsigned uvSize = sizeof(float) * uvSizeQuantity;
const unsigned bonesIDSize = sizeof(unsigned) * bonesIDSizeQuantity;
const unsigned weightsSize = sizeof(float) * weightSizeQuantity;

const unsigned nonPosVertexAttributesSize = normalSize + tangentSize + uvSize + bonesIDSize + weightsSize;

void ResourceMesh::Load() {
	// Timer to measure loading a mesh
	MSTimer timer;
	timer.Start();
	std::string filePath = GetResourceFilePath();
	LOG("Loading mesh from path: \"%s\".", filePath.c_str());

	// Load file
	Buffer<char> buffer = App->files->Load(filePath.c_str());
	char* cursor = buffer.Data();

	// Header
	unsigned numVertices = *((unsigned*) cursor);
	cursor += sizeof(unsigned);
	unsigned numIndices = *((unsigned*) cursor);
	cursor += sizeof(unsigned);
	unsigned numBones = *((unsigned*) cursor);
	cursor += sizeof(unsigned);

	// Bones
	bones.resize(numBones);
	for (unsigned i = 0; i < numBones; ++i) {
		float3 position, scaling;
		Quat rotation;

		unsigned lengthName = *((unsigned*) cursor);
		cursor += sizeof(unsigned);

		char* name = (char*) malloc((lengthName + 1) * sizeof(char));

		memcpy_s(name, lengthName * sizeof(char), cursor, lengthName * sizeof(char));
		cursor += FILENAME_MAX * sizeof(char);

		name[lengthName] = '\0';

		// Translation
		position.x = *((float*) cursor);
		cursor += sizeof(float);
		position.y = *((float*) cursor);
		cursor += sizeof(float);
		position.z = *((float*) cursor);
		cursor += sizeof(float);

		// Scaling
		scaling.x = *((float*) cursor);
		cursor += sizeof(float);
		scaling.y = *((float*) cursor);
		cursor += sizeof(float);
		scaling.z = *((float*) cursor);
		cursor += sizeof(float);

		// Rotation
		rotation.x = *((float*) cursor);
		cursor += sizeof(float);
		rotation.y = *((float*) cursor);
		cursor += sizeof(float);
		rotation.z = *((float*) cursor);
		cursor += sizeof(float);
		rotation.w = *((float*) cursor);
		cursor += sizeof(float);

		bones[i].transform = float4x4::FromTRS(position, rotation, scaling);
		bones[i].boneName = name;

		free(name);
	}

	LOG("Loading %i vertices...", numVertices);

	// Vertices
	vertices.resize(numVertices);
	for (unsigned i = 0; i < numVertices; ++i) {
		vertices[i] = *((Vertex*) cursor);
		cursor += sizeof(Vertex);
	}

	// Indices
	indices.resize(numIndices);
	for (unsigned i = 0; i < numIndices; ++i) {
		indices[i] = *((unsigned*) cursor);
		cursor += sizeof(unsigned);
	}

	unsigned timeMs = timer.Stop();
	LOG("Mesh loaded in %ums", timeMs);
}

void ResourceMesh::FinishLoading() {
	// Create VAO
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	// Load VBO
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), (bones.size() > 0) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	// Load EBO
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

	// Load vertex attributes
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) positionSize);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (positionSize + normalSize));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (positionSize + normalSize + tangentSize));
	glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*) (positionSize + normalSize + tangentSize + uvSize));
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (positionSize + normalSize + tangentSize + uvSize + bonesIDSize));

	// Unbind VAO
	glBindVertexArray(0);
}

void ResourceMesh::Unload() {
	bones.clear();
	vertices.clear();
	indices.clear();

	if (vao) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}

	if (vbo) {
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}
	
	if (ebo) {
		glDeleteBuffers(1, &ebo);
		ebo = 0;
	}
}

std::vector<Triangle> ResourceMesh::ExtractTriangles(const float4x4& modelMatrix) const {
	std::string filePath = GetResourceFilePath();

	// Load file
	Buffer<char> buffer = App->files->Load(filePath.c_str());
	char* cursor = buffer.Data();

	// Header
	unsigned numVertices = *((unsigned*) cursor);
	cursor += sizeof(unsigned);
	unsigned numIndices = *((unsigned*) cursor);
	cursor += sizeof(unsigned);
	unsigned numBones = *((unsigned*) cursor);
	cursor += sizeof(unsigned);

	// Bones
	for (unsigned i = 0; i < numBones; ++i) {
		// bone name size
		cursor += sizeof(unsigned);
		// bone name
		cursor += FILENAME_MAX * sizeof(char);
		// translation, scaling, rotation
		cursor += sizeof(float) * 10;
	}

	// Vertices
	std::vector<float3> vertices;
	for (unsigned i = 0; i < numVertices; ++i) {
		float vertex[3] = {};
		vertex[0] = *((float*) cursor);
		cursor += sizeof(float);
		vertex[1] = *((float*) cursor);
		cursor += sizeof(float);
		vertex[2] = *((float*) cursor);
		cursor += sizeof(float);
		cursor += nonPosVertexAttributesSize;

		vertices.push_back((modelMatrix * float4(vertex[0], vertex[1], vertex[2], 1)).xyz());
	}

	std::vector<Triangle> triangles;
	triangles.reserve(numIndices / 3);
	for (unsigned i = 0; i < numIndices / 3; ++i) {
		unsigned triangeIndices[3] = {};
		triangeIndices[0] = *((unsigned*) cursor);
		cursor += sizeof(unsigned);
		triangeIndices[1] = *((unsigned*) cursor);
		cursor += sizeof(unsigned);
		triangeIndices[2] = *((unsigned*) cursor);
		cursor += sizeof(unsigned);
		triangles.push_back(Triangle(vertices[triangeIndices[0]], vertices[triangeIndices[1]], vertices[triangeIndices[2]]));
	}

	return triangles;
}
