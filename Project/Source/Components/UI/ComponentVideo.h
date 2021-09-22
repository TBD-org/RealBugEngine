#pragma once
#include "Components/Component.h"

extern "C" {
#include "libavutil/rational.h"
}
class ModuleTime;
class ComponentAudioSource;
class ComponentTransform2D;
struct ProgramImageUI;
struct AVFormatContext;
struct AVCodecContext;
struct SwsContext;
struct AVFrame;
struct AVPacket;

class ComponentVideo : public Component {
public:
	REGISTER_COMPONENT(ComponentVideo, ComponentType::VIDEO, false); // Refer to ComponentType for the Constructor

	~ComponentVideo(); // Destructor

	void Init() override;							// Inits the component
	void Start() override;							// Starts the component
	void Update() override;							// Update
	void OnEditorUpdate() override;					// Show component info in inspector
	void Save(JsonValue jComponent) const override; // Serializes object
	void Load(JsonValue jComponent) override;		// Deserializes object
	void Draw(ComponentTransform2D* transform);		// Draws the current frame of the loaded video as a texture in a framebuffer, and plays the corresponding audio data through an internal audio source component.

	TESSERACT_ENGINE_API void Play();
	TESSERACT_ENGINE_API void Pause();
	TESSERACT_ENGINE_API void Stop();
	TESSERACT_ENGINE_API void SetVideoFrameSize(int width, int height); // Sets the ComponentTransform2D size to adjust to the video sizes.
	TESSERACT_ENGINE_API bool HasVideoFinished();						// Returns true if the video has finished and it is not playing anymore.

private:
	void OpenVideoReader(const char* filename); // Opens a video file and allocates the neccessary memory to work with it.
	void ReadVideoFrame();						// Reads the next video frame of the allocated video.
	void ReadAudioFrame();						// Reads the next audio frame of the allocated video.
	void RestartVideo();						// Resets the current frame to the 1st frame of the file.
	void CloseVideoReader();					// Frees the memory of the allocated video.
	void RemoveVideoResource();					// Reinitialises the video variables when changing the Video Resource loaded from inspector.
	void CleanFrameBuffer();					// Sets the framebuffer to all zeros (black screen).

private:
	UID videoID = 0;							 // Video file resource ID.
	ComponentAudioSource* audioPlayer = nullptr; // This AudioSource will store the audio stream from the video and send it to openAL.

	// Video Controllers
	bool isPlaying = false;		   // Control for the video Play/pause/Stop functionalities.
	bool forceStop = false;		   // Signal that indicates that the video decoding process must be stopped. Set to true on Stop().
	bool hasVideoFinished = false; // Signal that the video has finished and it is not playing anymore.

	// Video Options
	bool playOnAwake = false;	   // Signal to automatically Play() the video when starting the scene.
	bool loopVideo = false;		   // If true, the video will restart after decoding the last frame.
	bool verticalFlip = false;	   // Invert the Y axis of the rendered image.

	// LibAV internal state
	AVFormatContext* formatCtx = nullptr;	 // Video file context.
	AVCodecContext* videoCodecCtx = nullptr; // Video Decoder context.
	AVCodecContext* audioCodecCtx = nullptr; // Audio decoder context.
	AVPacket* avPacket = nullptr;			 // Data packet. This is sent to de decoders to obtain a frame of any type (video or audio).
	AVFrame* avFrame = nullptr;				 // Frame Data. This is what a decoder returns after decoding a packet.
	SwsContext* scalerCtx = nullptr;		 // Used for converting the frame data to RGB format.
	AVRational timeBase = {0, 0};			 // Used to obtain the FrameTime -> Used to sync video and audio.

	// LibAV external Video data
	int videoStreamIndex = -1;			 // Video data stream inside file.
	int frameWidth = 0, frameHeight = 0; // Size of video frame.
	uint8_t* frameData = nullptr;		 // Buffer for the frame texture data.
	float videoFrameTime = 0;			 // Time in seconds when the next audio frame must appear.

	// LibAV external Audio data
	int audioStreamIndex = -1; // Audio data stream inside file.
	float audioFrameTime = 0;  // Time in seconds when the next audio frame must appear.

	// Auxiliar members
	ProgramImageUI* imageUIProgram = nullptr; // Shader program.
	unsigned int frameTexture = 0;			  // GL texture ID that stores the frame as an image texture.
	float elapsedVideoTime = 0;				  // Elapsed time playing video. Used for framerate sync and video-audio sync.
};
