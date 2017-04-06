#pragma once

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#include <SDL2/SDL.h>

struct SDL_Manager {
	SDL_Window* mainWindow;
	SDL_GLContext mainContext;
	struct size_t {
		int width;
		int height;
		float ratio;
	} size;
	SDL_Manager();
	~SDL_Manager();
	void refresh_size();
};