#include "ResourceAnimation.h"

#include "Application.h"
#include "Modules/ModuleFiles.h"
#include "Utils/Buffer.h"
#include "Utils/MSTimer.h"
#include "Utils/Logging.h"

#include "Utils/Leaks.h"

void ResourceAnimation::Load() {
	MSTimer timer;
	timer.Start();

	Buffer<char> buffer = App->files->Load(GetResourceFilePath().c_str());
	char* cursor = buffer.Data();

	duration = *((float*) cursor);
	cursor += sizeof(float);
	unsigned numKreyFrames = *((unsigned*) cursor);
	cursor += sizeof(unsigned);
	unsigned numChannels = *((unsigned*) cursor);
	cursor += sizeof(unsigned);

	keyFrames.resize(numKreyFrames);

	for (unsigned i = 0; i < numKreyFrames; ++i) {
		ResourceAnimation::KeyFrameChannels keyFrame;

		for (unsigned j = 0; j < numChannels; ++j) {
			ResourceAnimation::Channel channel;

			unsigned sizeName = *((unsigned*) cursor);
			cursor += sizeof(unsigned);


			Buffer<char> name(sizeof(char) * (sizeName + 1));
			memcpy_s(name.Data(), sizeof(char) * sizeName, cursor, sizeof(char) * sizeName);
			cursor += (FILENAME_MAX / 2) * sizeof(char);

			name[sizeName] = '\0';

			channel.translation.x = *((float*) cursor);
			cursor += sizeof(float);
			channel.translation.y = *((float*) cursor);
			cursor += sizeof(float);
			channel.translation.z = *((float*) cursor);
			cursor += sizeof(float);

			channel.rotation.x = *((float*) cursor);
			cursor += sizeof(float);
			channel.rotation.y = *((float*) cursor);
			cursor += sizeof(float);
			channel.rotation.z = *((float*) cursor);
			cursor += sizeof(float);
			channel.rotation.w = *((float*) cursor);
			cursor += sizeof(float);

			keyFrame.channels.emplace(name.Data(), channel);
		}

		keyFrames[i] = keyFrame;
	}

	unsigned timeMs = timer.Stop();
	LOG("Animation loaded in %ums", timeMs);
}

void ResourceAnimation::Unload() {
	keyFrames.clear();
}


