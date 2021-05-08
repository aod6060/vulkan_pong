#include "sys.h"

namespace audio {

	std::map<std::string, Mix_Chunk*> soundFX;

	void init() {
		Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 512);
	}

	void release() {
		for (std::map<std::string, Mix_Chunk*>::iterator it = soundFX.begin(); it != soundFX.end(); it++) {
			Mix_FreeChunk(it->second);
		}
		soundFX.clear();

		Mix_CloseAudio();
	}

	void createSoundFX(std::string name, std::string path) {
		soundFX[name] = Mix_LoadWAV(path.c_str());
	}

	void playSoundFX(std::string name) {
		Mix_PlayChannel(-1, soundFX[name], 0);
	}

}