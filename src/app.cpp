#include "sys.h"

static app::Config* g_config = nullptr;
static SDL_Window* g_window = nullptr;
static bool g_running = true;

void app::init(Config* config) {
	g_config = config;

	SDL_Init(SDL_INIT_EVERYTHING);

	g_window = SDL_CreateWindow(
		g_config->caption.c_str(),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		g_config->width,
		g_config->height,
		SDL_WINDOW_SHOWN);

	//input::init();

	if (g_config->initCB) {
		g_config->initCB();
	}
}

void app::update() {
	SDL_Event e;

	uint32_t pre = SDL_GetTicks();
	uint32_t curr = 0;
	float delta = 0.0f;

	while (g_running) {

		curr = SDL_GetTicks();
		delta = (curr - pre) / 1000.0f;
		pre = curr;

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				g_running = false;
			}

			//input::doEvent(e);

			if (g_config->doEventCB) {
				g_config->doEventCB(e);
			}
		}

		if (g_config->updateCB) {
			g_config->updateCB(delta);
		}

		if (g_config->renderCB) {
			g_config->renderCB();
		}

		//input::update();
		if (g_config->postUpdate) {
			g_config->postUpdate();
		}
	}
}

void app::release() {
	if (g_config->releaseCB) {
		g_config->releaseCB();
	}

	SDL_DestroyWindow(g_window);
	SDL_Quit();
}

uint32_t app::getWidth() {
	return g_config->width;
}

uint32_t app::getHeight() {
	return g_config->height;
}

SDL_Window* app::getWindow() {
	return g_window;
}

void app::exit() {
	g_running = false;
}