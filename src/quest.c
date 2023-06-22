#include "common.h"

// ===== [[ Defines ]] =====

#define MAX_QUESTS 64
#define MAX_QUEST_OBJS 64
#define MAX_DESC_LENGTH 256

// ===== [[ Local Types ]] =====

typedef enum {
    ObjKind_talk,
    ObjKind_defeat,
    ObjKind_collect,
    ObjKind_craft,
    ObjKind_defeatBoss
} ObjKind;

typedef struct {
    char name[NAME_LENGTH];
    char title[NAME_LENGTH];
    char desc[MAX_DESC_LENGTH];
    bool primary;
    int objectives[4];
    int objectiveCount;

    // dynamic state
    bool begun;
    bool completed;
    int nextObj;
    int progressInObj;
    int priority;
} Quest;

typedef struct {
    char name[NAME_LENGTH];
    ObjKind kind;
    int arg;
    int amount;
    char str[MAX_DESC_LENGTH];
} QuestObj;

// ===== [[ Declarations ]] =====

static int _findObjective(const char* name);

// ===== [[ Static Data ]] =====

static Quest _quests[MAX_QUESTS];
static QuestObj _questObjs[MAX_QUEST_OBJS];
static int _questCount;
static int _questObjCount;
static int _lastPriority;
static bool _slimeKingDefeated;

// ===== [[ Implementations ]] =====

void Quest_load(void) {
    Ini_readAsset("quest_objs.ini");

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_questObjCount == MAX_QUEST_OBJS) {
            Log_error("Max quest objectives exceeded");
            break;
        }

        QuestObj* questObj = &_questObjs[_questObjCount++];
        strncpy(questObj->name, name, NAME_LENGTH);

        questObj->kind = String_parseEnum(
            Ini_get(name, "kind"),
            "talk;defeat;collect;craft;defeat_boss",
            ObjKind_talk
        );

        if (questObj->kind == ObjKind_talk) {
            questObj->arg = Villager_find(Ini_get(name, "npc"));
        } else if (questObj->kind == ObjKind_defeat) {
            // ...
        } else if (questObj->kind == ObjKind_collect) {
            questObj->arg = Item_find(Ini_get(name, "item"));
        } else if (questObj->kind == ObjKind_craft) {
            questObj->arg = Recipe_find(Ini_get(name, "recipe"));
        }
        questObj->amount = String_parseInt(Ini_get(name, "amount"), 1);
    }

    Ini_clear();

    Ini_readAsset("quests.ini");

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_questCount == MAX_QUESTS) {
            Log_error("Max quests exceeded");
            break;
        }

        Quest* quest = &_quests[_questCount++];
        strncpy(quest->name, name, NAME_LENGTH);

        const char* title = Ini_get(name, "title");
        strncpy(quest->title, title ? title : "no title", NAME_LENGTH);
        const char* desc = Ini_get(name, "desc");
        strncpy(quest->desc, desc ? desc : "no desc", MAX_DESC_LENGTH);

        quest->primary = String_parseBool(Ini_get(name, "primary"), false);
        quest->objectiveCount = String_parseIntArrayExt(
            Ini_get(name, "objs"),
            quest->objectives, 4, _findObjective);
    }

    Ini_clear();
}

void Quest_reset(void) {
    for (int i = 0; i < _questCount; i++) {
        Quest* q = &_quests[i];
        q->begun = false;
        q->completed = false;
        q->nextObj = 0;
        q->progressInObj = 0;
    }
}

QuestID Quest_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _questCount; i++) {
        if (strcmp(_quests[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find quest %s", name);
    return -1;
}

void Quest_begin(QuestID quest) {
    if (quest < 0 || quest >= _questCount) return;
    Quest* q = &_quests[quest];
    q->begun = true;
    q->priority = ++_lastPriority;
    int o = q->objectives[0];
    if (o < 0 || o > _questObjCount) return;
    QuestObj* qo = &_questObjs[o];
    if (qo->kind == ObjKind_collect) {
        int slot = Inventory_find(qo->arg);
        int qty = Inventory_getQuantity(slot);
        for (int i = 0; i < qty; i++) {
            // todo: this is really hacky lol
            // if another quest is expecting this item type, it will also signal it
            Quest_signalEvent(QuestEvent_collect, qo->arg);
        }
    } else if (qo->kind == ObjKind_defeatBoss) {
        if (_slimeKingDefeated) q->nextObj++;
    }
}

const char* Quest_getTitle(QuestID quest) {
    if (quest < 0 || quest >= _questCount) return "<invalid>";
    return _quests[quest].title;
}

QuestID Quest_getActive(int slot) {
    // primary quests first for priority
    for (int i = 0; i < _questCount; i++) {
        Quest* quest = &_quests[i];
        if (quest->primary && quest->begun && !quest->completed) {
            if (slot-- == 0) return i;
        }
    }
    for (int i = 0; i < _questCount; i++) {
        Quest* quest = &_quests[i];
        if (!quest->primary && quest->begun && !quest->completed) {
            if (slot-- == 0) return i;
        }
    }
    return -1;
}

const char* Quest_getObjectiveStr(QuestID quest, int obj) {
    if (quest < 0 || quest >= _questCount) return NULL;
    Quest* q = &_quests[quest];
    if (obj < 0 || obj >= q->objectiveCount) return NULL;
    int objid = q->objectives[obj];
    if (objid == -1) return NULL;
    QuestObj* qo = &_questObjs[objid];
    bool isCurrent = q->nextObj == obj;
    bool isCompleted = q->nextObj > obj;
    if (qo->kind == ObjKind_talk) {
        snprintf(qo->str, MAX_DESC_LENGTH,
            "Talk to %s", Villager_getTitle(qo->arg));
    } else if (qo->kind == ObjKind_defeat) {
        int prog = 0;
        if (isCurrent) prog = q->progressInObj;
        if (isCompleted) prog = qo->amount;
        snprintf(qo->str, MAX_DESC_LENGTH,
            "Defeat %d slimes   %d/%d", qo->amount, prog, qo->amount);
    } else if (qo->kind == ObjKind_defeatBoss) {
        snprintf(qo->str, MAX_DESC_LENGTH,
            "Defeat the Slime King");
    } else if (qo->kind == ObjKind_collect) {
        int prog = 0;
        if (isCurrent) prog = q->progressInObj;
        if (isCompleted) prog = qo->amount;
        snprintf(qo->str, MAX_DESC_LENGTH,
            "Collect %d %s   %d/%d", qo->amount,
            Item_getDisplayName(qo->arg), prog, qo->amount);
    } else if (qo->kind == ObjKind_craft) {
        snprintf(qo->str, MAX_DESC_LENGTH,
            "Improve Ol' Woodsy");
    } else {
        snprintf(qo->str, MAX_DESC_LENGTH, "<unknown kind>");
    }
    return qo->str;
}

bool Quest_getObjectiveComplete(QuestID quest, int obj) {
    if (quest < 0 || quest >= _questCount) return NULL;
    Quest* q = &_quests[quest];
    return obj < q->nextObj;
}

void Quest_signalEvent(QuestEvent event, int arg) {
    ObjKind relevantKind = -1;
    switch (event) {
        case QuestEvent_talk: relevantKind = ObjKind_talk; break;
        case QuestEvent_craft: relevantKind = ObjKind_craft; break;
        case QuestEvent_defeat: relevantKind = ObjKind_defeat; break;
        case QuestEvent_collect: relevantKind = ObjKind_collect; break;
        case QuestEvent_defeatBoss: relevantKind = ObjKind_defeatBoss; break;
    }

    if (event == QuestEvent_defeatBoss) _slimeKingDefeated = true;

    for (int i = 0; i < _questCount; i++) {
        Quest* q = &_quests[i];
        if (q->begun && !q->completed) {
            int objid = q->objectives[q->nextObj];
            if (objid == -1) continue;
            QuestObj* qo = &_questObjs[objid];
            if (qo->kind != relevantKind) continue;
            bool makeProgress = false;
            switch (relevantKind) {
                case ObjKind_talk:
                if (arg == qo->arg) makeProgress = true; break;
                case ObjKind_craft:
                if (arg == qo->arg) makeProgress = true; break;
                case ObjKind_defeat:
                case ObjKind_defeatBoss:
                makeProgress = true; break;
                case ObjKind_collect:
                if (arg == qo->arg) makeProgress = true; break;
            }
            if (makeProgress) {
                q->priority = ++_lastPriority;
                q->progressInObj++;
                if (q->progressInObj == qo->amount) {
                    q->nextObj++;
                    q->progressInObj = 0;
                    if (q->nextObj == q->objectiveCount) {
                        q->completed = true;
                    }
                }
            }
        }
    }
}

bool Quest_isBegun(QuestID quest) {
    if (quest < 0 || quest >= _questCount) return false;
    return _quests[quest].begun;
}

bool Quest_isAwaitingTalk(QuestID quest, VillagerID villager) {
    if (quest < 0 || quest >= _questCount) return NULL;
    Quest* q = &_quests[quest];
    if (q->completed) return false;
    int objid = q->objectives[q->nextObj];
    if (objid == -1) return false;
    QuestObj* qo = &_questObjs[objid];
    return qo->kind == ObjKind_talk && qo->arg == villager;
}

bool Quest_isCompleted(QuestID quest) {
    if (quest < 0 || quest >= _questCount) return false;
    return _quests[quest].completed;
}

int Quest_getPriority(QuestID quest) {
    if (quest < 0 || quest >= _questCount) return false;
    return _quests[quest].priority;
}

void Quest_bump(QuestID quest) {
    if (quest < 0 || quest >= _questCount) return;
    _quests[quest].priority = ++_lastPriority;
}

static int _findObjective(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _questObjCount; i++) {
        if (strcmp(_questObjs[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find objective %s", name);
    return -1;
}
