#include "ComponentVideo.h"

#include "Application.h"
#include "GameObject.h"
#include "Modules/ModuleUserInterface.h"
#include "Modules/ModulePrograms.h"
#include "Modules/ModuleCamera.h"
#include "Modules/ModuleRender.h"
#include "Modules/ModuleTime.h"
#include "Components/ComponentAudioSource.h"
#include "Components/UI/ComponentTransform2D.h"
#include "Resources/ResourceVideo.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/Logging.h"

#include "Math/TransformOps.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include "Utils/Leaks.h"

#define JSON_TAG_VIDEOID "VideoId"
#define JSON_TAG_VIDEO_PLAY_ON_AWAKE "PlayOnAwake"
#define JSON_TAG_VIDEO_IS_LOOPING "Loop"
#define JSON_TAG_VIDEO_IS_FLIPPED "VerticalFlip"

char av_error[AV_ERROR_MAX_STRING_SIZE] = {0};
#define libav_err2str(errnum) av_make_error_string(av_error, AV_ERROR_MAX_STRING_SIZE, errnum)

ComponentVideo::~ComponentVideo() {
	CloseVideoReader();

	App->resources->DecreaseReferenceCount(videoID);

	// Release GL texture
	glDeleteTextures(1, &frameTexture);
}

void ComponentVideo::Init() {
	App->resources->IncreaseReferenceCount(videoID);

	// Load shader
	imageUIProgram = App->programs->imageUI;

	// Set GL texture buffer
	glGenTextures(1, &frameTexture);
	glBindTexture(GL_TEXTURE_2D, frameTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void ComponentVideo::Start() {
	Resource* videoResource = App->resources->GetResource<ResourceVideo>(videoID);
	if (videoResource == nullptr) return;

	OpenVideoReader(videoResource->GetResourceFilePath().c_str());
	if (playOnAwake) isPlaying = true;
}

void ComponentVideo::Update() {
	if (videoID != 0) {
		if (isPlaying) {
			elapsedVideoTime += App->time->GetRealTimeDeltaTime();
			if (elapsedVideoTime > videoFrameTime) {
				ReadVideoFrame();
			}
		}
		/* if (elapsedVideoTime > audioFrameTime) {
			ReadAudioFrame(); //each packet contains complete frames, or multiple frames in the case of audio.
		}*/
		// audioPlayer.UpdateStreamData(audioFrameData, audioPlayer.checkFramesSync());
	}
}

void ComponentVideo::OnEditorUpdate() {
	if (ImGui::Checkbox("Active", &active)) {
		if (GetOwner().IsActive()) {
			if (active) {
				Enable();
			} else {
				Disable();
			}
		}
	}
	ImGui::Separator();

	ImGui::ResourceSlot<ResourceVideo>(
		"Video Resource",
		&videoID,
		[this]() { RemoveVideoResource(); },
		[this]() {
			ResourceVideo* video = App->resources->GetResource<ResourceVideo>(videoID);
			if (video) OpenVideoReader(video->GetResourceFilePath().c_str());
		});

	std::string removeButton = std::string(ICON_FA_TIMES "##") + "video";
	if (ImGui::Button(removeButton.c_str())) {
		if (videoID != 0) {
			RemoveVideoResource();
			App->resources->DecreaseReferenceCount(videoID);
			videoID = 0;
		}
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("Remove Video");

	if (videoID != 0) {
		ResourceVideo* videoResource = App->resources->GetResource<ResourceVideo>(videoID);
		if (videoResource != nullptr) {
			ImGui::Separator();
			if (ImGui::Button("Play")) {
				Play();
			}
			ImGui::SameLine();
			if (ImGui::Button("Pause")) {
				Pause();
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop")) {
				Stop();
			}
			ImGui::Checkbox("Play on Awake", &playOnAwake);
			ImGui::Checkbox("Loop", &loopVideo);
			ImGui::Checkbox("Flip Vertically", &verticalFlip);
		}
	}
}

void ComponentVideo::Save(JsonValue jComponent) const {
	jComponent[JSON_TAG_VIDEOID] = videoID;
	jComponent[JSON_TAG_VIDEO_PLAY_ON_AWAKE] = playOnAwake;
	jComponent[JSON_TAG_VIDEO_IS_LOOPING] = loopVideo;
	jComponent[JSON_TAG_VIDEO_IS_FLIPPED] = verticalFlip;
}

void ComponentVideo::Load(JsonValue jComponent) {
	videoID = jComponent[JSON_TAG_VIDEOID];
	playOnAwake = jComponent[JSON_TAG_VIDEO_PLAY_ON_AWAKE];
	loopVideo = jComponent[JSON_TAG_VIDEO_IS_LOOPING];
	verticalFlip = jComponent[JSON_TAG_VIDEO_IS_FLIPPED];
}

void ComponentVideo::Draw(ComponentTransform2D* transform) {
	if (imageUIProgram == nullptr) return;

	glBindBuffer(GL_ARRAY_BUFFER, App->userInterface->GetQuadVBO());
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) ((sizeof(float) * 6 * 3)));
	glUseProgram(imageUIProgram->program);

	float4x4 modelMatrix = transform->GetGlobalScaledMatrix();
	float4x4& proj = App->camera->GetProjectionMatrix();
	float4x4& view = App->camera->GetViewMatrix();

	if (App->userInterface->IsUsing2D()) {
		proj = float4x4::D3DOrthoProjLH(-1, 1, App->renderer->GetViewportSize().x, App->renderer->GetViewportSize().y); //near plane. far plane, screen width, screen height
		view = float4x4::identity;
	}

	ComponentCanvasRenderer* canvasRenderer = GetOwner().GetComponent<ComponentCanvasRenderer>();
	if (canvasRenderer != nullptr) {
		float factor = canvasRenderer->GetCanvasScreenFactor();
		view = view * float4x4::Scale(factor, factor, factor);
	}

	glUniformMatrix4fv(imageUIProgram->viewLocation, 1, GL_TRUE, view.ptr());
	glUniformMatrix4fv(imageUIProgram->projLocation, 1, GL_TRUE, proj.ptr());
	glUniformMatrix4fv(imageUIProgram->modelLocation, 1, GL_TRUE, modelMatrix.ptr());

	glUniform2fv(imageUIProgram->offsetLocation, 1, float2::zero.ptr());
	glUniform2fv(imageUIProgram->tilingLocation, 1, float2::one.ptr());

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(imageUIProgram->diffuseLocation, 0);
	glUniform4fv(imageUIProgram->inputColorLocation, 1, float4::one.ptr());

	// allocate memory and set texture data
	glBindTexture(GL_TEXTURE_2D, frameTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, frameWidth, frameHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, frameData);

	glUniform1i(imageUIProgram->hasDiffuseLocation, 1);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ComponentVideo::Play() {
	isPlaying = true;
	hasVideoFinished = false;
}

void ComponentVideo::Pause() {
	isPlaying = false;
}

void ComponentVideo::Stop() {
	forceStop = true;
	RestartVideo();
	CleanFrameBuffer();
}

void ComponentVideo::SetVideoFrameSize(int width, int height) {
	GameObject* owner = &this->GetOwner();
	if (owner) {
		ComponentTransform2D* transform = owner->GetComponent<ComponentTransform2D>();
		if (transform) {
			transform->SetSize(float2((float) width, (float) height));
		}
	}
}

bool ComponentVideo::HasVideoFinished() {
	return hasVideoFinished;
}

void ComponentVideo::OpenVideoReader(const char* filename) {
	MSTimer timer;
	timer.Start();

	// Open video file
	formatCtx = avformat_alloc_context();
	if (!formatCtx) {
		LOG("Couldn't allocate AVFormatContext.");
		return;
	}
	if (avformat_open_input(&formatCtx, filename, nullptr, nullptr) != 0) {
		LOG("Couldn't open video file.");
		return;
	}

	// DECODING VIDEO
	// Find a valid video stream in the file
	AVCodecParameters* videoCodecParams;
	AVCodec* videoDecoder;
	videoStreamIndex = -1;

	videoStreamIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	if (videoStreamIndex < 0) {
		LOG("Couldn't find valid video stream inside file.");
		return;
	}

	// Find an appropiate video decoder
	videoCodecParams = formatCtx->streams[videoStreamIndex]->codecpar;
	videoDecoder = avcodec_find_decoder(videoCodecParams->codec_id);
	if (!videoDecoder) {
		LOG("Couldn't find valid video decoder.");
		return;
	}

	// Set up a video codec context for the decoder
	videoCodecCtx = avcodec_alloc_context3(videoDecoder);
	if (!videoCodecCtx) {
		LOG("Couldn't allocate AVCodecContext.");
		return;
	}
	if (avcodec_parameters_to_context(videoCodecCtx, videoCodecParams) < 0) {
		LOG("Couldn't initialise AVCodecContext.");
		return;
	}
	if (avcodec_open2(videoCodecCtx, videoDecoder, nullptr) < 0) {
		LOG("Couldn't open video codec.");
		return;
	}

	// Set video parameters and Allocate frame buffer
	frameWidth = videoCodecParams->width;
	frameHeight = videoCodecParams->height;
	timeBase = formatCtx->streams[videoStreamIndex]->time_base;
	frameData = new uint8_t[frameWidth * frameHeight * 4];
	SetVideoFrameSize(frameWidth, frameHeight);
	CleanFrameBuffer();

	// DECODING AUDIO
	// Find a valid audio stream in the file
	AVCodecParameters* audioCodecParams;
	AVCodec* audioDecoder;
	audioStreamIndex = -1;

	audioStreamIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (audioStreamIndex < 0) {
		LOG("Couldn't find valid audio stream inside file.");
		return;
	}

	audioCodecParams = formatCtx->streams[audioStreamIndex]->codecpar;
	audioDecoder = avcodec_find_decoder(audioCodecParams->codec_id);
	if (!audioDecoder) {
		LOG("Couldn't find valid audio decoder.");
		return;
	}

	// Set up a audio codec context for the decoder
	audioCodecCtx = avcodec_alloc_context3(audioDecoder);
	if (!audioCodecCtx) {
		LOG("Couldn't allocate AVCodecContext.");
		return;
	}
	if (avcodec_parameters_to_context(audioCodecCtx, audioCodecParams) < 0) {
		LOG("Couldn't initialise AVCodecContext.");
		return;
	}
	if (avcodec_open2(audioCodecCtx, audioDecoder, nullptr) < 0) {
		LOG("Couldn't open video codec.");
		return;
	}

	// Allocate memory for packets and frames
	avPacket = av_packet_alloc();
	if (!avPacket) {
		LOG("Couldn't allocate AVPacket.");
		return;
	}
	avFrame = av_frame_alloc();
	if (!avFrame) {
		LOG("Couldn't allocate AVFrame.");
		return;
	}

	// Set audio parameters
	/*
	wanted_spec.freq = aCodecCtx->sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = aCodecCtx->channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
	wanted_spec.callback = audio_callback;
	wanted_spec.userdata = aCodecCtx;
	*/

	unsigned timeMs = timer.Stop();
	LOG("Video initialised in %ums", timeMs);
}

void ComponentVideo::ReadVideoFrame() {
	int response = -1;
	int error = 0;
	while (error >= 0) {
		error = av_read_frame(formatCtx, avPacket);

		if (avPacket->stream_index != videoStreamIndex) {
			av_packet_unref(avPacket);
			continue;
		}

		//SEEK to frame 0 -> Restart the video timestamp
		if (error == AVERROR_EOF) {
			RestartVideo();
			av_packet_unref(avPacket);
			break;
		}

		response = avcodec_send_packet(videoCodecCtx, avPacket);
		if (response < 0) {
			LOG("Failed to decode packet: %s.", libav_err2str(response));
			return;
		}

		response = avcodec_receive_frame(videoCodecCtx, avFrame);
		if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
			av_packet_unref(avPacket);
			continue;
		}
		if (response < 0) {
			LOG("Failed to decode frame: %s.", libav_err2str(response));
			return;
		}

		av_packet_unref(avPacket);
		break;
	}

	videoFrameTime = avFrame->pts * timeBase.num / (float) timeBase.den;
	if (videoFrameTime == 0) elapsedVideoTime = 0;
	if (!scalerCtx) {
		// Set SwScaler - Scale frame size + Pixel converter to RGB
		scalerCtx = sws_getContext(frameWidth, frameHeight, videoCodecCtx->pix_fmt, frameWidth, frameHeight, AV_PIX_FMT_RGB0, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

		if (!scalerCtx) {
			LOG("Couldn't initialise SwScaler.");
			return;
		}
	}

	// Transform pixel format to RGB and send the data to the framebuffer
	if (!verticalFlip) { // We flip the image by default. To have an inverted image, don't do the flipping
		avFrame->data[0] += avFrame->linesize[0] * (videoCodecCtx->height - 1);
		avFrame->linesize[0] *= -1;
		avFrame->data[1] += avFrame->linesize[1] * (videoCodecCtx->height / 2 - 1);
		avFrame->linesize[1] *= -1;
		avFrame->data[2] += avFrame->linesize[2] * (videoCodecCtx->height / 2 - 1);
		avFrame->linesize[2] *= -1;
	}
	uint8_t* dest[4] = {frameData, nullptr, nullptr, nullptr};
	int linSize[4] = {frameWidth * 4, 0, 0, 0};
	sws_scale(scalerCtx, avFrame->data, avFrame->linesize, 0, frameHeight, dest, linSize);
}

void ComponentVideo::ReadAudioFrame() {
	int response = -1;
	while (av_read_frame(formatCtx, avPacket) >= 0) {
		if (avPacket->stream_index != audioStreamIndex) {
			av_packet_unref(avPacket);
			continue;
		}

		response = avcodec_send_packet(audioCodecCtx, avPacket);
		if (response < 0) {
			LOG("Failed to decode packet: %s.", libav_err2str(response));
			return;
		}

		response = avcodec_receive_frame(audioCodecCtx, avFrame);
		if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
			av_packet_unref(avPacket);
			continue;
		}
		if (response < 0) {
			LOG("Failed to decode frame: %s.", libav_err2str(response));
			return;
		}

		av_packet_unref(avPacket);
		break;
	}

	audioFrameTime = avFrame->pts * timeBase.num / (float) timeBase.den;
	// TODO: Do stuff with audio
}

void ComponentVideo::RestartVideo() {
	avio_seek(formatCtx->pb, 0, SEEK_SET);
	if (av_seek_frame(formatCtx, videoStreamIndex, -1, 0) >= 0) {
		if (!loopVideo || forceStop) {
			isPlaying = false;
			hasVideoFinished = true;
		}
		forceStop = false;
	}
}

void ComponentVideo::CloseVideoReader() {
	// Close libAV context -  free allocated memory
	sws_freeContext(scalerCtx);
	scalerCtx = nullptr;
	avformat_close_input(&formatCtx);
	avformat_free_context(formatCtx);
	avcodec_free_context(&videoCodecCtx);
	avcodec_free_context(&audioCodecCtx);
	av_frame_free(&avFrame);
	av_packet_free(&avPacket);

	// Release frame data buffer
	RELEASE(frameData);
}

void ComponentVideo::RemoveVideoResource() {
	// Reset external members
	videoStreamIndex = -1;
	frameWidth = 0;
	frameHeight = 0;
	videoFrameTime = 0;

	audioStreamIndex = -1;
	audioFrameTime = 0;

	// Clean libAV space
	CloseVideoReader();
}

void ComponentVideo::CleanFrameBuffer() {
	memset(frameData, 0, frameWidth * frameHeight * 4);
}
