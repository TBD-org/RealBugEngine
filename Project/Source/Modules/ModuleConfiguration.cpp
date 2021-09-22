#include "ModuleConfiguration.h"

#include "Application.h"
#include "Utils/Logging.h"
#include "Utils/MSTimer.h"
#include "Utils/JsonValue.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleScene.h"
#include "Modules/ModuleTime.h"
#include "Modules/ModulePhysics.h"
#include "Modules/ModuleRender.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

#include "Utils/Leaks.h"

#define JSON_TAG_NAME "Name"
#define JSON_TAG_ORGANIZATION "Organization"
#define JSON_TAG_START_SCENE_ID "StartSceneId"
#define JSON_TAG_LIMIT_FRAMERATE "LimitFramerate"
#define JSON_TAG_MAX_FPS "MaxFPS"
#define JSON_TAG_VSYNC "VSync"
#define JSON_TAG_GRAVITY "Gravity"
#define JSON_TAG_SSAO_ACTIVE "SSAOActive"
#define JSON_TAG_SSAO_RANGE "SSAORange"
#define JSON_TAG_SSAO_BIAS "SSAOBias"
#define JSON_TAG_SSAO_POWER "SSAOPower"
#define JSON_TAG_SSAO_DIRECT_LIGHTING_STRENGTH "SSAODirectLightingStrength"
#define JSON_TAG_BLOOM_ACTIVE "BloomActive"
#define JSON_TAG_BLOOM_QUALITY "BloomQuality"
#define JSON_TAG_BLOOM_THRESHOLD "BloomThreshold"
#define JSON_TAG_BLOOM_INTENSITY "BloomIntensity"
#define JSON_TAG_BLOOM_SIZE_MULTIPLIER "BloomSizeMultiplier"
#define JSON_TAG_BLOOM_VERY_SMALL_WEIGHT "BloomVerySmallWeight"
#define JSON_TAG_BLOOM_SMALL_WEIGHT "BloomSmallWeight"
#define JSON_TAG_BLOOM_MEDIUM_WEIGHT "BloomMediumWeight"
#define JSON_TAG_BLOOM_LARGE_WEIGHT "BloomLargeWeight"
#define JSON_TAG_BLOOM_VERY_LARGE_WEIGHT "BloomVeryLargeWeight"
#define JSON_TAG_MSAA_ACTIVE "MSAAActive"
#define JSON_TAG_MSAA_SAMPLE_TYPE "MSAASampleType"
#define JSON_TAG_CHROMATIC_ABERRATION_ACTIVE "ChromaticAberrationActive"
#define JSON_TAG_CHROMATIC_ABERRATION_STRENGTH "ChromaticAberrationStrength"

bool ModuleConfiguration::Init() {
	LoadConfiguration();
	return true;
}

void ModuleConfiguration::LoadConfiguration() {
	// Timer to measure loading the config file
	MSTimer timer;
	timer.Start();
	LOG("Loading configuration from config.json...");

	// Read from file
	Buffer<char> buffer = App->files->Load(CONFIGURATION_FILE_NAME);

	if (buffer.Size() == 0) return;

	// Parse document from file
	rapidjson::Document document;
	document.ParseInsitu<rapidjson::kParseNanAndInfFlag>(buffer.Data());
	if (document.HasParseError()) {
		LOG("Error parsing JSON: %s (offset: %u)", rapidjson::GetParseError_En(document.GetParseError()), document.GetErrorOffset());
		return;
	}
	JsonValue jConfig(document, document);

	// Load configuration
	std::string appName = jConfig[JSON_TAG_NAME];
	std::strncpy(App->appName, appName.c_str(), sizeof(App->appName));

	std::string organization = jConfig[JSON_TAG_ORGANIZATION];
	std::strncpy(App->organization, organization.c_str(), sizeof(App->organization));

	App->scene->startSceneId = jConfig[JSON_TAG_START_SCENE_ID];

	App->time->limitFramerate = jConfig[JSON_TAG_LIMIT_FRAMERATE];
	App->time->maxFps = jConfig[JSON_TAG_MAX_FPS];
	App->time->vsync = jConfig[JSON_TAG_VSYNC];

	App->physics->gravity = jConfig[JSON_TAG_GRAVITY];

	App->renderer->ssaoActive = jConfig[JSON_TAG_SSAO_ACTIVE];
	App->renderer->ssaoRange = jConfig[JSON_TAG_SSAO_RANGE];
	App->renderer->ssaoBias = jConfig[JSON_TAG_SSAO_BIAS];
	App->renderer->ssaoPower = jConfig[JSON_TAG_SSAO_POWER];
	App->renderer->ssaoDirectLightingStrength = jConfig[JSON_TAG_SSAO_DIRECT_LIGHTING_STRENGTH];

	App->renderer->bloomActive = jConfig[JSON_TAG_BLOOM_ACTIVE];
	App->renderer->bloomThreshold = jConfig[JSON_TAG_BLOOM_THRESHOLD];
	App->renderer->bloomIntensity = jConfig[JSON_TAG_BLOOM_INTENSITY];
	App->renderer->bloomSizeMultiplier = jConfig[JSON_TAG_BLOOM_SIZE_MULTIPLIER];
	App->renderer->bloomVerySmallWeight = jConfig[JSON_TAG_BLOOM_VERY_SMALL_WEIGHT];
	App->renderer->bloomSmallWeight = jConfig[JSON_TAG_BLOOM_SMALL_WEIGHT];
	App->renderer->bloomMediumWeight = jConfig[JSON_TAG_BLOOM_MEDIUM_WEIGHT];
	App->renderer->bloomLargeWeight = jConfig[JSON_TAG_BLOOM_LARGE_WEIGHT];
	App->renderer->bloomVeryLargeWeight = jConfig[JSON_TAG_BLOOM_VERY_LARGE_WEIGHT];

	App->renderer->msaaActive = jConfig[JSON_TAG_MSAA_ACTIVE];
	App->renderer->msaaSampleType = (MSAA_SAMPLES_TYPE)(int) jConfig[JSON_TAG_MSAA_SAMPLE_TYPE];

	App->renderer->chromaticAberrationActive = jConfig[JSON_TAG_CHROMATIC_ABERRATION_ACTIVE];
	App->renderer->chromaticAberrationStrength = jConfig[JSON_TAG_CHROMATIC_ABERRATION_STRENGTH];

	unsigned timeMs = timer.Stop();
	LOG("Configuration loaded in %ums.", timeMs);
}

void ModuleConfiguration::SaveConfiguration() {
	// Timer to measure saving the config file
	MSTimer timer;
	timer.Start();
	LOG("Saving configuration to config.json...");

	// Create document
	rapidjson::Document document;
	document.SetObject();
	JsonValue jConfig(document, document);

	// Save configuration
	jConfig[JSON_TAG_NAME] = App->appName;

	jConfig[JSON_TAG_ORGANIZATION] = App->organization;

	jConfig[JSON_TAG_START_SCENE_ID] = App->scene->startSceneId;

	jConfig[JSON_TAG_LIMIT_FRAMERATE] = App->time->limitFramerate;
	jConfig[JSON_TAG_MAX_FPS] = App->time->maxFps;
	jConfig[JSON_TAG_VSYNC] = App->time->vsync;

	jConfig[JSON_TAG_GRAVITY] = App->physics->gravity;

	jConfig[JSON_TAG_SSAO_ACTIVE] = App->renderer->ssaoActive;
	jConfig[JSON_TAG_SSAO_RANGE] = App->renderer->ssaoRange;
	jConfig[JSON_TAG_SSAO_BIAS] = App->renderer->ssaoBias;
	jConfig[JSON_TAG_SSAO_POWER] = App->renderer->ssaoPower;
	jConfig[JSON_TAG_SSAO_DIRECT_LIGHTING_STRENGTH] = App->renderer->ssaoDirectLightingStrength;

	jConfig[JSON_TAG_BLOOM_ACTIVE] = App->renderer->bloomActive;
	jConfig[JSON_TAG_BLOOM_THRESHOLD] = App->renderer->bloomThreshold;
	jConfig[JSON_TAG_BLOOM_INTENSITY] = App->renderer->bloomIntensity;
	jConfig[JSON_TAG_BLOOM_SIZE_MULTIPLIER] = App->renderer->bloomSizeMultiplier;
	jConfig[JSON_TAG_BLOOM_VERY_SMALL_WEIGHT] = App->renderer->bloomVerySmallWeight;
	jConfig[JSON_TAG_BLOOM_SMALL_WEIGHT] = App->renderer->bloomSmallWeight;
	jConfig[JSON_TAG_BLOOM_MEDIUM_WEIGHT] = App->renderer->bloomMediumWeight;
	jConfig[JSON_TAG_BLOOM_LARGE_WEIGHT] = App->renderer->bloomLargeWeight;
	jConfig[JSON_TAG_BLOOM_VERY_LARGE_WEIGHT] = App->renderer->bloomVeryLargeWeight;

	jConfig[JSON_TAG_MSAA_ACTIVE] = App->renderer->msaaActive;
	jConfig[JSON_TAG_MSAA_SAMPLE_TYPE] = (int) App->renderer->msaaSampleType;

	jConfig[JSON_TAG_CHROMATIC_ABERRATION_ACTIVE] = App->renderer->chromaticAberrationActive;
	jConfig[JSON_TAG_CHROMATIC_ABERRATION_STRENGTH] = App->renderer->chromaticAberrationStrength;

	// Write document to buffer
	rapidjson::StringBuffer stringBuffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator, rapidjson::kWriteNanAndInfFlag> writer(stringBuffer);
	document.Accept(writer);

	App->files->Save(CONFIGURATION_FILE_NAME, stringBuffer.GetString(), stringBuffer.GetSize());

	unsigned timeMs = timer.Stop();
	LOG("Configuration saved in %ums.", timeMs);
}
