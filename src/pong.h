#pragma once


#include "sys.h"


namespace pong {
	enum AiPlayerType {
		AI_PLAYER_EASY = 0,
		AI_PLAYER_NORMAL,
		AI_PLAYER_HARD,
		AI_PLAYER_EXPERT,
		AI_PLAYER_IMPOSSIBLE
	};

	void init();

	void doEvent(SDL_Event& e);

	void update(float delta);

	void render();

	void release();

	void setup(app::Config* conf, AiPlayerType type);

}