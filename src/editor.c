#include "common.h"

// ===== [[ Local Types ]] =====

// ===== [[ Declarations ]] =====

static void _saveRegion(void);
static void _loadRegion(void);

// ===== [[ Static Data ]] =====

static bool _isOpen;
static bool _isUnsaved;
static int _regionID;
static bool _isConsoleOpen;

// ===== [[ Implementations ]] =====

void Editor_enter(void) {
    Music_play(Music_find("brave"));
    _isConsoleOpen = false;
}

void Editor_leave(void) {
    Log_debug("LEAVE");
    _saveRegion();
    _isOpen = false;
}

void Editor_update(void) {
    if (_isConsoleOpen) {
        Console_update();
        if (Console_shouldClose()) {
            _isConsoleOpen = false;
            Console_leave();
        }
        return;
    }

    if (Input_isReleased(InputButton_back)) {
        Main_stateChange(MainState_title);
    }
    if (Input_isReleased(InputButton_hotbar)) {
        _isConsoleOpen = true;
        Console_enter();
    }
}

void Editor_render(void) {
    if (_isOpen) {
        const char* saveMsg = _isUnsaved ? " (unsaved)" : "";
        Draw_text(10, 10, "editor: %d%s", _regionID, saveMsg);
    } else {
        Draw_text(10, 10, "editor: <no region>");
    }

    if (_isConsoleOpen) {
        Console_render();
    }
}

void Editor_setRegion(int regionID) {
    if (_isUnsaved) {
        _saveRegion();
    }
    _regionID = regionID;
    _loadRegion();
}

static void _saveRegion(void) {
    if (!_isUnsaved) return;
    Log_debug("SAVING...");
    // todo ...
    _isUnsaved = false;
}

static void _loadRegion(void) {
    _saveRegion();
    Log_debug("LOADING...");
    // todo ...
    _isUnsaved = false;
    _isOpen = true;
}

// todo: remove editor?
