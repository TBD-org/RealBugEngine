#pragma once

#include "Module.h"

class SDL_Window;
class SDL_Surface;

class ModuleWindow : public Module
{
public:
	ModuleWindow();
	virtual ~ModuleWindow();

	bool Init();
	bool CleanUp();

	void SetScreenMode(int mode);
	void SetResizable(bool resizable);
	void SetSize(int width, int height);
	void SetBrightness(float brightness);

public:
	SDL_Window* window = nullptr; // The window we'll be rendering to
	SDL_Surface* screen_surface = nullptr; // The surface contained by the window
};
