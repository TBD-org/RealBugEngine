#include "AudioImporter.h"

#include "Application.h"
#include "Utils/Logging.h"
#include "Utils/Buffer.h"
#include "Utils/FileDialog.h"
#include "Resources/ResourceAudioClip.h"
#include "Modules/ModuleResources.h"
#include "Modules/ModuleFiles.h"
#include "Modules/ModuleTime.h"
#include "Globals.h"
#include "ImporterCommon.h"

#include "Utils/Leaks.h"



bool AudioImporter::ImportAudio(const char* filePath, JsonValue jMeta) {
	LOG("Importing audio from path: \"%s\".", filePath);

	// Timer to measure importing audio
	MSTimer timer;
	timer.Start();

	//Convert Wavs to Ogg's
	std::string extension = FileDialog::GetFileExtension(filePath);

	if (extension == WAV_AUDIO_EXTENSION) {
		std::string fileIn(filePath);
		std::string fileOut = fileIn.replace(fileIn.end() - 3, fileIn.end(), "ogg");

		char* filePathOgg;
		filePathOgg = &fileOut[0];

		EncondeWavToOgg(filePath, filePathOgg);
	}

	// Read from file
	Buffer<char> buffer = App->files->Load(filePath);
	if (buffer.Size() == 0) {
		LOG("Error loading audio %s", filePath);
		return false;
	}

	// Create audio clip resource
	unsigned resourceIndex = 0;
	std::unique_ptr<ResourceAudioClip> audioClip = ImporterCommon::CreateResource<ResourceAudioClip>(FileDialog::GetFileName(filePath).c_str(), filePath, jMeta, resourceIndex);

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

	SNDFILE *infile, *outfile;
	SF_INFO sfinfo;
	int readcount;

	if (!(infile = sf_open(infilename, SFM_READ, &sfinfo))) {
		printf("Error : could not open file : %s\n", infilename);
		puts(sf_strerror(NULL));
		exit(1);
	}

	sfinfo.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;

	if (!sf_format_check(&sfinfo)) {
		sf_close(infile);
		printf("Invalid encoding\n");
		return;
	};

	if (!(outfile = sf_open(outfilename, SFM_WRITE, &sfinfo))) {
		printf("Error : could not open file : %s\n", outfilename);
		puts(sf_strerror(NULL));
		exit(1);
	};
	
	while ((readcount = sf_read_double(infile, data, BUFFER_LEN))) {
		sf_write_double(outfile, data, readcount);
	}

	sf_close(infile);
	sf_close(outfile);

	return;
}