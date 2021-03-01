#include "ModulePrograms.h"

#include "Globals.h"
#include "Application.h"
#include "Utils/Logging.h"
#include "Modules/ModuleFiles.h"

#include "GL/glew.h"

#include "Utils/Leaks.h"

static unsigned CreateShader( unsigned type, const char* filePath) {

	LOG("Creating shader from file: \"%s\"...", filePath);

	Buffer<char> sourceBuffer = App->files->Load(filePath);
	char* source = sourceBuffer.Data();

	unsigned shaderId = glCreateShader(type);
	if (shaderId == 0) {
		return 0;
	}
	std::string v = "#version 460\n";
	std::string defineVertexShader   = "#define VERTEX  \n";
	std::string defineFragmentShader = "#define FRAGMENT\n";

	std::string shaderDefine = (type == GL_VERTEX_SHADER) ? defineVertexShader : defineFragmentShader;

	GLchar const * shaderStrings[3] = {v.c_str(),shaderDefine.c_str(), source};
	GLint shaderStringLengths[3] = {v.size(),shaderDefine.size(), strlen(source)};

	glShaderSource(shaderId, 3, shaderStrings, shaderStringLengths);
	glCompileShader(shaderId);

	int res = GL_FALSE;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &res);

	if (res == GL_FALSE) {
		int len = 0;
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &len);

		if (len > 1) {
			int written = 0;
			Buffer<char> info = Buffer<char>(len);
			glGetShaderInfoLog(shaderId, len, &written, info.Data());
			LOG("Log Info: %s", info.Data());
		}
		return 0;
	}

	LOG("Shader created successfuly.");
	return shaderId;
}

static unsigned CreateProgram(const char* shaderFilePath) {
	LOG("Creating program...");

	// Compile the shaders and delete them at the end
	LOG("Compiling Vertex...");
	unsigned vertexShader = CreateShader(GL_VERTEX_SHADER, shaderFilePath);
	DEFER {
		glDeleteShader(vertexShader);
	};
	LOG("Compiling Fragment...");
	unsigned fragmentShader = CreateShader(GL_FRAGMENT_SHADER, shaderFilePath);
	DEFER {
		glDeleteShader(fragmentShader);
	};

	// Link the program
	LOG("Linking program...");
	unsigned programId = glCreateProgram();
	glAttachShader(programId, vertexShader);
	glAttachShader(programId, fragmentShader);
	glLinkProgram(programId);
	int res = GL_FALSE;
	glGetProgramiv(programId, GL_LINK_STATUS, &res);
	if (res == GL_FALSE) {
		int len = 0;
		glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &len);
		if (len > 0) {
			int written = 0;
			Buffer<char> info = Buffer<char>(len);
			glGetProgramInfoLog(programId, len, &written, info.Data());
			LOG("Program Log Info: %s", info.Data());
		}

		LOG("Error linking program.");
	} else {
		LOG("Program linked.");
	}

	return programId;
}

bool ModulePrograms::Start() {
	defaultProgram = CreateProgram("Shaders/test.glsl");
	phongPbrProgram = CreateProgram("Shaders/phong.glsl");
	skyboxProgram = CreateProgram("Shaders/skybox.glsl");

	return true;
}

bool ModulePrograms::CleanUp() {
	glDeleteProgram(defaultProgram);
	glDeleteProgram(phongPbrProgram);
	glDeleteProgram(skyboxProgram);
	return true;
}
