#include "common.h"

// ===== [[ Local Types ]] =====

typedef struct {
    InputButton button;
    int code; // scancode or gamepad button
} InputBinding;

// ===== [[ Declarations ]] =====

// ===== [[ Static Data ]] =====

static bool _currState[InputButton_COUNT];
static bool _prevState[InputButton_COUNT];
static float _duration[InputButton_COUNT];
static int _lastClock;
static InputSource _lastSource;
static SDL_GameController* _controller;
static int _controllerID;
static char* _textBuffer;
static int _textBufferSize;
static char _imeBuffer[64];

static InputBinding _keyboardBindings[] = {
    { InputButton_accept, SDL_SCANCODE_RETURN },
    { InputButton_back, SDL_SCANCODE_ESCAPE },
    { InputButton_interact, SDL_SCANCODE_Z },
    { InputButton_roll, SDL_SCANCODE_SPACE },
    { InputButton_inventory, SDL_SCANCODE_C },
    { InputButton_attack, SDL_SCANCODE_X },
    { InputButton_pause, SDL_SCANCODE_ESCAPE },
    { InputButton_journal, SDL_SCANCODE_TAB },
    //{ InputButton_skill1, SDL_SCANCODE_A },
    //{ InputButton_skill2, SDL_SCANCODE_S },
    //{ InputButton_skill3, SDL_SCANCODE_D },
    { InputButton_hotbar, SDL_SCANCODE_GRAVE },
    { InputButton_left, SDL_SCANCODE_LEFT },
    { InputButton_right, SDL_SCANCODE_RIGHT },
    { InputButton_up, SDL_SCANCODE_UP },
    { InputButton_down, SDL_SCANCODE_DOWN }
};

static InputBinding _controllerBindings[] = {
    { InputButton_accept, SDL_CONTROLLER_BUTTON_A },
    { InputButton_back, SDL_CONTROLLER_BUTTON_B },
    { InputButton_interact, SDL_CONTROLLER_BUTTON_B },
    { InputButton_roll, SDL_CONTROLLER_BUTTON_A },
    { InputButton_inventory, SDL_CONTROLLER_BUTTON_Y },
    { InputButton_attack, SDL_CONTROLLER_BUTTON_X },
    { InputButton_pause, SDL_CONTROLLER_BUTTON_START },
    { InputButton_journal, SDL_CONTROLLER_BUTTON_BACK },
    { InputButton_skill1, SDL_CONTROLLER_BUTTON_DPAD_LEFT },
    { InputButton_skill2, SDL_CONTROLLER_BUTTON_DPAD_UP },
    { InputButton_skill3, SDL_CONTROLLER_BUTTON_DPAD_RIGHT },
    { InputButton_hotbar, SDL_CONTROLLER_BUTTON_DPAD_DOWN },
    { InputButton_left, SDL_CONTROLLER_BUTTON_DPAD_LEFT },
    { InputButton_right, SDL_CONTROLLER_BUTTON_DPAD_RIGHT },
    { InputButton_up, SDL_CONTROLLER_BUTTON_DPAD_UP },
    { InputButton_down, SDL_CONTROLLER_BUTTON_DPAD_DOWN },
};

// ===== [[ Implementations ]] =====

void Input_update(void) {
    // set prev state to current state, wipe current state
    for (int i = 0; i < InputButton_COUNT; i++) {
        _prevState[i] = _currState[i];
        _currState[i] = false;
    }

    // check keyboard input
    const uint8_t* keyStates = SDL_GetKeyboardState(NULL);
    for (int i = 0; i < countof(_keyboardBindings); i++) {
        if (keyStates[_keyboardBindings[i].code]) {
            _currState[_keyboardBindings[i].button] = true;
            _lastSource = InputSource_keyboard;
        }
    }

    // check controller input
    if (_controller) {
        for (int i = 0; i < countof(_controllerBindings); i++) {
            int code = _controllerBindings[i].code;
            if (SDL_GameControllerGetButton(_controller, code)) {
                _currState[_controllerBindings[i].button] = true;
                _lastSource = InputSource_controller;
            }
        }
    }

    // update button durations
    int newClock = SDL_GetTicks();
    float delta = (newClock - _lastClock) / 1000.0f;
    for (int i = 0; i < InputButton_COUNT; i++) {
        if (_currState[i]) {
            if (_prevState[i]) {
                _duration[i] += delta;
            } else {
                _duration[i] = 0;
            }
        }
    }
    _lastClock = newClock;
}

void Input_processEvent(SDL_Event* event) {
    switch (event->type) {
        case SDL_CONTROLLERDEVICEADDED: {
            if (_controller) return;
            _controllerID = event->cdevice.which;
            _controller = SDL_GameControllerOpen(_controllerID);
            Log_info("Controller connected");
        } break;

        case SDL_CONTROLLERDEVICEREMOVED: {
            if (event->cdevice.which != _controllerID) return;
            SDL_GameControllerClose(_controller);
            _controller = NULL;
            Log_info("Controller disconnected");
        } break;

        case SDL_CONTROLLERBUTTONDOWN: {
            int id = event->cbutton.button;
        } break;

        case SDL_KEYDOWN: {
            if (!_textBuffer) break;
            SDL_Keycode sym = event->key.keysym.sym;
            if (sym == SDLK_BACKSPACE) {
                int len = strlen(_textBuffer);
                // trim any continuation bytes first
                while (len > 0 && (_textBuffer[len - 1] & 0xc0) == 0x80) {
                    _textBuffer[len - 1] = 0;
                    len = strlen(_textBuffer);
                }
                if (len > 0) _textBuffer[len - 1] = 0;
            } else if (sym == SDLK_c && SDL_GetModState() & KMOD_CTRL) {
                SDL_SetClipboardText(_textBuffer);
            } else if (sym == SDLK_v && SDL_GetModState() & KMOD_CTRL) {
                const char* append = SDL_GetClipboardText();
                int len = strlen(_textBuffer);
                strncpy(_textBuffer + len, append, _textBufferSize - len);
            }
        } break;

        case SDL_TEXTINPUT: {
            if (!_textBuffer) break;
            if (SDL_GetModState() & KMOD_CTRL) break;

            int len = strlen(_textBuffer);
            strncpy(_textBuffer + len, event->text.text, _textBufferSize - len);
        } break;

        case SDL_TEXTEDITING: {
            if (!_textBuffer) break;
            strncpy(_imeBuffer, event->edit.text, 64);
        } break;
    }
}

bool Input_isPressed(InputButton button) {
    return _currState[button] && !_prevState[button];
}

bool Input_isReleased(InputButton button) {
    return !_currState[button] && _prevState[button];
}

bool Input_isDown(InputButton button) {
    return _currState[button];
}

// how long the button has been down for in seconds
// if not down, previous press duration
float Input_getDuration(InputButton button) {
    return _duration[button];
}

void Input_clear(InputButton button) {
    _currState[button] = false;
    _prevState[button] = false;
    _duration[button] = 0;
}

void Input_clearAll(void) {
    for (int i = 0; i < InputButton_COUNT; i++) {
        Input_clear(i);
    }
}

float Input_getMoveX(void) {
    if (_lastSource == InputSource_keyboard) {
        if (Input_isDown(InputButton_left)) return -1;
        if (Input_isDown(InputButton_right)) return 1;
        return 0;
    } else {
        float raw = SDL_GameControllerGetAxis(_controller,
            SDL_CONTROLLER_AXIS_LEFTX) / 32768.f;
        bool negative = raw < 0;
        if (negative) raw = -raw;
        // todo: deadzone calculation should probably consider
        //       both axes at the same time to avoid 'snapping'
        float scaleFactor = 1 / (1 - Config_deadzoneX);
        float scaled = (raw - Config_deadzoneX) * scaleFactor;
        float clamped = Math_clampf(scaled, 0, 1);
        return negative ? -clamped : clamped;
    }
}

float Input_getMoveY(void) {
    if (_lastSource == InputSource_keyboard) {
        if (Input_isDown(InputButton_up)) return -1;
        if (Input_isDown(InputButton_down)) return 1;
        return 0;
    } else {
        float raw = SDL_GameControllerGetAxis(_controller,
            SDL_CONTROLLER_AXIS_LEFTY) / 32768.f;
        bool negative = raw < 0;
        if (negative) raw = -raw;
        float scaleFactor = 1 / (1 - Config_deadzoneY);
        float scaled = (raw - Config_deadzoneY) * scaleFactor;
        float clamped = Math_clampf(scaled, 0, 1);
        return negative ? -clamped : clamped;
    }
}

InputSource Input_getInputSource(void) {
    return _lastSource;
}

bool Input_isKeyboard(void) {
    return Input_getInputSource() == InputSource_keyboard;
}

bool Input_isController(void) {
    return Input_getInputSource() == InputSource_controller;
}

void Input_setTextInput(char* buffer, int size) {
    _textBuffer = buffer;
    _textBufferSize = size;

    if (buffer) SDL_StartTextInput();
    else SDL_StopTextInput();
}

void Input_setTextInputPos(int x, int y) {
    SDL_Rect region = { x, y - 16, 16, 16 };
    SDL_SetTextInputRect(&region);
}

const char* Input_getImeText(void) {
    return _imeBuffer;
}
