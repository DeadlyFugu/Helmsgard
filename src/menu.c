#include "common.h"
//#include <SDL_mixer.h>

// ===== [[ Local Types ]] =====

typedef enum {
    MenuSubState_main,
    MenuSubState_options,
    MenuSubState_saveSelect,
    MenuSubState_confirmDelete
} MenuSubState;

// ===== [[ Declarations ]] =====

static void _updateMain(void);
static void _updateOptions(void);
static void _updateSaveSelect(void);

static void _renderMain(void);
static void _renderOptions(void);
static void _renderSaveSelect(void);

static void _enterMain(void);
static void _enterOptions(void);
static void _enterSaveSelect(void);

// ===== [[ Static Data ]] =====

static MenuSubState _subState;
static int _sel;
static int _xc;
static int _xt;

// ===== [[ Implementations ]] =====

void Menu_enter(void) {
    _subState = MenuSubState_main;
    _sel = 0;
    _xc = -100;
    _xt = 50;
}

void Menu_leave(void) {
}

void Menu_update(void) {
    switch (_subState) {
        case MenuSubState_main: _updateMain(); break;
        case MenuSubState_options: _updateOptions(); break;
        case MenuSubState_saveSelect: _updateSaveSelect(); break;
        default: break;
    }
}

void Menu_render(void) {
    switch (_subState) {
        case MenuSubState_main: _renderMain(); break;
        case MenuSubState_options: _renderOptions(); break;
        case MenuSubState_saveSelect: _renderSaveSelect(); break;
        default: break;
    }
}

static void _updateMain(void) {
    if (Input_isPressed(InputButton_up) && _sel > 0) {
        _sel--; 
        Sound_play(Sound_find("click3"));
    }
    if (Input_isPressed(InputButton_down) && _sel < 2) {
        _sel++;
        Sound_play(Sound_find("click3"));
    }

    if (Input_isReleased(InputButton_accept) ||
		Input_isKeyboard() && Input_isPressed(InputButton_interact)) {
        switch (_sel) {
            case 0:
                Main_stateChange(MainState_game);
                Sound_play(Sound_find("titleok")); break;
            case 1: _enterOptions(); break;
            case 2: exit(0); break;
            default: break;
        }
    }
    if (Input_isReleased(InputButton_back)) exit(0);
}

static void _updateOptions(void) {
    if (Input_isReleased(InputButton_back)) _enterMain();
}

static void _updateSaveSelect(void) {
    if (Input_isPressed(InputButton_up) && _sel > 0) {
        _sel--;
        Sound_play(Sound_find("click3"));
    }
    if (Input_isPressed(InputButton_down) && _sel < 2) {
        _sel++;
        Sound_play(Sound_find("click3"));
    }

    if (Input_isReleased(InputButton_accept) ||
		Input_isKeyboard() && Input_isPressed(InputButton_interact)) {
        Main_stateChange(MainState_game);
        Sound_play(Sound_find("titleok"));
    }
    if (Input_isReleased(InputButton_back)) _enterMain();
}

static void _renderMain(void) {
    SpriteID spr_btn = Sprite_find("menu/btn");
    SpriteID spr_btnActive = Sprite_find("menu/btnActive");
    SpriteID spr_hand = Sprite_find("menu/hand");
    SpriteID spr_bgtiles = Sprite_find("menu/bgtiles");
    SpriteID spr_title = Sprite_find("menu/title");

    int time = (SDL_GetTicks() / 20) % 32;
    for (int x = 0; x < 20; x++) {
        for (int y = 0; y < 10; y++) {
            Sprite_draw(spr_bgtiles, x*32 - time, y*32 - time);
        }
    }
    
    Sprite_draw(spr_title, (SCREEN_WIDTH-212)/2, 60);

    if (_xt - _xc < 2) _xc = _xt;
    else _xc += ceil((_xt - _xc) * 0.1f);
    int x = _xc;

    Sprite_draw(_sel==0?spr_btnActive:spr_btn, x, 140);
    Sprite_draw(_sel==1?spr_btnActive:spr_btn, x, 140+32);
    Sprite_draw(_sel==2?spr_btnActive:spr_btn, x, 140+64);
    Sprite_draw(spr_hand, x - 32, 140+32*_sel);

    Draw_setColor(Color_black);
    // Draw_text(100, 140, "  Start");
    // Draw_text(100, 140+32, "  Options");
    // Draw_text(100, 140+64, "  Exit");
    Draw_setColor(Color_white);

    Sprite_draw(Sprite_find("menu/txt_play"), x+21, 147);
    Sprite_draw(Sprite_find("menu/txt_credits"), x+13, 147+32);
    Sprite_draw(Sprite_find("menu/txt_quit"), x+23, 147+64);

    BFont_drawTextExt(BFont_find("dialog"),
        SCREEN_WIDTH/2-100, SCREEN_HEIGHT-24, 200, 0.5f, "GDS O-WEEK 2021 DEMO");
}

static void _renderOptions(void) {
    // Draw_text(100, 100, "Options");
    // Draw_text(100, 140, "not implemented...");

    SpriteID spr_bgtiles = Sprite_find("menu/bgtiles");
    SpriteID spr_title = Sprite_find("menu/title");

    int time = (SDL_GetTicks() / 20) % 32;
    for (int x = 0; x < 20; x++) {
        for (int y = 0; y < 10; y++) {
            Sprite_draw(spr_bgtiles, x*32 - time, y*32 - time);
        }
    }
    
    BFontID bf = BFont_find("dialog");

    BFont_drawTextExt(bf, 50, 50, SCREEN_WIDTH-100, 0,
        "Made by Matthew Turner. Copyright 2020/2021.\n"
        "Using SDL2, SDL FontCache, and cute tiled.\n"
        "-----\n"
        "Using assets from\n"
        "> RPG Dungeon Tileset   by Pita on itch.io.\n"
        "> RPG asset pack   by Moose Stache on itch.io.\n"
        "-----\n"
        "Thanks for playing!\n");
}

static void _renderSaveSelect(void) {
    Draw_text(100, 100, "Save Select");
    Draw_text(100, 140, "  Save A");
    Draw_text(100, 140+32, "  Save B");
    Draw_text(100, 140+64, "  Save C");
    Draw_text(100, 140+32*_sel, "*");
}

static void _enterMain(void) {
    _sel = 0;
    _subState = MenuSubState_main;
    _xc = -100;
    _xt = 50;
}

static void _enterOptions(void) {
    _sel = 0;
    _subState = MenuSubState_options;
}

static void _enterSaveSelect(void) {
    _sel = 0;
    _subState = MenuSubState_saveSelect;
}
