#include "common.h"

// ===== [[ Defines ]] =====

#define SOURCE_LENGTH 256
#define MAX_VILLAGERS 128

// ===== [[ Local Types ]] =====

typedef struct {
    char name[NAME_LENGTH];
    char title[NAME_LENGTH];
    char dialog[64];
} Villager;

// ===== [[ Declarations ]] =====

// ===== [[ Static Data ]] =====

static Villager _villagers[MAX_VILLAGERS];
static int _villagerCount;

// ===== [[ Implementations ]] =====

void Villager_load(void) {
    Ini_readAsset("villagers.ini");

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_villagerCount == MAX_VILLAGERS) {
            Log_error("Max villagers exceeded");
            Ini_clear();
            return;
        }

        Villager* villager = &_villagers[_villagerCount++];
        strncpy(villager->name, name, NAME_LENGTH);
        
        const char* title = Ini_get(name, "title");
        if (!title) title = "Charlie";
        strncpy(villager->title, title, 32);
        
        const char* dialog = Ini_get(name, "dialog");
        if (!dialog) dialog = "uwu whats this?";
        strncpy(villager->dialog, dialog, 64);
    }

    Ini_clear();
}

VillagerID Villager_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _villagerCount; i++) {
        if (strcmp(_villagers[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find villager %s", name);
    return -1;
}

const char* Villager_getDialog(VillagerID villagerID) {
    if (villagerID < 0 || villagerID > _villagerCount) return NULL;
    return _villagers[villagerID].dialog;
}

const char* Villager_getTitle(VillagerID villagerID) {
    if (villagerID < 0 || villagerID > _villagerCount) return NULL;
    return _villagers[villagerID].title;
}
