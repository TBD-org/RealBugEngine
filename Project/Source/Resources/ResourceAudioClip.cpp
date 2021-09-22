#include "ResourceAudioClip.h"

#include "Globals.h"
#include "Application.h"
#include "Modules/ModuleAudio.h"
#include "Utils/MSTimer.h"
#include "Utils/Logging.h"

#include "AL/alext.h"
#include <inttypes.h>
#include <sndfile.h>

#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include "Utils/Leaks.h"

void ResourceAudioClip::Load() {
	MSTimer timer;
	timer.Start();
	std::string filePath = GetResourceFilePath();
	LOG("Loading audio from path: \"%s\".", filePath.c_str());

	SNDFILE* sndfile;
	sf_count_t numFrames;

	// Open Audio File
	sndfile = sf_open(filePath.c_str(), SFM_READ, &sfInfo);
	if (!sndfile) {
		LOG("Could not open audio in %s: %s", filePath.c_str(), sf_strerror(sndfile));
		return;
	}
	DEFER {
		sf_close(sndfile);
	};

	if (sfInfo.frames < 1 || sfInfo.frames > (sf_count_t)(INT_MAX / sizeof(short)) / sfInfo.channels) {
		LOG("Bad sample count in %s (%" PRId64 ")", filePath, sfInfo.frames);
		return;
	}

	format = AL_NONE;
	if (sfInfo.channels == 1) {
		format = AL_FORMAT_MONO16;
	} else if (sfInfo.channels == 2) {
		format = AL_FORMAT_STEREO16;
	}
	if (!format) {
		LOG("Unsupported channel count: %d", sfInfo.channels);
		return;
	}

	// Decode the whole audio file to a buffer
	audioData = static_cast<short*>(malloc((size_t)(sfInfo.frames * sfInfo.channels) * sizeof(short)));
	numFrames = sf_readf_short(sndfile, audioData, sfInfo.frames);
	if (numFrames < 1) {
		LOG("Failed to read samples in %s (%" PRId64 ")", filePath, numFrames);
		return;
	}
	size = (ALsizei)(numFrames * sfInfo.channels) * (ALsizei) sizeof(short);

	validAudio = true;

	unsigned timeMs = timer.Stop();
	LOG("Audio loaded in %ums.", timeMs);
}

void ResourceAudioClip::FinishLoading() {
	if (!validAudio) return;

	// Create AL buffer
	alBuffer = 0;
	alGenBuffers(1, &alBuffer);
	alBufferData(alBuffer, format, audioData, size, sfInfo.samplerate);

	// Free data
	if (audioData != nullptr) {
		free(audioData);
		audioData = nullptr;
	}

	// Check if an error occured, and clean up if so.
	ALenum err = alGetError();
	if (err != AL_NO_ERROR) {
		LOG("OpenAL Error: %s", alGetString(err));
		if (alBuffer && alIsBuffer(alBuffer)) {
			alDeleteBuffers(1, &alBuffer);
		}
		return;
	}
};

void ResourceAudioClip::Unload() {
	if (audioData) {
		free(audioData);
		audioData = nullptr;
	}

	if (validAudio) {
		alDeleteBuffers(1, &alBuffer);
		alBuffer = 0;
	}
}
