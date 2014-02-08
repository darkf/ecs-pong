#pragma once
#include <SDL/SDL.h>

class Renderer {
public:
	unsigned int screenWidth, screenHeight;
	SDL_Surface *screen;
	Uint32 black, red;
	Uint16 mouseX, mouseY;
	Renderer(const Renderer&) = delete;
	Renderer(const unsigned int screenWidth, const unsigned int screenHeight)
		: screenWidth(screenWidth), screenHeight(screenHeight),
		  mouseX(0), mouseY(0)
		{
			if(SDL_Init(SDL_INIT_VIDEO) > 0)
				throw "Couldn't initialize SDL";
			screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, 0);
			if(screen == nullptr)
				throw "Couldn't initialize screen";

			black = SDL_MapRGB(screen->format, 0, 0, 0);
			red = SDL_MapRGB(screen->format, 255, 0, 0);
		}

	void drawRect(const int x, const int y, const unsigned int w, const unsigned int h, Uint32 color) {
		SDL_Rect rect {(Sint16)x, (Sint16)y, (Uint16) w, (Uint16) h};
		SDL_FillRect(screen, &rect, color);
	}

	bool pollEvents() {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) return false;
			else if(event.type == SDL_MOUSEMOTION) {
				mouseX = event.motion.x;
				mouseY = event.motion.y;
			}
		}
		return true;
	}

	void clear() {
		SDL_FillRect(screen, NULL, black);
	}

	void flip() {
		SDL_Flip(screen);
	}
};