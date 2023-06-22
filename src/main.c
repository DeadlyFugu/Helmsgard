#include <SDL.h>
//#include <SDL_mixer.h>
#include "SDL_video.h"
#include "common.h"

SDL_Window* Main_window;
SDL_Renderer* Main_renderer;

static bool _running;
static int _lastUpdateTime;
static int _lastDelta;
//static FC_Font* _fntDetail;
//static FC_Font* _fntMedium;
//static FC_Font* _fntJapanese;
// static SystemState* _state;
// static SystemState* _nextState;
static MainState _state2;
static MainState _nextState2;
static bool _stateLeaving;

static void _startup(void);
static void _shutdown(void);
static void _beginUpdate(void);

// Main entry point to the application
int main(int argc, char* argv[]) {
    Config_load();

    SDL_version ver;
    SDL_GetVersion(&ver);
    Log_info("Helmsgard %s on SDL %d.%d.%d", FAERJOLD_VERSION, ver.major, ver.minor, ver.patch);

    _startup();

    _nextState2 = MainState_loading;

    /*_fntDetail = FC_CreateFont();
    //FC_LoadFont(_fntDetail, Main_renderer, "assets/Nunito-Light.ttf", 16, FC_MakeColor(255,255,255,255), TTF_STYLE_NORMAL);
    FC_LoadFont(_fntDetail, Main_renderer, "assets/fonts/KosugiMaru-Regular.ttf", 12, FC_MakeColor(255,255,255,255), TTF_STYLE_NORMAL);

    _fntMedium = FC_CreateFont();
    FC_LoadFont(_fntMedium, Main_renderer, "assets/fonts/Nunito-Regular.ttf", 24, FC_MakeColor(255,255,255,255), TTF_STYLE_NORMAL);

    _fntJapanese = FC_CreateFont();
    FC_LoadFont(_fntJapanese, Main_renderer, "assets/fonts/KosugiMaru-Regular.ttf", 24, FC_MakeColor(255,255,255,255), TTF_STYLE_NORMAL);
	*/
    SDL_Texture* fbo = SDL_CreateTexture(Main_renderer, SDL_PIXELFORMAT_ARGB32, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);


    while (_running) {
        _beginUpdate();
        Input_update();

        Draw_setColor(Color_black);
        SDL_SetRenderTarget(Main_renderer, fbo);
        SDL_RenderClear(Main_renderer);
        Draw_setColor(Color_white);
        //Draw_setFont(_fntDetail);

        if (_nextState2 != MainState_invalid) {
            // Exit previous state
            switch (_state2) {
                case MainState_invalid: break;
                case MainState_loading: Loading_leave(); break;
                case MainState_title: Title_leave(); break;
                case MainState_menu: Menu_leave(); break;
                case MainState_game: Game_leave(); break;
                case MainState_editor: Editor_leave(); break;
            }

            // Enter new state
            switch (_nextState2) {
                case MainState_invalid: break;
                case MainState_loading: Loading_enter(); break;
                case MainState_title: Title_enter(); break;
                case MainState_menu: Menu_enter(); break;
                case MainState_game: Game_enter(); break;
                case MainState_editor: Editor_enter(); break;
            }

            _state2 = _nextState2;
            _nextState2 = MainState_invalid;
        }

        uint32_t timeBeginUpdate = SDL_GetTicks();
        switch (_state2) {
            case MainState_invalid: break;
            case MainState_loading: Loading_update(); break;
            case MainState_title: Title_update(); break;
            case MainState_menu: Menu_update(); break;
            case MainState_game: Game_update(); break;
            case MainState_editor: Editor_update(); break;
        }
        uint32_t timeBeginDraw = SDL_GetTicks();
        switch (_state2) {
            case MainState_invalid: break;
            case MainState_loading: Loading_render(); break;
            case MainState_title: Title_render(); break;
            case MainState_menu: Menu_render(); break;
            case MainState_game: Game_render(); break;
            case MainState_editor: Editor_render(); break;
        }
        uint32_t timeEndDraw = SDL_GetTicks();

        if (Config_showPerf) {
            const char* stateName = "(error)";
            switch (_state2) {
                case MainState_invalid: stateName = "(none)"; break;
                case MainState_loading: stateName = "Loading"; break;
                case MainState_title: stateName = "Title"; break;
                case MainState_menu: stateName = "Menu"; break;
                case MainState_game: stateName = "Game"; break;
                case MainState_editor: stateName = "Editor"; break;
            }
            /*FC_Draw(_fntDetail, Main_renderer, 10, 10,
                    "total  %2dms\nupdate  %2dms\nrender  %2dms\nstate %-8s",
                    _lastDelta, timeBeginDraw - timeBeginUpdate,
                    timeEndDraw - timeBeginDraw, stateName);*/
			BFont_drawText(
				BFont_find("dialog"), SCREEN_WIDTH - 60, 10,
				"total  %2dms\nupdate  %dms\nrender  %dms",
				_lastDelta, timeBeginDraw - timeBeginUpdate,
				timeEndDraw - timeBeginDraw
			);
        }

        SDL_SetRenderTarget(Main_renderer, NULL);
        SDL_RenderCopy(Main_renderer, fbo, NULL, NULL);
        SDL_RenderPresent(Main_renderer);
    }

    /*FC_FreeFont(_fntDetail);
    FC_FreeFont(_fntMedium);
    FC_FreeFont(_fntJapanese);*/

    _shutdown();
    return 0;
}

// Prints an error and exits if cond is false
void SDLAssert(bool cond) {
    if (!cond) {
        Log_error("(SDL) %s", SDL_GetError());
        abort();
    }
}

// Begin a state change (not immediate)
void Main_stateChange(MainState nextState) {
    assert(_nextState2 == MainState_invalid); // already changing state
    assert(nextState != MainState_invalid);
    if (nextState != _state2) _nextState2 = nextState;
}

// Initialize SDL
static void _startup(void) {
    if (SDL_Init(SDL_INIT_VIDEO |
            SDL_INIT_GAMECONTROLLER |
            SDL_INIT_AUDIO) < 0) {
        Log_error("(SDL) %s", SDL_GetError());
        exit(1);
    }

    /*if (TTF_Init() < 0) {
        Log_error("(SDL_ttf) %s\n", TTF_GetError());
        exit(1);
    }*/

    static int windowModeDims[6][2] = {
        { 426, 240 },
        { 853, 480 },
        { 1280, 720 },
        { 1706, 960 },
        { 1280, 720 }, // dummy size, actually fullscreen
        { 1920, 1080 } // dummy size, actually fullscreen
    };

    int dm = Config_displayMode;
    if (dm < 0 || dm > 5) {
        dm = 1;
    }
    
    Main_window = SDL_CreateWindow("Helmsgard",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            windowModeDims[dm][0], windowModeDims[dm][1], SDL_WINDOW_OPENGL);
    SDLAssert(Main_window);

    if (dm == 4) {
        SDL_SetWindowFullscreen(Main_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else if (dm == 5) {
        SDL_SetWindowFullscreen(Main_window, SDL_WINDOW_FULLSCREEN);
    }

    Main_renderer = SDL_CreateRenderer(
            Main_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDLAssert(Main_renderer);

    _running = true;
}

// Shutdown SDL
static void _shutdown(void) {
    SDL_DestroyRenderer(Main_renderer);
    SDL_DestroyWindow(Main_window);

    //TTF_Quit();
    SDL_Quit();
}

// Framerate limiter & event polling
static void _beginUpdate(void) {
    _lastDelta = SDL_GetTicks() - _lastUpdateTime;
    while (SDL_GetTicks() - _lastUpdateTime < 16) {
        SDL_Delay(1);
    }
    _lastUpdateTime = SDL_GetTicks();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
            _running = false;
            break;

            case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
                // Toggle stats with F3
                case SDLK_F3:
                Config_showPerf = !Config_showPerf;
                break;

                // Toggle fullscreen with F4
                case SDLK_F4:
                // TODO impl
                break;
            }
            break;
        }
        Input_processEvent(&event);
    }
}
