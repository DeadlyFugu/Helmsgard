#include "common.h"

// ===== [[ Defines ]] =====

#ifdef NDEBUG
#define IS_DEBUG 0
#else
#define IS_DEBUG 1
#endif

// ===== [[ Declarations ]] =====

static bool _getBoolean(const char* section, const char* name, bool def);
static int _getInt(const char* section, const char* name, int def);
static float _getFloat(const char* section, const char* name, float def);
static void _getString(const char* section, const char* name,
        char* dst, int size);
static int _getEnum(const char* section, const char* name,
        const char* opts, int def);

// ===== [[ Static Data ]] =====

int Config_displayMode;
char Config_assetDirectory[256];
bool Config_muteMusic;
bool Config_muteSounds;
bool Config_disableAudio;
bool Config_showPerf;
bool Config_showCollision;
bool Config_showNavGrid;
float Config_deadzoneX;
float Config_deadzoneY;
int Config_logLevel;

// ===== [[ Implementations ]] =====

void Config_load(void) {
    Ini_readFile("config.ini");
    _getString("Assets", "assetDirectory", Config_assetDirectory, 256);
    Config_displayMode = _getInt("Display", "displayMode", 1);
    Config_muteMusic = _getBoolean("Audio", "muteMusic", false);
    Config_muteSounds = _getBoolean("Audio", "muteSounds", false);
    Config_disableAudio = _getBoolean("Audio", "disableAudio", false);
    Config_showPerf = _getBoolean("Debug", "showPerf", IS_DEBUG);
    Config_showCollision = _getBoolean("Debug", "showCollision", false);
    Config_showNavGrid = _getBoolean("Debug", "showNavGrid", false);
    Config_deadzoneX = _getFloat("Input", "deadzoneX", 0.05f);
    Config_deadzoneY = _getFloat("Input", "deadzoneY", 0.05f);
    Config_logLevel = _getEnum("Debug", "logLevel",
            "error;warn;info;debug", 2);
    Ini_clear();
}

static bool _getBoolean(const char* section, const char* name, bool def) {
    const char* value = Ini_get(section, name);
    if (!value) return def;
    if (!value[0]) return def;

    return String_parseBool(value, def);
}

static int _getInt(const char* section, const char* name, int def) {
    const char* value = Ini_get(section, name);
    if (!value) return def;
    if (!value[0]) return def;

    return atoi(value);
}

static float _getFloat(const char* section, const char* name, float def) {
    const char* value = Ini_get(section, name);
    if (!value) return def;
    if (!value[0]) return def;

    return atof(value);
}

static void _getString(const char* section, const char* name,
        char* dst, int size) {
    const char* value = Ini_get(section, name);
    if (!value) {
        dst[0] = '\0';
        return;
    }

    strncpy(dst, value, size);
}

static int _getEnum(const char* section, const char* name,
        const char* opts, int def) {
    const char* value = Ini_get(section, name);
    if (!value) return def;

    char buf[256];
    strncpy(buf, opts, 256);

    int i = 0;
    char* token = strtok(buf, ";");
    while (token) {
        if (strcmp(value, token) == 0) return i;
        i++;
        token = strtok(NULL, ";");
    }

    return def;
}
