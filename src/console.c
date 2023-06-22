#include "common.h"

// ===== [[ Local Types ]] =====

// ===== [[ Declarations ]] =====

// ===== [[ Static Data ]] =====

static char _buffer[128];
static char _history[4][128];
static int _historyCount;
static int _historyLine;
static int _historyOffset;
static bool _shouldClose;

// ===== [[ Implementations ]] =====

void Console_enter(void) {
    _buffer[0] = 0;
    _historyLine = 0;
    Input_setTextInput(_buffer, 128);
}

void Console_leave(void) {
    Input_setTextInput(NULL, 0);
}

void Console_update(void) {
    _shouldClose = false;
    if (Input_isReleased(InputButton_back)) {
        _shouldClose = true;
    }
    if (Input_isPressed(InputButton_accept)) {
        if (_buffer[0]) {
            Command_execute(_buffer);
            if (_historyCount < 4) _historyCount++;
            _historyOffset = (_historyOffset + 1) % 4;
            memcpy(_history[_historyOffset], _buffer, 128);
        }
        _shouldClose = true;
    }
    if (Input_isPressed(InputButton_up)) {
        if (_historyLine < _historyCount) _historyLine++;
        int i = (_historyOffset - _historyLine + 5) % 4;
        memcpy(_buffer, _history[i], 128);
    }
    if (Input_isPressed(InputButton_down)) {
        if (_historyLine > 0) _historyLine--;
        if (_historyLine == 0) _buffer[0] = 0;
        else {
            int i = (_historyOffset - _historyLine + 5) % 4;
            memcpy(_buffer, _history[i], 128);
        }
    }
}

void Console_render(void) {
    Draw_setColor(Color_dkgray);
    Draw_rect(0, SCREEN_HEIGHT - 36, SCREEN_WIDTH, 100);

    int w = Draw_getTextWidth("%s", _buffer);
    int h = Draw_getTextHeight("%s", _buffer);
    int imeWidth = Draw_getTextWidth("%s", Input_getImeText());
    Input_setTextInputPos(10 + w, SCREEN_HEIGHT - 26 + h);
    if (imeWidth > 0) {
        Draw_setColor(Color_gray);
        Draw_rect(10 + w, SCREEN_HEIGHT - 26, imeWidth, 16);
    }
    
    Draw_setColor(Color_white);
    Draw_text(10, SCREEN_HEIGHT - 26, "%s%s", _buffer, Input_getImeText());
}

bool Console_shouldClose(void) {
    return _shouldClose;
}
