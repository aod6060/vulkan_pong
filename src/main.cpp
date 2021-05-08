#include "pong.h"



int main(int argc, char** argv) {

	pong::AiPlayerType type = pong::AiPlayerType::AI_PLAYER_EASY;

	if (argc == 2) {
		std::string cmd(argv[1]);

		if (cmd == "easy") {
			type = pong::AiPlayerType::AI_PLAYER_EASY;
		}
		else if (cmd == "normal") {
			type = pong::AiPlayerType::AI_PLAYER_NORMAL;
		}
		else if (cmd == "hard") {
			type = pong::AiPlayerType::AI_PLAYER_HARD;
		}
		else if (cmd == "expert") {
			type = pong::AiPlayerType::AI_PLAYER_EXPERT;
		}
		else if (cmd == "impossible") {
			type = pong::AiPlayerType::AI_PLAYER_IMPOSSIBLE;
		}
	}

	app::Config config;
	pong::setup(&config, type);

	app::init(&config);
	app::update();
	app::release();

	return 0;
}