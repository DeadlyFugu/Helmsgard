#include "common.h"
//#include <SDL_mixer.h>

// ===== [[ Local Types ]] =====

// ===== [[ Declarations ]] =====

// ===== [[ Static Data ]] =====

// ===== [[ Implementations ]] =====

void Title_enter(void) {
    Music_play(Music_find("brave"));
}

void Title_leave(void) {
}

void Title_update(void) {
    if (Input_isReleased(InputButton_accept)) {
        Sound_play(Sound_find("titleok"));
        Main_stateChange(MainState_menu);
    }
    // todo: temporary hack, to be removed
    if (Input_isReleased(InputButton_attack)) {
        Main_stateChange(MainState_editor);
    }
    if (Input_isReleased(InputButton_back)) exit(0);
}

void Title_render(void) {
    // draw title text
    Draw_setColor(Color_gray);
    Draw_textAligned(
        SCREEN_WIDTH / 2 + 2, SCREEN_HEIGHT / 2 + 2,
        0.5f, 0.5f, "Tales of a Starchild - Helmsgard"
    );
    Draw_setColor(Color_white);
    Draw_textAligned(
        SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
        0.5f, 0.5f, "Tales of a Starchild - Helmsgard"
    );

    // draw bottom text
    Draw_setColor(Color_gray);
    Draw_textAligned(
        SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10,
        1.0f, 1.0f, FAERJOLD_VERSION
    );
    Draw_textAligned(
        SCREEN_WIDTH / 2, SCREEN_HEIGHT - 64,
        0.5f, 0.5f, "Press Enter"
    );
}
