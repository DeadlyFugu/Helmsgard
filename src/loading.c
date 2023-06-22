#include "common.h"
//#include <SDL_mixer.h>

// ===== [[ Local Types ]] =====

typedef void (*LoadingFn)(void);

// ===== [[ Declarations ]] =====

static void _loadMisc(void);
static void _loadSpriteImage(const char* name);

// ===== [[ Static Data ]] =====

static SDL_Texture* _texLoading;
static SDL_Texture* _texLoading2;

static LoadingFn _loadFns[] = {
    Audio_startup,
    Game_init,
    _loadMisc
};

static int _step;

// ===== [[ Implementations ]] =====

void Loading_update(void) {
    if (_step >= countof(_loadFns)) {
        Main_stateChange(MainState_menu);
        return;
    }

    LoadingFn fn = _loadFns[_step];
    fn();

    _step++;
}

void Loading_render(void) {
    SDL_Rect dst = {640 - 64 - 16, 480 - 64 - 16, 64, 64};
    SDL_RenderCopyEx(Main_renderer, _texLoading, NULL, &dst, SDL_GetTicks()*0.2, NULL, SDL_FLIP_NONE);
    SDL_Rect dst2 = {640 - 64 - 32 - 128, 480 - 64, 128, 32};
    SDL_RenderCopy(Main_renderer, _texLoading2, NULL, &dst2);

    //FC_Draw(_fntMedium, Main_renderer, 10, 10, "Now Loading");
    //FC_Draw(_fntDetail, Main_renderer, 10, 10+24+4, "Now Loading");
    //FC_Draw(_fntJapanese, Main_renderer, 640 - 64 - 32 - 128, 480 - 64 + 4, "読み込み中");
    //FC_Draw(_fntJapanese, Main_renderer, 640 - 64 - 32 - 128, 480 - 64 + 4, "Now Loading");

    //FC_Draw(_fntJapanese, Main_renderer, (640/2) - 24*3, (480-24) / 2, "フェアヨルド");

    //Draw_text(640 - 64 - 32 - 128, 480 - 64 + 4, "読み込み中");

    //float progress = SDL_GetTicks()*0.0004;
    float progress = _step / (float) countof(_loadFns);

    progress = fmodf(progress, 1);
    SDL_SetRenderDrawColor(Main_renderer, 255, 255, 255 ,255);
    SDL_Rect progressOutline = {10, 480 - 10 - 8 - 4, 128+4, 8 + 4};
    SDL_RenderDrawRect(Main_renderer, &progressOutline);
    SDL_Rect progressBar = {10+2, 480 - 10 - 8 - 2, 128*progress, 8};
    SDL_RenderFillRect(Main_renderer, &progressBar);
    SDL_SetRenderDrawColor(Main_renderer, 0, 0, 0 ,255);
}

void Loading_enter(void) {
    // load loadscreen assets immediately
    _texLoading = Loading_loadTexture("assets/images/loading.bmp");
    _texLoading2 = Loading_loadTexture("assets/images/loading2.bmp");
    
    // set initial configuration
    _step = 0;
}

void Loading_leave(void) {
    SDL_DestroyTexture(_texLoading);
    SDL_DestroyTexture(_texLoading2);
}

SDL_Texture* Loading_loadTexture(const char* name) {
    SDL_Surface* surface = SDL_LoadBMP(name);
    if (surface == NULL) {
//        Log_debug("searching archive for %s", name);
        Archive arc = Archive_open("assets/images.arc");
        if (arc) {
            const char* filename = strrchr(name, '/')+1;
            size_t size = Archive_getSize(arc, filename);
            uint8_t *data = malloc(size);
            Archive_read(arc, filename, size, data);
            surface = SDL_LoadBMP_RW(SDL_RWFromConstMem(data, (int) size), 1);
            Archive_close(arc);
            free(data);
        }
    }
    if (!surface) {
        Log_error("file not found: '%s'", name);
        abort();
    }

    SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0, 0, 0));

    SDL_Texture* tex = SDL_CreateTextureFromSurface(Main_renderer, surface);
    SDLAssert(tex);

    SDL_FreeSurface(surface);

    return tex;
}

void Loading_loadSpriteImage(const char* name) {
    char filepath[256];
    snprintf(filepath, 256, "assets/images/%s", name);
//	Log_debug("### name '%s' fpath '%s'", name, filepath);
    SDL_Texture* result = Loading_loadTexture(filepath);
    Sprite_setSpriteImage(name, result);
}

static void _loadMisc(void) {
    Sprite_loadFrom("sprites.ini");
    Animation_loadFrom("animations.ini");
    BFont_load();
    Music_loadFrom("music.ini");
    Sound_loadFrom("sounds.ini");
    Particles_load();
    Attack_loadFrom("attacks.ini");
    StatusEffect_load();
    ItemType_loadFrom("itemtypes.ini");
    Item_loadFrom("items.ini");
    LootTable_init();
    Recipe_load();
    Entity_loadPrefabsFrom("prefabs.ini");
    Villager_load();
    Quest_load();
    Dialog_load();
}
