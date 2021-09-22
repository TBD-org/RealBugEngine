#pragma once

#if defined(TESSERACT_ENGINE_API)
/* do nothing. */
#elif defined(_MSC_VER)
#define TESSERACT_ENGINE_API __declspec(dllexport)
#endif

// Enums -----------
enum class UpdateStatus {
	CONTINUE,
	STOP,
	ERROR
};

// Constants -----------
#define DEGTORAD 0.01745329251f
#define RADTODEG 1.0f / DEGTORAD

// Files -----------
#define ASSETS_PATH "Assets"
#define LIBRARY_PATH "Library"
#define SKYBOX_PATH "Assets/Skybox"
#define TEXTURES_PATH "Assets/Textures"
#define SHADERS_PATH "Assets/Shaders"
#define SCENES_PATH "Assets/Scenes"
#define AUDIO_PATH "Assets/Audio"
#define MATERIALS_PATH "Assets/Materials"
#define NAVMESH_PATH "Assets/NavMesh"

#define JPG_TEXTURE_EXTENSION ".jpg"
#define PNG_TEXTURE_EXTENSION ".png"
#define TIF_TEXTURE_EXTENSION ".tif"
#define DDS_TEXTURE_EXTENSION ".dds"
#define TGA_TEXTURE_EXTENSION ".tga"

#define PREFABS_PATH "Assets/Prefabs"

#define MATERIAL_EXTENSION ".mat"
#define SCENE_EXTENSION ".scene"
#define PREFAB_EXTENSION ".prefab"
#define META_EXTENSION ".meta"

#define FRAGMENT_SHADER_EXTENSION ".frag"
#define VERTEX_SHADER_EXTENSION ".vert"
#define DEFAULT_SHADER_EXTENSION ".glsl"

#define MODEL_EXTENSION ".fbx"

#define SKYBOX_EXTENSION ".hdr"

#define NAVMESH_EXTENSION ".navmesh"

#define FONT_EXTENSION ".ttf"
#define SCRIPT_EXTENSION ".h"

#define WAV_AUDIO_EXTENSION ".wav"
#define OGG_AUDIO_EXTENSION ".ogg"

#define MP4_VIDEO_EXTENSION ".mp4"

#define STATE_MACHINE_EXTENSION ".stma"
#define ANIMATION_CLIP_EXTENSION ".clip"

#define TEMP_SCENE_FILE_NAME "_scene_autosave.temp"
#define CONFIGURATION_FILE_NAME "config.json"

// Configuration -----------
#define GLSL_VERSION "#version 460"
#define POINT_LIGHTS 32
#define SPOT_LIGHTS 8
#define CASCADE_FRUSTUMS 10

// Timers
#define TIME_BETWEEN_RESOURCE_IMPORT_UPDATES_MS 300
#define MAX_RESOURCE_LOADING_DURATION_PER_FRAME_MS 10
#define TIME_BETWEEN_AUTOSAVES_MS 300000 // 5 minutes

// Delete helpers -----------
#define RELEASE(x)          \
	{                       \
		if (x != nullptr) { \
			delete x;       \
			x = nullptr;    \
		}                   \
	}
#define RELEASE_ARRAY(x)    \
	{                       \
		if (x != nullptr) { \
			delete[] x;     \
			x = nullptr;    \
		}                   \
	}

// Defer -----------
// Defer calls the given code at the end of the scope. Useful for freeing resources.
// Important: End of scope is not end of function. Be careful when using it inside loops.
// Usage: DEFER{ statements; };
#ifndef DEFER
struct defer_dummy {};

template<class F>
struct deferrer {
	F f;
	~deferrer() {
		f();
	}
};

template<class F>
deferrer<F> operator*(defer_dummy, F f) {
	return {f};
}

#define DEFER__(LINE) zz_defer##LINE
#define DEFER_(LINE) DEFER__(LINE)
#define DEFER auto DEFER_(__LINE__) = defer_dummy {}* [&]()
#endif // DEFER
