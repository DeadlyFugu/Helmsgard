#include "common.h"
#include <SDL_mixer.h>

// ===== [[ Defines ]] =====

#define SOURCE_LENGTH 256
#define MAX_MUSIC 64
#define MAX_SOUNDS 64

// ===== [[ Local Types ]] =====

typedef struct {
    char name[NAME_LENGTH];
    Mix_Music* mixMusic;
} Music;

typedef struct {
    char name[NAME_LENGTH];
    Mix_Chunk* mixChunk;
} Sound;

// ===== [[ Declarations ]] =====

// ===== [[ Static Data ]] =====

static Music _music[MAX_MUSIC];
static Sound _sounds[MAX_SOUNDS];

static int _musicCount;
static int _soundCount;

// ===== [[ Implementations ]] =====

void Audio_startup() {
    if (Config_disableAudio) {
        Log_info("Audio is disabled");
        return;
    }
    if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 512) < 0) {
        Log_error("failed to init audio: %s", Mix_GetError());
        return;
    }
    if (Mix_AllocateChannels(16) < 0) {
        Log_error("failed to init audio: %s", Mix_GetError());
        return;
    }
}

void Music_loadFrom(const char* assetpath) {
    Ini_readAsset(assetpath);

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_musicCount == MAX_MUSIC) {
            Log_error("Max music exceeded");
            Ini_clear();
            return;
        }

        Music* music = &_music[_musicCount++];
        strncpy(music->name, name, NAME_LENGTH);
        
        const char* source = Ini_get(name, "source");
        if (!source) {
            Log_warn("Missing source for %s", name);
            continue;
        }

        if (Config_disableAudio) continue;

        char filepath[256];
        snprintf(filepath, 256, "assets/music/%s", source);
        music->mixMusic = Mix_LoadMUS(filepath);
        if (!music->mixMusic) Log_warn("Failed to load %s", filepath);
    }

    Ini_clear();
}

MusicID Music_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _musicCount; i++) {
        if (strcmp(_music[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find music %s", name);
    return -1;
}

void Music_play(MusicID self) {
    if (Config_muteMusic) return;
    if (self < 0 || self > _musicCount) return;
    Music* music = &_music[self];
    if (!music->mixMusic) return;

    Mix_PlayMusic(music->mixMusic, -1);
}

void Music_stop(void) {
    Mix_HaltMusic();
}

void Sound_loadFrom(const char* assetpath) {
    Ini_readAsset(assetpath);

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_soundCount == MAX_SOUNDS) {
            Log_error("Max sounds exceeded");
            Ini_clear();
            return;
        }

        Sound* sound = &_sounds[_soundCount++];
        strncpy(sound->name, name, NAME_LENGTH);
        
        const char* source = Ini_get(name, "source");
        if (!source) {
            Log_warn("Missing source for %s", name);
            continue;
        }

        if (Config_disableAudio) continue;

        char filepath[256];
        snprintf(filepath, 256, "assets/sounds/%s", source);
        sound->mixChunk = Mix_LoadWAV(filepath);
        if (!sound->mixChunk) {
            Archive arc = Archive_open("assets/sounds.arc");
            if (arc) {
                size_t size = Archive_getSize(arc, source);
                uint8_t *data = malloc(size);
                Archive_read(arc, source, size, data);
                sound->mixChunk = Mix_LoadWAV_RW(SDL_RWFromConstMem(data, (int) size), 1);
                Archive_close(arc);
                free(data);
            }
        }
        if (!sound->mixChunk) Log_warn("Failed to load %s", filepath);
    }

    Ini_clear();
}

SoundID Sound_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _soundCount; i++) {
        if (strcmp(_sounds[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find sound %s", name);
    return -1;
}

void Sound_play(SoundID self) {
    if (Config_muteSounds) return;
    if (self < 0 || self > _soundCount) return;
    Sound* sound = &_sounds[self];
    if (!sound->mixChunk) return;

    Mix_PlayChannel(-1, sound->mixChunk, 0);
}
