#pragma once

#include "Module.h"

#include "SDL_video.h"
#include <vector>

struct SDL_Surface;

enum class WindowMode {
	WINDOWED,
	BORDERLESS,
	FULLSCREEN,
	FULLSCREEN_DESKTOP
};

enum class RESOLUTION_PRESET {
	m_720x480 = 0,
	m_1280x720,
	m_1920x1080,
	MAX
};

class ModuleWindow : public Module {
public:
	// ------- Core Functions ------ //
	bool Init() override;
	bool CleanUp() override;

	// ---------- Setters ---------- //
	void SetWindowMode(WindowMode mode);
	void SetResizable(bool resizable);
	void SetCurrentDisplayMode(int index);
	void SetSize(int width, int height);
	void SetBrightness(float brightness);
	void SetTitle(const char* title);

	// ---------- Getters ---------- //
	WindowMode GetWindowMode() const;
	bool GetMaximized() const;
	bool GetResizable() const;
	int GetCurrentDisplayMode() const;
	int GetWidth() const;
	int GetHeight() const;
	int GetPositionX() const;
	int GetPositionY() const;
	float GetBrightness() const;
	const char* GetTitle() const;
	int GetResolutionPreset() const;
	void SetResolutionPreset(int resolutionPreset_);

public:
	SDL_Window* window = nullptr;
	SDL_Surface* surface = nullptr;

	std::vector<SDL_DisplayMode> displayModes;

private:
	int currentResolutionPreset = 2;

	WindowMode windowMode = WindowMode::WINDOWED;
	int currentDisplayMode = 0;
};
