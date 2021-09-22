#include "AudioImporter.h"

#include "Globals.h"
#include "Application.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleTime.h"
#include "Resources/ResourceAudioClip.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
#include "Utils/PathUtils.h"
#include "ImporterCommon.h"

#include <sndfile.h>

#include "Utils/Leaks.h"

bool AudioImporter::ImportAudio(const char* filePath, JsonValue jMeta) {
	LOG("Importing audio from path: \"%s\".", filePath);

	// Timer to measure importing audio
	MSTimer timer;
	timer.Start();

	// TODO: Fix Converter
	////Convert Wavs to Ogg's
	//std::string extension = FileDialog::GetFileExtension(filePath);

	//if (extension == WAV_AUDIO_EXTENSION) {
	//	std::string fileIn(filePath);
	//	std::string fileOut = fileIn.replace(fileIn.end() - 4, fileIn.end(), ".ogg");

	//	char* filePathOgg;
	//	filePathOgg = &fileOut[0];

	//	EncondeWavToOgg(filePath, filePathOgg);
	//}

	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error loading audio %s", filePath);
		return false;
	}

	// Create audio clip resource
	unsigned resourceIndex = 0;
	std::unique_ptr<ResourceAudioClip> audioClip = ImporterCommon::CreateResource<ResourceAudioClip>(PathUtils::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

	// Save resource meta file
	bool saved = ImporterCommon::SaveResourceMetaFile(audioClip.get());
	if (!saved) {
		LOG("Failed to save audio clip resource meta file.");
		return false;
	}

	// Save to file
	saved = App->files->Save(audioClip->GetResourceFilePath().c_str(), buffer);
	if (!saved) {
		LOG("Failed to save audio clip resource file.");
		return false;
	}

	// Send resource creation event
	App->resources->SendCreateResourceEvent(audioClip);

	unsigned timeMs = timer.Stop();
	LOG("Audio imported in %ums", timeMs);
	return true;
}

void AudioImporter::EncondeWavToOgg(const char* infilename, const char* outfilename) {
	static double data[BUFFER_LEN];

	SNDFILE *inFile, *outFile;
	SF_INFO sfInfo;
	int readCount;

	if (!(inFile = sf_open(infilename, SFM_READ, &sfInfo))) {
		printf("Error : could not open file : %s\n", infilename);
		puts(sf_strerror(NULL));
		exit(1);
	}

	sfInfo.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;

	if (!sf_format_check(&sfInfo)) {
		sf_close(inFile);
		printf("Invalid encoding\n");
		return;
	};

	if (!(outFile = sf_open(outfilename, SFM_WRITE, &sfInfo))) {
		printf("Error : could not open file : %s\n", outfilename);
		puts(sf_strerror(NULL));
		exit(1);
	};

	while ((readCount = (int) sf_read_double(inFile, data, BUFFER_LEN))) {
		sf_write_double(outFile, data, readCount);
	}

	sf_close(inFile);
	sf_close(outFile);

	return;
}