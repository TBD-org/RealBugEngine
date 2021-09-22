#include "ModulePrograms.h"

#include "Globals.h"
#include "Application.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleTime.h"
#include "Utils/Logging.h"
#include "Utils/PathUtils.h"
#include "Utils/StringBlocksParser.h"

#include "GL/glew.h"
#include <string>

#include "Utils/Leaks.h"

static unsigned CompileShader(unsigned type, const char* filePath, const char* snippets) {
	LOG("Creating shader from snippets: \"%s\"...", snippets);

	parsb_options options;
	options.line_directives = false;
	parsb_context* blocks = parsb_create_context(options);
	DEFER {
		parsb_destroy_context(blocks);
	};
	parsb_add_blocks_from_file(blocks, filePath);

	// Add version
	parsb_add_block(blocks, "prefix", GLSL_VERSION "\n");
	std::string s = snippets;
	std::string finalSnippet = "prefix " + s;

	// Concatenate
	const char* shaderData = parsb_get_blocks(blocks, finalSnippet.c_str());
	if (shaderData == nullptr) {
		LOG("Error reading blocks %s from %s", finalSnippet.c_str(), filePath);
		return 0;
	}

	unsigned shaderId = glCreateShader(type);
	if (shaderId == 0) return 0;

	glShaderSource(shaderId, 1, &shaderData, 0);
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

	LOG("Shader created successfully.");
	return shaderId;
}

void ModulePrograms::LoadShaderBinFile() {
	// Clean file on Start
	Buffer<char> cleanBuffer;
	if (!App->files->IsDirectory(LIBRARY_PATH)) {
		App->files->CreateFolder(LIBRARY_PATH);
	}
	App->files->Save(filePath, cleanBuffer);

	// Save all .shader from Assets/Shaders/ into one bin file
	std::vector<std::string> files = App->files->GetFilesInFolder("Assets/Shaders/");
	for (std::string file : files) {
		if (PathUtils::GetFileExtension(file.c_str()) == ".shader") {
			Buffer<char> buffer = App->files->Load(("Assets/Shaders/" + file).c_str());
			App->files->Save(filePath, buffer, true);
		}
	}
}

void ModulePrograms::LoadShaders() {
	MSTimer timer;
	timer.Start();

#if !GAME
	LoadShaderBinFile();
#endif

	// SkyBox shaders
	hdrToCubemap = new ProgramHDRToCubemap(CreateProgram(filePath, "vertCube", "fragFunctionIBL fragHDRToCubemap"));
	irradiance = new ProgramIrradiance(CreateProgram(filePath, "vertCube", "fragFunctionIBL fragIrradianceMap"));
	preFilteredMap = new ProgramPreFilteredMap(CreateProgram(filePath, "vertCube", "fragFunctionIBL fragPreFilteredMap"));
	environmentBRDF = new ProgramEnvironmentBRDF(CreateProgram(filePath, "vertScreen", "fragFunctionIBL fragEnvironmentBRDF"));
	skybox = new ProgramSkybox(CreateProgram(filePath, "vertCube", "gammaCorrection fragSkybox"));

	// Unlit Shader
	unlit = new ProgramUnlit(CreateProgram(filePath, "vertUnlit", "gammaCorrection fragFunctionEmptyDissolve fragUnlit"));

	// Volumetric Light Shader
	volumetricLight = new ProgramVolumetricLight(CreateProgram(filePath, "vertVolumetricLight", "gammaCorrection fragVolumetricLight"));

	// General shaders
	phongNotNormal = new ProgramStandardPhong(CreateProgram(filePath, "vertVarCommon vertMainCommon", "gammaCorrection fragVarStandard fragVarSpecular fragMainPhong"));
	phongNormal = new ProgramStandardPhong(CreateProgram(filePath, "vertVarCommon vertMainNormal", "gammaCorrection fragVarStandard fragVarSpecular fragMainPhong"));
	standardNotNormal = new ProgramStandardMetallic(CreateProgram(filePath, "vertVarCommon vertMainCommon", "gammaCorrection fragVarStandard fragVarMetallic fragFunctionLight fragFunctionEmptyDissolve fragMainMetallic"));
	standardNormal = new ProgramStandardMetallic(CreateProgram(filePath, "vertVarCommon vertMainNormal", "gammaCorrection fragVarStandard fragVarMetallic fragFunctionLight fragFunctionEmptyDissolve fragMainMetallic"));
	specularNotNormal = new ProgramStandardSpecular(CreateProgram(filePath, "vertVarCommon vertMainCommon", "gammaCorrection fragVarStandard fragVarSpecular fragFunctionLight fragMainSpecular"));
	specularNormal = new ProgramStandardSpecular(CreateProgram(filePath, "vertVarCommon vertMainNormal", "gammaCorrection fragVarStandard fragVarSpecular fragFunctionLight fragMainSpecular"));

	// Dissolve shaders. Maybe another one for Normals
	dissolveStandard = new ProgramStandardDissolve(CreateProgram(filePath, "vertVarCommon vertMainNormal", "gammaCorrection fragVarStandard fragVarMetallic fragFunctionLight fragFunctionDissolveCommon fragFunctionDissolveFunction fragMainMetallic"));
	dissolveUnlit = new ProgramUnlitDissolve(CreateProgram(filePath, "vertUnlit", "gammaCorrection fragFunctionDissolveCommon fragFunctionDissolveFunction fragUnlit"));

	// Depth Prepass Shaders
	depthPrepass = new ProgramDepthPrepass(CreateProgram(filePath, "vertVarCommon vertMainCommon", "fragFunctionEmptyDissolveDepth fragDepthPrepass"));
	depthPrepassConvertTextures = new ProgramDepthPrepassConvertTextures(CreateProgram(filePath, "vertScreen", "fragDepthPrepassConvertTextures"));
	depthPrepassDissolve = new ProgramDepthPrepassDissolve(CreateProgram(filePath, "vertVarCommon vertMainCommon", "fragFunctionDissolveCommon fragFunctionDepthDissolve fragDepthPrepass"));

	// SSAO Shaders
	ssao = new ProgramSSAO(CreateProgram(filePath, "vertScreen", "fragSSAO"));
	blur = new ProgramBlur(CreateProgram(filePath, "vertScreen", "fragGaussianBlur"));

	// Bloom shaders
	bloomCombine = new ProgramBloomCombine(CreateProgram(filePath, "vertScreen", "fragBloomCombine"));

	// Post-processing Shaders
	postprocess = new ProgramPostprocess(CreateProgram(filePath, "vertScreen", "fragPostprocess"));
	colorCorrection = new ProgramColorCorrection(CreateProgram(filePath, "vertScreen", "gammaCorrection fragColorCorrection"));

	// Fog Shaders
	heightFog = new ProgramHeightFog(CreateProgram(filePath, "vertScreen", "gammaCorrection fragHeightFog"));

	// Shadow Shaders
	shadowMap = CreateProgram(filePath, "vertDepthMap", "fragDepthMap");

	//UI shaders
	textUI = new ProgramTextUI(CreateProgram(filePath, "vertTextUI", "gammaCorrection fragTextUI"));
	imageUI = new ProgramImageUI(CreateProgram(filePath, "vertImageUI", "gammaCorrection fragImageUI"));

	// Engine Shaders
	drawTexture = new ProgramDrawTexture(CreateProgram(filePath, "vertScreen", "fragDrawTexture"));

	// Particle Shaders
	billboard = new ProgramBillboard(CreateProgram(filePath, "billboardVertex", "gammaCorrection billboardFragment"));
	trail = new ProgramTrail(CreateProgram(filePath, "trailVertex", "gammaCorrection trailFragment"));

	unsigned timeMs = timer.Stop();
	LOG("Shaders loaded in %ums", timeMs);
}

void ModulePrograms::UnloadShaders() {
	RELEASE(hdrToCubemap);
	RELEASE(irradiance);
	RELEASE(preFilteredMap);
	RELEASE(environmentBRDF);
	RELEASE(skybox);

	RELEASE(unlit);

	RELEASE(volumetricLight);

	RELEASE(phongNormal);
	RELEASE(phongNotNormal);
	RELEASE(standardNormal);
	RELEASE(standardNotNormal);
	RELEASE(specularNormal);
	RELEASE(specularNotNormal);

	RELEASE(depthPrepass);
	RELEASE(depthPrepassConvertTextures);
	RELEASE(depthPrepassDissolve);

	RELEASE(dissolveStandard);
	RELEASE(dissolveUnlit);

	RELEASE(ssao);
	RELEASE(blur);

	RELEASE(bloomCombine);

	RELEASE(colorCorrection);

	RELEASE(postprocess);

	RELEASE(heightFog);

	glDeleteProgram(shadowMap);

	RELEASE(textUI);
	RELEASE(imageUI);

	RELEASE(drawTexture);

	RELEASE(billboard);
	RELEASE(trail);
}

unsigned ModulePrograms::CreateProgram(const char* shaderFile, const char* vertexSnippets, const char* fragmentSnippets) {
	LOG("Creating program...");

	// Compile the shaders and delete them at the end
	LOG("Compiling shaders...");
	unsigned vertexShader = CompileShader(GL_VERTEX_SHADER, shaderFile, vertexSnippets);
	DEFER {
		glDeleteShader(vertexShader);
	};
	unsigned fragmentShader = CompileShader(GL_FRAGMENT_SHADER, shaderFile, fragmentSnippets);
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
	LoadShaders();
	return true;
}

void ModulePrograms::DeleteProgram(unsigned int IdProgram) {
	glDeleteProgram(IdProgram);
}

bool ModulePrograms::CleanUp() {
	UnloadShaders();
	return true;
}