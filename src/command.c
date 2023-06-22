#include "common.h"

// ===== [[ Local Types ]] =====

#define MAX_VALUE 128

typedef struct {
    bool valid;
    char name[NAME_LENGTH];
    char value[MAX_VALUE];
} CommandVar;

// ===== [[ Declarations ]] =====

#define MAX_VARIABLES 256

static const char* _nextToken(void);
static CommandVar* _findVariable(const char* name);
static const char* _getvar(const char* name);
static bool _setvar(const char* name, const char* value);
static bool _appvar(const char* name, const char* value);
static void _delvar(const char* name);

static void _commandSet(void);
static void _commandGet(void);
static void _commandDel(void);
static void _commandReload(void);
static void _commandGod(void);
static void _commandNoclip(void);
static void _commandSethp(void);
static void _commandGive(void);
static void _commandEdit(void);
static void _commandStatusFX(void);
static void _commandQuest(void);
static void _commandHelp(void);

// ===== [[ Static Data ]] =====

static char _command[512];
static CommandVar _variables[MAX_VARIABLES];

// ===== [[ Implementations ]] =====

void Command_execute(const char* command) {
    strncpy(_command, command, 512);

    const char* operation = strtok(_command, " ");
    if (strcmp(operation, "set") == 0) _commandSet();
    else if (strcmp(operation, "get") == 0) _commandGet();
    else if (strcmp(operation, "del") == 0) _commandDel();
    else if (strcmp(operation, "reload") == 0) _commandReload();
    else if (strcmp(operation, "god") == 0) _commandGod();
    else if (strcmp(operation, "noclip") == 0) _commandNoclip();
    else if (strcmp(operation, "sethp") == 0) _commandSethp();
    else if (strcmp(operation, "give") == 0) _commandGive();
    else if (strcmp(operation, "edit") == 0) _commandEdit();
    else if (strcmp(operation, "statusfx") == 0) _commandStatusFX();
    else if (strcmp(operation ,"quest") == 0) _commandQuest();
    else if (strcmp(operation, "help") == 0) _commandHelp();
    else Log_error("unknown command %s", operation);
}

static const char* _nextToken(void) {
    return strtok(NULL, " ");
}

static CommandVar* _findVariable(const char* name) {
    for (int i = 0; i < MAX_VARIABLES; i++) {
        if (_variables[i].valid &&
                strncmp(_variables[i].name, name, NAME_LENGTH) == 0) {
            return &_variables[i];
        }
    }
    return NULL;
}

static const char* _getvar(const char* name) {
    CommandVar* var = _findVariable(name);
    if (var) return var->value;
    else return NULL;
}

static bool _setvar(const char* name, const char* value) {
    CommandVar* var = _findVariable(name);
    if (var == NULL) {
        for (int i = 0; i < MAX_VARIABLES; i++) {
            if (!_variables[i].valid) {
                _variables[i].valid = true;
                strncpy(_variables[i].name, name, NAME_LENGTH);
                var = &_variables[i];
                break;
            }
        }
    }

    if (!var) return false;

    strncpy(var->value, value, MAX_VALUE);
    return true;
}

static bool _appvar(const char* name, const char* value) {
    CommandVar* var = _findVariable(name);
    if (!var) return _setvar(name, value);

    int len = strlen(var->value);
    strncat(var->value, value, MAX_VALUE - len);
    return true;
}

static void _delvar(const char* name) {
    CommandVar* var = _findVariable(name);
    if (var) var->valid = false;
}

static void _commandSet(void) {
    const char* name = _nextToken();
    if (!name) goto format_error;

    const char* value = _nextToken();
    if (!value) goto format_error;

    if (!_setvar(name, value)) goto resource_error;

    const char* next;
    while ((next = _nextToken())) {
        _appvar(name, " ");
        _appvar(name, next);
    }

    return;

    format_error:
    Log_error("usage: set <name> <value>");
    return;

    resource_error:
    Log_error("too many variables set");
    return;
}

static void _commandGet(void) {
    const char* name = _nextToken();
    if (!name) goto format_error;

    do {
        const char* value = _getvar(name);
        if (value) {
            Log_debug("%s = %s", name, value);
        } else {
            Log_debug("%s undefined", name);
        }
    } while ((name = _nextToken()));
    return;

    format_error:
    Log_error("usage: get <names...>");
    return;
}

static void _commandDel(void) {
    const char* name = _nextToken();
    if (!name) goto format_error;

    do {
        _delvar(name);
    } while ((name = _nextToken()));
    return;

    format_error:
    Log_error("usage: del <names...>");
    return;
}

static void _commandReload(void) {
    Game_reload();
}

static void _commandGod(void) {
    int enable = 2; // 0: off, 1: on, 2: toggle
    const char* value = _nextToken();
    if (value && strcmp(value, "on") == 0) enable = 1;
    else if (value && strcmp(value, "off") == 0) enable = 0;
    else if (value) goto format_error;

    int entID = Entity_getPlayer();
    if (entID == -1) return;
    Entity_setGodMode(entID, enable);

    return;

    format_error:
    Log_error("usage: god [on|off]");
    return;
}

static void _commandNoclip(void) {
    int enable = 2; // 0: off, 1: on, 2: toggle
    const char* value = _nextToken();
    if (value && strcmp(value, "on") == 0) enable = 1;
    else if (value && strcmp(value, "off") == 0) enable = 0;
    else if (value) goto format_error;
    
    int entID = Entity_getPlayer();
    if (entID == -1) return;
    Entity_setNoclipMode(entID, enable);

    return;

    format_error:
    Log_error("usage: noclip [on|off]");
    return;
}

static void _commandSethp(void) {
    const char* value = _nextToken();
    if (!value) goto format_error;
    if (_nextToken()) goto format_error;

    int entID = Entity_getPlayer();
    if (entID == -1) return;
    Entity_setHP(entID, atoi(value));
    
    return;

    format_error:
    Log_error("usage: sethp <amount>");
    return;
}

static void _commandGive(void) {
    const char* itemName = _nextToken();
    if (!itemName) goto format_error;
    int quantity = String_parseInt(_nextToken(), 1);
    if (_nextToken()) goto format_error;

    ItemID itemID = Item_find(itemName);
    if (itemID == -1) return;

    if (!Inventory_addAll(itemID, quantity)) {
        Log_warn("inventory full");
    }

    return;

    format_error:
    Log_error("usage: give <item> [amount]");
    return;
}

static void _commandEdit(void) {
    const char* region = _nextToken();
    if (!region) goto format_error;
    if (_nextToken()) goto format_error;

    int regionID = String_parseInt(region, -1);
    if (regionID < 0 || regionID > 99) goto value_error;

    Main_stateChange(MainState_editor);
    Editor_setRegion(regionID);
    return;

    format_error:
    Log_error("usage: edit [region]");
    return;

    value_error:
    Log_error("region must be between 0 and 99");
    return;
}

static void _commandStatusFX(void) {
    const char* seName = _nextToken();
    if (!seName) goto format_error;
    if (_nextToken()) goto format_error;

    StatusEffectID seID = StatusEffect_find(seName);
    if (seID == -1) return;

    int player = Entity_getPlayer();
    if (player == -1) return;

    Entity_addStatusEffect(player, seID);
    return;

    format_error:
    Log_error("usage: statusfx <name>");
    return;
}

static void _commandQuest(void) {
    const char* qName = _nextToken();
    if (!qName) goto format_error;
    if (_nextToken()) goto format_error;

    QuestID qid = Quest_find(qName);
    if (qid == -1) return;

    Quest_begin(qid);
    return;

    format_error:
    Log_error("usage: quest <name>");
    return;
}

static void _commandHelp(void) {
    if (_nextToken()) goto format_error;

    Log_info("reload                 - reloads the current region");
    Log_info("god [on|off]           - toggles god mode");
    Log_info("noclip [on|off]        - toggles noclip");
    Log_info("sethp <amount>         - sets player hp");
    Log_info("give <item> [quantity] - give player an item");
    Log_info("help                   - lists commands");
    
    return;

    format_error:
    Log_error("usage: help");
    return;
}
