#include "common.h"

// ===== [[ Defines ]] =====

#define MAX_DIALOG_TRIGGERS 256
#define MAX_DIALOG_CONVOS 512
#define MAX_CANDIDATES 32
#define MESSAGE_LENGTH 128
#define MAX_CHOICES 4
#define CHOICE_LENGTH 16

// ===== [[ Local Types ]] =====

typedef struct {
    char name[NAME_LENGTH];
    DialogLineID entry;
    VillagerID npc;
    bool onlyOnce;
    int friendshipAbove;
    QuestID questNotBegun;
    QuestID questAwaiting;
    QuestID questCompleted;
    QuestID questInProgress;
    int priority;
    int displayIcon;

    // game state
    int timesTriggered;
} DialogTrigger;

typedef struct {
    char name[NAME_LENGTH];
    char message[MESSAGE_LENGTH];
    DialogLineID next[MAX_CHOICES];
    int choiceCount;
    char choices[MAX_CHOICES][CHOICE_LENGTH];
    ItemID give;
    ItemID remove;
    int give_qty;
    QuestID quest;
    QuestID bump;
    bool hideName;
    int crowns;
    bool endDemo;
    RecipeID unlock;
} DialogConvo;

// ===== [[ Declarations ]] =====

// ===== [[ Static Data ]] =====

static DialogTrigger _triggers[MAX_DIALOG_TRIGGERS];
static DialogConvo _convos[MAX_DIALOG_CONVOS];

static int _triggerCount;
static int _convoCount;

// ===== [[ Implementations ]] =====

void Dialog_load(void) {
    Ini_readAsset("dlgconvos.ini");

    // first pass to set up names
    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_convoCount == MAX_DIALOG_CONVOS) {
            Log_error("Max dialog convos exceeded");
            Ini_clear();
            return;
        }

        DialogConvo* convo = &_convos[_convoCount++];
        strncpy(convo->name, name, NAME_LENGTH);
    }

    // second pass, fill out structs
    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        // note: assume i matches up with correct array index
        DialogConvo* convo = &_convos[i];

        const char* message = Ini_get(name, "message");
        if (!message) message = "No message.";
        strncpy(convo->message, message, MESSAGE_LENGTH);
        convo->choiceCount = String_parseIntArrayExt(
            Ini_get(name, "next"), convo->next, MAX_CHOICES,
            Dialog_findLine);
        for (int i = convo->choiceCount; i < MAX_CHOICES; i++) {
            convo->next[i] = -1;
        }
        const char* choices = Ini_get(name, "choices");
        if (!choices) {
            if (convo->choiceCount > 1) {
                Log_warn("Multiple nexts but no choices specified");
            }
            convo->choiceCount = 0;
        } else {
            char buf[512];
            strncpy(buf, choices, 512);
            char* token = strtok(buf, ",");
            int i;
            for (i = 0; i < MAX_CHOICES && token; i++) {
                strncpy(convo->choices[i], token, CHOICE_LENGTH);
                token = strtok(NULL, ",");
            }
            if (i != convo->choiceCount || token) {
                Log_warn("Amount of choices does not match nexts");
                for (int j = i; j < MAX_CHOICES; j++) {
                    sprintf(convo->choices[j], "Choice %d", j);
                }
            }
        }
        convo->give = Item_find(Ini_get(name, "give"));
        convo->remove = Item_find(Ini_get(name, "remove"));
        convo->give_qty = String_parseInt(Ini_get(name, "quantity"), 1);
        convo->quest = Quest_find(Ini_get(name, "quest"));
        convo->bump = Quest_find(Ini_get(name, "bump"));
        convo->crowns = String_parseInt(Ini_get(name, "crowns"), 1);
        convo->hideName = String_parseBool(Ini_get(name, "hideName"), false);
        convo->endDemo = String_parseBool(Ini_get(name, "endDemo"), false);
        convo->unlock = Recipe_find(Ini_get(name, "unlock"));
    }

    Ini_clear();

    Ini_readAsset("dlgtrigger.ini");

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_triggerCount == MAX_DIALOG_TRIGGERS) {
            Log_error("Max dialog triggers exceeded");
            Ini_clear();
            return;
        }

        DialogTrigger* trigger = &_triggers[_triggerCount++];
        strncpy(trigger->name, name, NAME_LENGTH);

        trigger->entry = Dialog_findLine(Ini_get(name, "entry"));
        trigger->npc = Villager_find(Ini_get(name, "npc"));
        trigger->onlyOnce = String_parseBool(Ini_get(name, "onlyOnce"), false);
        trigger->friendshipAbove = String_parseInt(Ini_get(name, "friendshipAbove"), -1);
        trigger->questNotBegun = Quest_find(Ini_get(name, "questNotBegun"));
        trigger->questAwaiting = Quest_find(Ini_get(name, "questAwaiting"));
        trigger->questCompleted = Quest_find(Ini_get(name, "questCompleted"));
        trigger->questInProgress = Quest_find(Ini_get(name, "questInProgress"));
        trigger->priority = String_parseInt(Ini_get(name, "priority"), 0);
        trigger->displayIcon = String_parseInt(Ini_get(name, "displayIcon"), 0);
    }

    Ini_clear();
}

DialogID Dialog_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _triggerCount; i++) {
        if (strcmp(_triggers[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find trigger %s", name);
    return -1;
}

DialogLineID Dialog_findLine(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _convoCount; i++) {
        if (strcmp(_convos[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find convo %s", name);
    return -1;
}

DialogID Dialog_findCandidate(VillagerID villager) {
    DialogID candidates[MAX_CANDIDATES];
    int nextCandidate = 0;

    // find possible dialogs
    for (int i = 0; i < _triggerCount; i++) {
        DialogTrigger* trigger = &_triggers[i];
        if (trigger->npc != villager) continue;
        if (trigger->onlyOnce && trigger->timesTriggered > 0) continue;
        if (trigger->questNotBegun != -1 &&
            Quest_isBegun(trigger->questNotBegun)) continue;
        if (trigger->questAwaiting != -1 &&
            !Quest_isAwaitingTalk(trigger->questAwaiting, villager)) continue;
        if (trigger->questCompleted != -1 &&
            !Quest_isCompleted(trigger->questCompleted)) continue;
        if (trigger->questInProgress != -1 &&
            (!Quest_isBegun(trigger->questInProgress) ||
            Quest_isCompleted(trigger->questInProgress))) continue;

        if (nextCandidate == MAX_CANDIDATES) {
            Log_warn("Max dialog candidates exceeded");
            break;
        }

        candidates[nextCandidate++] = i;
    }

    // filter highest priority
    // todo: per-trigger priorities
    // todo: somehow prioritise dialog heard less times?
    int maxPriority = 0;
    for (int i = 0; i < nextCandidate; i++) {
        if (_triggers[candidates[i]].priority > maxPriority) {
            maxPriority = _triggers[candidates[i]].priority;
        }
    }
    if (maxPriority > 0) {
        for (int i = 0; i < nextCandidate; i++) {
            if (_triggers[candidates[i]].priority < maxPriority) {
                // erase from list
                candidates[i] = candidates[--nextCandidate];
                i--; // <- so we pass over replacement in i
            }
        }
    }

    // shuffle
    for (int i = 0; i < nextCandidate; i++) {
        // j is random value in range [0,nextCandidate)
        int j = i + (rand() % (nextCandidate - i));
        DialogID tmp = candidates[i];
        candidates[i] = candidates[j];
        candidates[j] = tmp;
    }

    // choose first valid
    for (int i = 0; i < nextCandidate; i++) {
        if (candidates[i] != -1) return candidates[i];
    }

    return -1;
}

DialogLineID Dialog_firstLine(DialogID dialog) {
    if (dialog < 0 || dialog > _triggerCount) return -1;
    _triggers[dialog].timesTriggered++;
    return _triggers[dialog].entry;
}

const char* Dialog_getMessage(DialogLineID dialogLine) {
    if (dialogLine < 0 || dialogLine > _convoCount) return "Invalid dialog line.";
    return _convos[dialogLine].message;
}

bool Dialog_getHideName(DialogLineID dialogLine) {
    if (dialogLine < 0 || dialogLine > _convoCount) return false;
    return _convos[dialogLine].hideName;
}

DialogLineID Dialog_getNext(DialogLineID dialogLine, int choice) {
    if (dialogLine < 0 || dialogLine > _convoCount) return -1;
    if (choice < 0 || choice > _convos[dialogLine].choiceCount) return -1;
    return _convos[dialogLine].next[choice];
}

int Dialog_getChoiceCount(DialogLineID dialogLine) {
    if (dialogLine < 0 || dialogLine > _convoCount) return 0;
    return _convos[dialogLine].choiceCount;
}

const char* Dialog_getChoice(DialogLineID dialogLine, int choice) {
    if (dialogLine < 0 || dialogLine > _convoCount) return "ye wot m7";
    if (dialogLine < 0 || dialogLine > _convoCount) return "ye wot m7";
    return _convos[dialogLine].choices[choice];
}

void Dialog_execute(DialogLineID dialogLine) {
    if (dialogLine < 0 || dialogLine > _convoCount) return;
    DialogConvo* convo = &_convos[dialogLine];
    if (convo->give != -1) {
        Inventory_addAll(convo->give, convo->give_qty);
    }
    if (convo->remove != -1) {
        int slot = Inventory_find(convo->remove);
        Inventory_remove(slot, convo->give_qty);
    }
    if (convo->quest != -1) {
        Quest_begin(convo->quest);
    }
    if (convo->bump != -1) {
        Quest_bump(convo->bump);
    }
    if (convo->unlock != -1) {
        Recipe_unlock(convo->unlock);
    }
    Game_addGold(convo->crowns);
    if (convo->endDemo) Game_setWon();
}

int Dialog_getDisplayIcon(DialogID dialog) {
    if (dialog < 0 || dialog > _triggerCount) return -1;
    return _triggers[dialog].displayIcon;
}
