#include "common.h"

// ===== [[ Defines ]] ======

#define MAX_PREFABS 256
#define MAX_ATTACKS 256
#define MAX_STATUS_EFFECTS 64
#define MAX_PATHFINDING_LENGTH 16

// ===== [[ Local Types ]] =====

typedef struct {
    char name[NAME_LENGTH];
    int duration;
    int hitDelay;
    int weaponClass; // fist, sword, bow, staff, etc.
    bool friendlyFire;
    bool aoe; // if true, multiple targets damaged
    int range;
    int damage;
    float moveFactor; // how much can the player control movement?
    bool surround; // whether attack also affects others behind
    int knockback;
    int launch; // how much player moves forward
    SoundID sound;
    int next;
    int hitstop;
} Attack;

typedef struct {
    // general props
    char name[NAME_LENGTH];
    bool actor;
    bool solid;
    bool pushable;
    int weight;
    int moveSpeed;
    int cloneCount; // numbering for prefab copies
    int spawnXOffs, spawnYOffs;

    // controller
    EntityController controller;

    // location
    int radius;

    // combat
    int lvl;
    int xp;
    int faction; // should this be a table? ini can specify who can hurt who
    int maxhp; // base max hp
    int atk; // base atk stat
    int def; // base def stat
    int defaultAttack; // unequipped attack index
    int hurtDuration; // -1 for use anim

    // enemy ai
    int alertRadius;
    bool pathfinding;

    // interaction
    InteractionType interaction; // todo: ..Type -> InteractionKind?

    // throwing
    bool breakable; // is it easily breakable? (insta destroy on damage)
    int breakHits;
    bool throwable; // is it throwable (e.g. vase)
    int thrownAtk; // damage dealt on collision
    
    // rolling
    int rollSpeed;
    int rollDuration;

    // item drop (prefab only)
    bool collectible; // -> has CItemCollectible
    ItemID item;
    bool collectGold;

    // misc stuff
    LootTableID lootTable;
    SoundID soundDie;
    ParticlesID particlesDie;
    SpriteID harvestedSprite;
    bool tentacle;

    // mini inventory
    bool hasMiniInv;
    ItemID invitems[16];
    int invqty[16];
    bool dropInvOnDestroy;

    // hints
    char hintText[128];
    int hintRadius;

    // sprite
    int sprite;
    int spritex;
    int spritey;
    int spritez;
    int spriteBob;
    int bobDuration;

    // animation
    int animIdle;
    int animWalk[4];
    int animAttack[4];
    int animHurt[4];
    int animRoll[4];
    int animAlert[4];
} Prefab;

typedef struct {
    char name[NAME_LENGTH];
    Stat stat;
    int change;
    SpriteID icon;
} StatusEffect;

typedef struct {
    int launch; // knockback amount
    int launchAngle;
} CLaunch;

typedef struct {
    int movex;
    int movey;
} CMotion;

typedef struct {
    ItemID item;
    bool isGold;
} CItemCollectible;

typedef struct {
    int collector;
    int collectTime;
} CBeingCollected;

typedef struct {
    int sprite;
    int spritex;
    int spritey;
    int spritez;
    // CSpriteBob should probably be it's own component
    int spriteBob;
    int bobDuration;
    int bobTimer;
} CSprite;

typedef struct {
    int animIdle;
    int animWalk[4];
    int animAttack[4];
    int animHurt[4];
    int animRoll[4];
    int animAlert[4];
    int time;
    Facing facing;
    AnimState animState;
    AnimState prevAnimState;
} CAnimation;

typedef struct {
    VillagerID villager;
} CVillager;

typedef struct {
    StatusEffectID activeIDs[4];
    int durations[4];
} CStatusEffects;

typedef struct {
    InteractionType type;
} CInteraction;

typedef struct {
    int rollSpeed;
    int rollDuration;
    int hurtDuration; // -1 for use anim
} CStateParams;

typedef struct {
    int attackTime;
    int attackID;
    int attackQueued; // if not -1, will be done next
} CStateAttack;

typedef struct {
    int hurtTime;
    int hurtDuration; // copy from params
} CStateHurt;

typedef struct {
    int rollSpeed; // copy from params
    int rollDuration;
    int rollTime;
    int rollx;
    int rolly;
} CStateRolling;

typedef struct {
    int alertTime;
} CStateAlert;

typedef struct {
    int lvl;
    int xp;
    int faction; // should this be a table? ini can specify who can hurt who
    int maxhp; // base max hp
    int atk; // base atk stat
    int def; // base def stat
    int defaultAttack; // unequipped attack index
    // defaultSkills[3]
    int hp;
    int moveSpeed;
} CActor;

typedef struct {
    int dummy;
} CPlayerController;

typedef struct {
    int alertRadius;
    bool pathfinding;
    int pathTimer;
    int target; // -1 if none/unalert
} CEnemyController;

typedef struct {
    EntityIntent intent;
    float intentx;
    float intenty;
    int intentPartner; // for interact
    int kind; // for attack, 0: normal, 1-3: skill
} CIntent;

typedef struct {
    bool god;
    bool noclip;
} CCheats;

typedef struct {
    int breakHits;
} CBreakable;

typedef struct {
    int radius;
    int weight;
    bool pushable;
} CSolid;

typedef struct {
    int x;
    int y;
    int zoff; // bonus to z height (e.g. from throwing)
    int prevx;
    int prevy;
} CLocation;

typedef struct {
    char name[NAME_LENGTH];
} CDebugLabel;

typedef struct {
    int targetX;
    int targetY;
    int xs[MAX_PATHFINDING_LENGTH];
    int ys[MAX_PATHFINDING_LENGTH];
    int length;
    int progress;
    int speed;
    bool incomplete;
} CPathfinding;

typedef struct {
    LootTableID lootTable;
    SoundID soundDie;
    ParticlesID particlesDie;
    SpriteID harvestedSprite;
    bool tentacle;
} CMisc;

typedef struct {
    ItemID items[16];
    int quantity[16];
    bool dropInvOnDestroy;
} CMiniInventory;

typedef struct {
    int timer;
} CShake;

typedef struct {
    char text[128];
    int radius;
} CHint;

typedef struct {
    int timer;
    bool phase;
    bool active;
    int stun;
    int stun_tents;
} CBoss;

// ===== [[ Declarations ]] =====

static void _playerUpdate(int i);
static void _enemyUpdate(int i);
static void* _changeState(int i, EntityState newState);
static int _findInteractionPartner(int entityID);
static bool _isInvincible(int i);
static void _playerCollect(int playerID, int collectibleID);
static int _entDistSq(CLocation* a, CLocation* b);
static const char* _getDebugName(int i);

//  ecs stuff
// todo: use proper typedef for entity ids publicly
// todo: properly expose ECS functions, separate systems/components
//       into own files.
typedef int EcsComponent;
typedef int EcsEntity;
static EcsComponent _componentRegister(int width);
static EcsEntity _entityCreate(void);
static void _entityDestroy(EcsEntity e);
static bool _entityValid(EcsEntity e);
static void* _componentAttach(EcsEntity e, EcsComponent c);
static void _componentDetach(EcsEntity e, EcsComponent c);
static void* _componentGet(EcsEntity e, EcsComponent c);
static void* _componentGetOrAttach(EcsEntity e, EcsComponent c);
static void _queryAddColumn(EcsComponent c, void** s);
static void _queryAddId(EcsEntity* s);
static void _queryBegin(void);
static bool _queryNext(void);
static void _queryEnd(void);
static void _ecsTest(void);
void _ecsDump(EcsEntity e);

// ===== [[ Static Data ]] =====

Prefab Entity_prefabs[MAX_PREFABS];

// Entity Entity_table[MAX_ENTITIES];

static Attack _attacks[MAX_ATTACKS];

static int _prefabCount;
static int _attackCount;
static int _playerInteractionPartner;

static StatusEffect _statusEffects[MAX_STATUS_EFFECTS];
static int _statusEffectCount;

// ecs stuff
typedef struct {
    int width;
    void* pool;
    bool* valid;
    // EcsEntity* rowToEntity;
    // int* entityToRow;
} ComponentData;
typedef struct {
    EcsEntity* idStorage;
    EcsComponent columnTypes[4];
    void** columnStorage[4];
    int columnCount;
    EcsEntity* rows;
    int rowCount;
    int index;
} QueryData;
#define ECS_MAX_ENTITIES 256
#define ECS_MAX_COMPONENTS 64
#define ECS_MAX_QUERIES 8
static int _ecsEntityGeneration[ECS_MAX_ENTITIES];
static bool _ecsEntityValid[ECS_MAX_ENTITIES];
static ComponentData _ecsComponents[ECS_MAX_COMPONENTS];
static QueryData _ecsQueries[ECS_MAX_QUERIES];
static int _ecsNextComponent;
static int _ecsNextQuery;
#define QUERY_COMPONENT(type, name) type* name; _queryAddColumn(type##_id, (void**) &name)
#define QUERY_ID(name) EcsEntity name; _queryAddId(&name)

// ecs components
static EcsComponent CLaunch_id;
static EcsComponent CMotion_id;
static EcsComponent CItemCollectible_id;
static EcsComponent CBeingCollected_id;
static EcsComponent CSprite_id;
static EcsComponent CAnimation_id;
static EcsComponent CVillager_id;
static EcsComponent CStatusEffects_id;
static EcsComponent CInteraction_id;
static EcsComponent CStateParams_id;
static EcsComponent CStateAttack_id;
static EcsComponent CStateHurt_id;
static EcsComponent CStateRolling_id;
static EcsComponent CStateAlert_id;
static EcsComponent CActor_id;
static EcsComponent CPlayerController_id;
static EcsComponent CEnemyController_id;
static EcsComponent CIntent_id;
static EcsComponent CCheats_id;
static EcsComponent CBreakable_id;
static EcsComponent CSolid_id;
static EcsComponent CLocation_id;
static EcsComponent CDebugLabel_id;
static EcsComponent CPathfinding_id;
static EcsComponent CMisc_id;
static EcsComponent CMiniInventory_id;
static EcsComponent CShake_id;
static EcsComponent CHint_id;
static EcsComponent CBoss_id;

// ===== [[ Implementations ]] =====

void Entity_loadPrefabsFrom(const char* assetpath) {
//    _ecsTest();
    CLaunch_id = _componentRegister(sizeof(CLaunch));
    CMotion_id = _componentRegister(sizeof(CMotion));
    CItemCollectible_id = _componentRegister(sizeof(CItemCollectible));
    CBeingCollected_id = _componentRegister(sizeof(CBeingCollected));
    CSprite_id = _componentRegister(sizeof(CSprite));
    CAnimation_id = _componentRegister(sizeof(CAnimation));
    CVillager_id = _componentRegister(sizeof(CVillager));
    CStatusEffects_id = _componentRegister(sizeof(CStatusEffects));
    CInteraction_id = _componentRegister(sizeof(CInteraction));
    CStateParams_id = _componentRegister(sizeof(CStateParams));
    CStateAttack_id = _componentRegister(sizeof(CStateAttack));
    CStateHurt_id = _componentRegister(sizeof(CStateHurt));
    CStateRolling_id = _componentRegister(sizeof(CStateRolling));
    CStateAlert_id = _componentRegister(sizeof(CStateAlert));
    CActor_id = _componentRegister(sizeof(CActor));
    CPlayerController_id = _componentRegister(sizeof(CPlayerController));
    CEnemyController_id = _componentRegister(sizeof(CEnemyController));
    CIntent_id = _componentRegister(sizeof(CIntent));
    CCheats_id = _componentRegister(sizeof(CCheats));
    CBreakable_id = _componentRegister(sizeof(CBreakable));
    CSolid_id = _componentRegister(sizeof(CSolid));
    CLocation_id = _componentRegister(sizeof(CLocation));
    CDebugLabel_id = _componentRegister(sizeof(CDebugLabel));
    CPathfinding_id = _componentRegister(sizeof(CPathfinding));
    CMisc_id = _componentRegister(sizeof(CMisc));
    CMiniInventory_id = _componentRegister(sizeof(CMiniInventory));
    CShake_id = _componentRegister(sizeof(CShake));
    CHint_id = _componentRegister(sizeof(CHint));
    CBoss_id = _componentRegister(sizeof(CBoss));
    // todo: make 0 invalid component id so i detect forgetting this bit

    Ini_readAsset(assetpath);

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_prefabCount == MAX_PREFABS) {
            Log_error("Max prefabs exceeded");
            Ini_clear();
            return;
        }

        Prefab* prefab = &Entity_prefabs[_prefabCount++];
        memset(prefab, 0, sizeof(Prefab));
        strncpy(prefab->name, name, NAME_LENGTH);
        
        prefab->controller = String_parseEnum(Ini_get(name, "controller"),
                "none;player;enemy;boss", 0);
        prefab->solid = String_parseBool(Ini_get(name, "solid"), true);
        prefab->actor = String_parseBool(Ini_get(name, "actor"), false);
        prefab->pushable = String_parseBool(Ini_get(name, "pushable"),
                prefab->actor);
        prefab->weight = String_parseInt(Ini_get(name, "weight"), 10);
        prefab->moveSpeed = String_parseInt(Ini_get(name, "moveSpeed"), 20);
        prefab->spawnXOffs = String_parseInt(Ini_get(name, "spawnXOffs"), 0) * 16;
        prefab->spawnYOffs = String_parseInt(Ini_get(name, "spawnYOffs"), 0) * 16;
        prefab->radius = String_parseInt(Ini_get(name, "radius"), 8) * 16;
        prefab->interaction = String_parseEnum(Ini_get(name, "interaction"),
                "none;debug;lift;talk;craft;chest;harvest;shop", 0);
        prefab->faction = String_parseInt(Ini_get(name, "faction"), 0);
        prefab->maxhp = String_parseInt(Ini_get(name, "hp"), 10);
        prefab->atk = String_parseInt(Ini_get(name, "atk"), 0); 
        prefab->def = String_parseInt(Ini_get(name, "def"), 0);
        prefab->defaultAttack = Attack_find(Ini_get(name, "defaultAttack"));
        prefab->hurtDuration = String_parseInt(Ini_get(name, "hurtDuration"), -1);
        prefab->alertRadius = String_parseInt(Ini_get(name, "alertRadius"), 0) * 16;
        prefab->pathfinding = String_parseBool(Ini_get(name, "pathfinding"), false);
        prefab->breakable = String_parseBool(Ini_get(name, "breakable"), false);
        prefab->breakHits = String_parseInt(Ini_get(name, "breakHits"), 1);
        prefab->throwable = String_parseBool(Ini_get(name, "throwable"), false);
        prefab->thrownAtk = String_parseInt(Ini_get(name, "thrownAtk"), 1);
        prefab->rollSpeed = String_parseInt(Ini_get(name, "rollSpeed"), 1);
        prefab->rollDuration = String_parseInt(Ini_get(name, "rollDuration"), 1);
        prefab->collectible = String_parseBool(Ini_get(name, "collectible"), false);
        prefab->collectGold = String_parseBool(Ini_get(name, "collectGold"), false);
        prefab->item = Item_find(Ini_get(name, "item"));
        prefab->lootTable = LootTable_find(Ini_get(name, "lootTable"));
        prefab->soundDie = Sound_find(Ini_get(name, "soundDie"));
        prefab->particlesDie = Particles_find(Ini_get(name, "particlesDie"));
        prefab->sprite = Sprite_find(Ini_get(name, "sprite"));
        prefab->spritex = String_parseInt(Ini_get(name, "spritex"), 0);
        prefab->spritey = String_parseInt(Ini_get(name, "spritey"), 0);
        prefab->spritez = String_parseInt(Ini_get(name, "spritez"), 0);
        prefab->spriteBob = String_parseInt(Ini_get(name, "spriteBob"), 0);
        prefab->bobDuration = String_parseInt(Ini_get(name, "bobDuration"), 120);
        prefab->animIdle = Animation_find(Ini_get(name, "animIdle"));
        String_parseIntDirs(Ini_get(name, "animWalk"),
                prefab->animWalk, Animation_find, -1);
        String_parseIntDirs(Ini_get(name, "animAttack"),
                prefab->animAttack, Animation_find, -1);
        String_parseIntDirs(Ini_get(name, "animHurt"),
                prefab->animHurt, Animation_find, -1);
        String_parseIntDirs(Ini_get(name, "animRoll"),
                prefab->animRoll, Animation_find, -1);
        String_parseIntDirs(Ini_get(name, "animAlert"),
                prefab->animAlert, Animation_find, -1);
        prefab->harvestedSprite = Sprite_find(Ini_get(name, "harvestedSprite"));
        prefab->tentacle = String_parseBool(Ini_get(name, "tentacle"), false);
        const char* hintText = Ini_get(name, "hintText");
        if (hintText) strncpy(prefab->hintText, hintText, 128);
        prefab->hintRadius = String_parseInt(Ini_get(name, "hintRadius"), 0) * 16;

        int miniInvCount = String_parseIntArrayExt(
            Ini_get(name, "invitems"),
            prefab->invitems, 16, Item_find);
        if (miniInvCount > 0) {
            prefab->hasMiniInv = true;
            for (int i = miniInvCount; i < 16; i++) {
                prefab->invitems[i] = -1;
            }
            int c2 = String_parseIntArray(Ini_get(name, "invqty"),
                prefab->invqty, 16);
            for (int i = c2; i < 16; i++) {
                prefab->invqty[i] = 1;
            }
        }
        prefab->dropInvOnDestroy = String_parseBool(
            Ini_get(name, "dropInvOnDestroy"), false);
        if (prefab->dropInvOnDestroy) prefab->hasMiniInv = true;
    }

    Ini_clear();
}

void Attack_loadFrom(const char* assetpath) {
    Ini_readAsset(assetpath);

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_attackCount == MAX_ATTACKS) {
            Log_error("Max attacks exceeded");
            Ini_clear();
            return;
        }

        Attack* attack = &_attacks[_attackCount++];
        strncpy(attack->name, name, NAME_LENGTH);
        
        attack->duration = String_parseInt(Ini_get(name, "duration"), 24);
        attack->hitDelay = String_parseInt(Ini_get(name, "hitDelay"), 9);
        attack->weaponClass = String_parseInt(Ini_get(name, "weaponClass"), 0);
        attack->friendlyFire = String_parseBool(Ini_get(name, "friendlyFire"), false);
        attack->aoe = String_parseBool(Ini_get(name, "aoe"), false);
        attack->range = String_parseInt(Ini_get(name, "range"), 16) * 16;
        attack->damage = String_parseInt(Ini_get(name, "damage"), 0);
        attack->moveFactor = String_parseFloat(Ini_get(name, "moveFactor"), 0.0f);
        attack->surround = String_parseBool(Ini_get(name, "surround"), false);
        attack->knockback = String_parseInt(Ini_get(name, "knockback"), 0) * 16;
        attack->launch = String_parseInt(Ini_get(name, "launch"), 0) * 16;
        attack->sound = Sound_find(Ini_get(name, "sound"));
        attack->next = Attack_find(Ini_get(name, "next"));
        attack->hitstop = String_parseInt(Ini_get(name, "hitstop"), 0);
    }

    Ini_clear();
}

int Attack_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _attackCount; i++) {
        if (strcmp(_attacks[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find attack %s", name);
    return -1;
}

void Entity_updateAll(void) {
    _playerInteractionPartner = -1;

    if (_ecsNextQuery != 0) {
        Log_error("mismatched query begin/end");
        _ecsNextQuery = 0;
    }

    // UpdatePlayerControllers()
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CLocation, loc);
    // todo: use a more generic 'ItemCollector' tag or something?
    QUERY_COMPONENT(CPlayerController, cpc);
    _queryBegin();
    while (_queryNext()) {
        if (Entity_getState(i) == EntityState_ready) {
            _playerInteractionPartner = _findInteractionPartner(i);
        }
        QUERY_ID(j);
        QUERY_COMPONENT(CLocation, otherLoc);
        QUERY_COMPONENT(CItemCollectible, other_cic);
        _queryBegin();
        while (_queryNext()) {
            // todo: how to make this part of query??
            // separate waiting-to-be-collected tag?
            if (!_componentGet(j, CBeingCollected_id)) {
                int distanceSq = _entDistSq(loc, otherLoc);
                if (distanceSq > 256 * 256) continue;
                _playerCollect(i, j);
            }
        }
        _queryEnd();
        _playerUpdate(i);
    }
    _queryEnd();
    }

    // UpdateEnemyControllers()
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CEnemyController, cec);
    _queryBegin();
    while (_queryNext()) {
        _enemyUpdate(i);
    }
    _queryEnd();
    }

    // follow pathfinding
    // todo: currently gets stuck on NW edges heading SW
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CLocation, location);
    QUERY_COMPONENT(CPathfinding, pathfinding);
    _queryBegin();
    while (_queryNext()) {
        if (pathfinding->progress >= pathfinding->length) continue;
        int nextX = pathfinding->xs[pathfinding->progress] * 256 + 128;
        int nextY = pathfinding->ys[pathfinding->progress] * 256 + 128;
        if (Math_lengthSquared(location->x-nextX, location->y-nextY) < 8*16*8*16) {
            pathfinding->progress++;
            if (pathfinding->progress >= pathfinding->length) continue;
            int nextX = pathfinding->xs[pathfinding->progress] * 256 + 128;
            int nextY = pathfinding->ys[pathfinding->progress] * 256 + 128;
        }
        float dx = nextX - location->x;
        float dy = nextY - location->y;
        Math_normalizeXY(&dx, &dy);

        if (pathfinding->speed == -1) {
            CIntent* intent = _componentGet(i, CIntent_id);
            if (intent) {
                intent->intentx = dx;
                intent->intenty = dy;
                if (intent->intent == EntityIntent_none) {
                    intent->intent = EntityIntent_walk;
                }
            }
        } else {
            Entity_applyMotion(i, dx*pathfinding->speed, dy*pathfinding->speed);
        }
    }
    _queryEnd();
    }

    // UpdateBoss()
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CBoss, boss);
    QUERY_COMPONENT(CLocation, loc);
    QUERY_COMPONENT(CAnimation, anm);
    QUERY_COMPONENT(CCheats, cheats);
    // QUERY_COMPONENT(CActor, actor); // limit 4 columns per query
    _queryBegin();
    while (_queryNext()) {
        // boss strategy: alternate spawning slimes and tentacles
        // stun main boss when a tentacle is destroyed
        int player = Entity_getPlayer();
        if (player == -1) continue;
        CLocation* player_loc = _componentGet(player, CLocation_id);
        AnimationID anm_idle = Animation_find("boss_idle2");
        AnimationID anm_stun = Animation_find("boss_idle");
        if (!boss->active) {
            if (_entDistSq(loc, player_loc) < 1500*1500) {
                Log_debug("ACTIVATE");
                boss->active = true;
            } else continue;
        } else {
            if (_entDistSq(loc, player_loc) > 3000*3000) {
                Log_debug("DEACTIV");
                boss->active = false; continue;
            }
        }
        if (boss->stun > 0) {
            boss->stun--;
            anm->animIdle = anm_stun;
            cheats->god = false;
            continue;
        } else {
            cheats->god = true;
            anm->animIdle = anm_idle;
        }
        boss->timer++;
        if (boss->phase) {
            boss->stun_tents = 0;
            // SPAWN TENTACLES
            if (boss->timer == 10) for (int i = 0; i < 3; i++) {
                int x = loc->x + rand() % 1000 - 500;
                int y = loc->y + 200 + rand() % 1000;
                int prefab = Entity_findPrefab("boss_spawn");
                Entity_spawn(x, y, prefab);
            }
            if (boss->timer > 100) {
                boss->phase = false;
                boss->timer = 0;
            }
        } else {
            // SPAWN SLIMES
            int amount = 1; // should be based on hp
            CActor* actor = _componentGet(i, CActor_id);
            // <75%: spawn 2
            // <50%: spawn 3
            // <25%: spawn 4
            if (actor && actor->hp < 75) amount++;
            if (actor && actor->hp < 50) amount++;
            if (actor && actor->hp < 25) amount++;
            if (boss->timer < 10*amount && boss->timer % 10 == 9) {
                int x = loc->x + rand() % 1000 - 500;
                int y = loc->y + 200 + rand() % 1000;
                int prefab = Entity_findPrefab("slime");
                Entity_spawn(x, y, prefab);
            }
            if (boss->timer > 200) {
                boss->phase = true;
                boss->timer = 0;
            }
        }
    }
    _queryEnd();
    }

    // UpdateIntent()
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CIntent, intent);
    QUERY_COMPONENT(CActor, actor);
    _queryBegin();
    while (_queryNext()) {
        // UpdateState() calls into other updates based on switch?
        // UpdateReady()
        // todo: will we need to delay state changes somehow?
        // otherwise depending on ordering state updates could fire twice
        // e.g. UpdateReady() (ready -> attack) UpdateAttack()
        // something like unity's entity command buffers?
        // (delay component add/removes)
        //  note: actually this first phase more like update intent?
        // todo: the above todos are out of date
        EntityState entState = Entity_getState(i);
        if (entState == EntityState_ready) {
            // should only attach if nonzero
            if (intent->intentx ||intent->intenty) {
                int mx = (int) (intent->intentx * actor->moveSpeed);
                int my = (int) (intent->intenty * actor->moveSpeed);
                Entity_applyMotion(i, mx, my);
            }
            if (intent->intent == EntityIntent_attack) {
                AttackID attack = -1;
                CActor* ca = _componentGet(i, CActor_id);
                if (intent->kind == 0) {
                    attack = ca->defaultAttack;
                } else {
                    // todo: separate this into reused function
                    //       getStat uses this also
                    bool isPlayer = _componentGet(i, CPlayerController_id) != NULL;
                    if (isPlayer) {
                        ItemID weapon = Inventory_getPlayerEquip(EquipType_weapon);
                        attack = Item_getSkill(weapon, intent->kind - 1);
                    }
                }
                if (ca && attack != -1) {
                    CStateAttack* csa = _changeState(i, EntityState_attack);
                    csa->attackID = attack;
                }
            } else if (intent->intent == EntityIntent_interact) {
                // find entity to interact with
                int other = intent->intentPartner;
                CInteraction* other_ci =
                    _componentGet(other, CInteraction_id);
                if (other_ci) {
                    if (other_ci->type == InteractionType_debug) {
                        const char* name = _getDebugName(other);
                        Log_debug("interact with %s", name);
                    } else if (other_ci->type == InteractionType_talk) {
                        Game_setDialog(other);
                    } else if (other_ci->type == InteractionType_craft) {
                        Game_setCrafting();
                    } else if (other_ci->type == InteractionType_chest) {
                        Log_debug("CHEST CONTENT:");
                        CMiniInventory* inv = _componentGet(
                            other, CMiniInventory_id
                        );
                        if (inv) {
                            for (int i = 0; i < 16; i++) {
                                if (inv->items[i] != -1) {
                                    Log_debug("  - %s (%dx)",
                                        Item_getDisplayName(inv->items[i]),
                                        inv->quantity[i]);
                                }
                            }
                        }
                        Game_setChest(other);
                    } else if (other_ci->type == InteractionType_harvest) {
                        CMisc* other_misc = _componentGet(other, CMisc_id);
                        CLocation* other_loc = _componentGet(other, CLocation_id);
                        LootTable_drop(other_misc->lootTable,
                            other_loc->x, other_loc->y+64);
                        _componentDetach(other, CInteraction_id);
                        CSprite* other_sprite = _componentGet(other, CSprite_id);
                        if (other_sprite && other_misc->harvestedSprite != -1) {
                            other_sprite->sprite = other_misc->harvestedSprite;
                        }
                    } else if (other_ci->type == InteractionType_shop) {
                        Game_setShop();
                    }
                }
            } else if (intent->intent == EntityIntent_roll) {
                float mx = intent->intentx;
                float my = intent->intenty;
                if (mx != 0 || my != 0) {
                    Math_normalizeXY(&mx, &my);
                    CStateRolling* csr = _changeState(i, EntityState_rolling);
                    csr->rollx = (int) (mx * csr->rollSpeed);
                    csr->rolly = (int) (my * csr->rollSpeed);
                }
            } else if (intent->intent == EntityIntent_walk) {
                Entity_setAnimState(i, AnimState_walking);
                CAnimation* ca = _componentGet(i, CAnimation_id);
                if (ca) {
                    if (fabsf(intent->intentx) >= fabsf(intent->intenty)) {
                        ca->facing = intent->intentx > 0 ? Facing_right : Facing_left;
                    } else {
                        ca->facing = intent->intenty > 0 ? Facing_down : Facing_up;
                    }
                }
            } else if (intent->intent == EntityIntent_none) {
                Entity_setAnimState(i, AnimState_idle);
            }
        } else if (entState == EntityState_attack) {
            if (intent->intent == EntityIntent_attack) {
                CStateAttack* csa = _componentGet(i, CStateAttack_id);
                if (csa && csa->attackID != -1) {
                    csa->attackQueued = _attacks[csa->attackID].next;
                }
            } else if (intent->intent == EntityIntent_roll) {
                float mx = intent->intentx;
                float my = intent->intenty;
                Math_normalizeXY(&mx, &my);
                CStateRolling* csr = _changeState(i, EntityState_rolling);
                csr->rollx = (int) (mx * csr->rollSpeed);
                csr->rolly = (int) (my * csr->rollSpeed);
            }
        }

    }
    _queryEnd();
    }

    // UpdateAttack()
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CStateAttack, csa);
    QUERY_COMPONENT(CActor, entActor);
    QUERY_COMPONENT(CLocation, entLoc);
    _queryBegin();
    while (_queryNext()) {
        csa->attackTime++;
        Attack* attack = &_attacks[csa->attackID];
        static int facingAngles[] = { 180, 0, 90, 270 };
        
        // controlled motion during attack
        if (csa->attackTime > attack->hitDelay) {
            float mf = attack->moveFactor;
            // todo: enemies can still move during attack from
            //       being pushed by others. should entities have
            //       increased mass (or become solid?) during combat?
            CIntent* intent = _componentGet(i, CIntent_id);
            if (intent) {
                int mx = (int) (intent->intentx * entActor->moveSpeed * mf);
                int my = (int) (intent->intenty * entActor->moveSpeed * mf);
                Entity_applyMotion(i, mx, my);
            }
        }

        // launch at start of attack
        if (csa->attackTime == 1) {
            CLaunch* cl = _componentGetOrAttach(i, CLaunch_id);
            if (cl) {
                cl->launch = attack->launch;

                CIntent* intent = _componentGet(i, CIntent_id);
                if (intent && (intent->intentx || intent->intenty)) {
                    cl->launchAngle = Math_angleTo(0, 0,
                            intent->intentx, intent->intenty);
                } else {
                    cl->launchAngle = facingAngles[Entity_getFacing(i)];
                }
            }
            Sound_play(attack->sound);
        }
        if (csa->attackTime == attack->hitDelay) {
            QUERY_ID(j);
            QUERY_COMPONENT(CLocation, otherLoc);
            _queryBegin();
            while (_queryNext()) {
                CActor* otherActor = _componentGet(j, CActor_id);
                if (otherActor && entActor->faction == otherActor->faction
                    && !attack->friendlyFire) continue;
                int distanceSq = _entDistSq(entLoc, otherLoc);
                if (distanceSq > attack->range * attack->range) continue;
                if (_isInvincible(j)) continue;

                // don't attack if other is behind this entity
                int angleTo = Math_angleTo(entLoc->x, entLoc->y,
                    otherLoc->x, otherLoc->y);
                int entAngle = facingAngles[Entity_getFacing(i)];
                int angleDiff = Math_sanitizeAngle(angleTo - entAngle);
                if (!attack->surround && angleDiff > 90 && angleDiff < 270) {
                    continue;
                }

                CBreakable* breakable = _componentGet(j, CBreakable_id);
                if (breakable) {
                    breakable->breakHits--;
                    if (breakable->breakHits <= 0) {
                        CMisc* misc = _componentGet(j, CMisc_id);
                        if (misc) {
                            LootTable_drop(misc->lootTable, otherLoc->x, otherLoc->y);
                            Sound_play(misc->soundDie);
                            Particles_spawn(misc->particlesDie,
                                otherLoc->x, otherLoc->y, 4);
                            Entity_dropGold(otherLoc->x, otherLoc->y, rand()%3);
                        }
                        CMiniInventory* inv = _componentGet(j, CMiniInventory_id);
                        if (inv && inv->dropInvOnDestroy) {
                            for (int k = 0; k < 16; k++) {
                                if (inv->items[k] != -1 && inv->quantity[k]) {
                                    for (int l = 0; l < inv->quantity[k]; l++) {
                                        Entity_dropItem(
                                            otherLoc->x, otherLoc->y,
                                            inv->items[k]);
                                    }
                                }
                            }
                        }
                        Entity_destroy(j);
                    } else {
                        Entity_shake(j, 2);
                    }
                } else if (otherActor) {
                    int atk = Entity_getStat(i, Stat_atk);
                    int def = Entity_getStat(j, Stat_def);
                    int dmg = atk + attack->damage - def;
                    int lck = Entity_getStat(i, Stat_lck);
                    // lck 0: 0% chance crit
                    // lck 10: 25% chance crit
                    int critChance = (lck * 25) / 10;
                    if (rand() % 100 < critChance) {
                        dmg *= 2;
                        SoundID snd_crit = Sound_find("crit");
                        Sound_play(snd_crit);
                        ParticlesID pt_crit = Particles_find("pt_crit");
                        Particles_spawn(pt_crit,
                            otherLoc->x, otherLoc->y, 4);
                    }
                    if (dmg <= 0) continue;
                    if (attack->hitstop > 0) Game_setHitstop(attack->hitstop);
                    otherActor->hp -= (dmg + 1) / 2;
                    if (otherActor->hp <= 0) {
                        CMisc* misc = _componentGet(j, CMisc_id);
                        if (misc) {
                            LootTable_drop(misc->lootTable, otherLoc->x, otherLoc->y);
                            Sound_play(misc->soundDie);
                            Particles_spawn(misc->particlesDie,
                                otherLoc->x, otherLoc->y, 4);
                            CBoss* boss = _componentGet(j, CBoss_id);
                            if (!misc->tentacle && !boss)
                                Quest_signalEvent(QuestEvent_defeat, j);
                            else if (boss) {
                                Log_debug("BOSS DEFEAT");
                                Quest_signalEvent(QuestEvent_defeatBoss, j);
                            }
                            if (!misc->tentacle)
                                Entity_dropGold(otherLoc->x, otherLoc->y, 1 + rand() % 5);
                            else {
                                QUERY_ID(i);
                                QUERY_COMPONENT(CBoss, boss);
                                QUERY_COMPONENT(CActor, actor);
                                _queryBegin();
                                while (_queryNext()) {
                                    if (++boss->stun_tents == 3) {
                                        boss->stun += 200;
                                    } else {
                                        boss->stun += 50;
                                    }
                                    if (actor->hp > 1) actor->hp -= 1;
                                    else actor->hp = 1;
                                }
                                _queryEnd();
                            }
                        }
                        CMiniInventory* inv = _componentGet(j, CMiniInventory_id);
                        if (inv && inv->dropInvOnDestroy) {
                            for (int k = 0; k < 16; k++) {
                                if (inv->items[k] != -1 && inv->quantity[k]) {
                                    for (int l = 0; l < inv->quantity[k]; l++) {
                                        Entity_dropItem(
                                            otherLoc->x, otherLoc->y,
                                            inv->items[k]);
                                    }
                                }
                            }
                        }

                        bool isPlayer = _componentGet(j,
                            CPlayerController_id) != NULL;
                        if (isPlayer) {
                            Game_setDefeat();
                        }

                        otherActor->hp = 0;
                        Entity_destroy(j);

                        int delta = otherActor->lvl - entActor->lvl + 1;
                        if (delta > 0) {
                            entActor->xp += delta;
                            while (entActor->xp > 50) {
                                entActor->lvl++;
                                entActor->xp -= 50;
                                Log_debug("LEvel up! Now %d", entActor->lvl);
                            }
                        }
                    } else {
                        SoundID snd_hit = Sound_find("hit1");
                        Sound_play(snd_hit);
                        CLaunch* cl = _componentAttach(j, CLaunch_id);
                        cl->launch = attack->knockback;
                        cl->launchAngle = angleTo;
                        _changeState(j, EntityState_hurt);
                    }
                } else continue; // todo: feelin a little spaghetti
                if (!attack->aoe) break;
            }
            _queryEnd();
        }

        // End of attack (stop attack or next queued)
        if (csa->attackTime > attack->duration) {
            if (csa->attackQueued != -1) {
                csa->attackID = csa->attackQueued;
                csa->attackQueued = -1;
                csa->attackTime = 0;
            } else {
                _changeState(i, EntityState_ready);
            }
        }
    }
    _queryEnd();
    }

    // UpdateHurt()
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CStateHurt, csh);
    _queryBegin();
    while (_queryNext()) {
        csh->hurtTime++;
        int duration = csh->hurtDuration;
        if (duration == -1) {
            AnimationID animHurt = Entity_getCurrentAnim(i);
            duration = Animation_getDuration(animHurt);
        }
        if (csh->hurtTime > duration) {
            _changeState(i, EntityState_ready);
        }
    }
    _queryEnd();
    }

    // UpdateRolling()
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CStateRolling, csr);
    _queryBegin();
    while (_queryNext()) {
        csr->rollTime++;
        if (csr->rollTime > csr->rollDuration) {
            _changeState(i, EntityState_ready);
        } else {
            Entity_applyMotion(i, csr->rollx, csr->rolly);
        }
    }
    _queryEnd();
    }

    // UpdateAlert()
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CStateAlert, csa);
    _queryBegin();
    while (_queryNext()) {
        csa->alertTime++;
        AnimationID animAlert = Entity_getCurrentAnim(i);
        int alertDuration = Animation_getDuration(animAlert);
        if (csa->alertTime > alertDuration) {
            _changeState(i, EntityState_ready);
        }
    }
    _queryEnd();
    }

    // UpdateStatusEffects()
    {
    QUERY_COMPONENT(CStatusEffects, cse);
    _queryBegin();
    while (_queryNext()) {
        for (int j = 0; j < 4; j++) {
            if (cse->durations[j]) {
                cse->durations[j]--;
            }
        }
    }
    _queryEnd();
    }

    // UpdateBeingCollected()
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CBeingCollected, cbc);
    QUERY_COMPONENT(CLocation, loc);
    _queryBegin();
    while (_queryNext()) {
        CLocation* otherLoc = _componentGet(cbc->collector, CLocation_id);
        if (!otherLoc) {
            _componentDetach(i, CBeingCollected_id);
            continue;
        }
        
        int angle = Math_angleTo(loc->x, loc->y, otherLoc->x, otherLoc->y);
        int distSq = _entDistSq(loc, otherLoc);
        int speed = cbc->collectTime;
        if (speed > 40) speed = 40;
        if (distSq < speed * speed) {
            Entity_applyMotion(i, otherLoc->x - loc->x, otherLoc->y - loc->y);
            if (cbc->collectTime < 40) cbc->collectTime = 40;
        } else {
            Entity_applyMotion(i,
                Math_angleLengthX(angle, speed),
                Math_angleLengthY(angle, speed));
        }
        if (cbc->collectTime > 40) loc->zoff++;
        if (cbc->collectTime > 50) loc->zoff++;
        cbc->collectTime++;
        if (cbc->collectTime == 55) {
            CItemCollectible* cic = _componentGet(i, CItemCollectible_id);
            if (cic) {
                if (cic->isGold) {
                    Game_addGold(1);
                } else if (cic->item != -1) {
                    // todo: handle if addAll fails (inv full)
                    // todo: should we add first then play anim of colelcting?
                    //       (if so, should collecting be some kind of
                    //       particle-y thing instead of entity?)
                    Inventory_addAll(cic->item, 1);
                }
            }
            Entity_destroy(i);
        }
    }
    _queryEnd();
    }

    // UpdateShake
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CShake, shake);
    QUERY_COMPONENT(CSprite, sprite);
    _queryBegin();
    while (_queryNext()) {
        shake->timer--;
        if (shake->timer == 0) {
            _componentDetach(i, CShake_id);
        } else {
            int frame = shake->timer % 6;
            if (frame == 1) {
                sprite->spritex -= 1;
            } else if (frame == 5) {
                sprite->spritex += 1;
            }
        }
    }
    _queryEnd();
    }

    // Play_UpdateLaunch()? FJ_UpdateLaunch()?
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CLaunch, cl);
    _queryBegin();
    while (_queryNext()) {
        if (cl->launch > 0) {
            // todo: should mass affect launch strength?
            //       mass of attacker also?
            int launchDelta = ceilf(cl->launch * 0.2f);
            cl->launch -= launchDelta;
            int mx = Math_angleLengthX(cl->launchAngle, launchDelta);
            int my = Math_angleLengthY(cl->launchAngle, launchDelta);
            Entity_applyMotion(i, mx, my);
        } else {
            _componentDetach(i, CLaunch_id);
        }
    }
    _queryEnd();
    }

    // UpdateHint()
    {
    int player = Entity_getPlayer();
    if (player == -1) goto skip_hint;
    CLocation* player_loc = _componentGet(player, CLocation_id);
    const char* nearestText = NULL;
    uint32_t nearestDistSq = UINT32_MAX;
    QUERY_COMPONENT(CHint, hint);
    QUERY_COMPONENT(CLocation, location);
    _queryBegin();
    while (_queryNext()) {
        uint32_t distSq = _entDistSq(player_loc, location);
        if (distSq < hint->radius * hint->radius && distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestText = hint->text;
        }
    }
    Game_setHintText(nearestText);
    _queryEnd();
    skip_hint:;
    }

    // entity motion
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CLocation, loc);
    QUERY_COMPONENT(CMotion, cm);
    _queryBegin();
    while (_queryNext()) {
        loc->prevx = loc->x;
        loc->prevy = loc->y;
        loc->x += cm->movex;
        loc->y += cm->movey;
        _componentDetach(i, CMotion_id);
    }
    _queryEnd();
    }

    // resolve entity intersections
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CSolid, entSolid);
    QUERY_COMPONENT(CLocation, entLoc);
    _queryBegin();
    while (_queryNext()) {
        CCheats* cheats = _componentGet(i, CCheats_id);
        if (cheats && cheats->noclip) continue;

        QUERY_ID(j);
        QUERY_COMPONENT(CSolid, otherSolid);
        QUERY_COMPONENT(CLocation, otherLoc);
        _queryBegin();
        while (_queryNext()) {
            if (i == j) continue;
            CCheats* ocheats = _componentGet(j, CCheats_id);
            if (ocheats && ocheats->noclip) continue;

            if (!entSolid->pushable && !otherSolid->pushable) continue;

            int distSq = _entDistSq(entLoc, otherLoc);
            int contactDist = entSolid->radius + otherSolid->radius;
            if (distSq < contactDist * contactDist) {
                int overlap = contactDist - (int) sqrtf(distSq);
                int angleToOther = Math_angleTo(entLoc->x, entLoc->y,
                    otherLoc->x, otherLoc->y);
                
                // if both pushable, amount pushed is distributed by weight
                int entForce = 0, otherForce = 0;
                if (entSolid->pushable && otherSolid->pushable) {
                    int totalWeight = entSolid->weight + otherSolid->weight;
                    float weightFactor = (float) otherSolid->weight / totalWeight;
                    entForce = roundf(weightFactor * overlap);
                    otherForce = overlap - entForce;
                } else {
                    if (entSolid->pushable) entForce = overlap;
                    else otherForce = overlap;
                }

                if (entForce > 0) {
                    entLoc->x += Math_angleLengthX(angleToOther + 180, entForce);
                    entLoc->y += Math_angleLengthY(angleToOther + 180, entForce);
                }
                if (otherForce > 0) {
                    otherLoc->x += Math_angleLengthX(angleToOther, otherForce);
                    otherLoc->y += Math_angleLengthY(angleToOther, otherForce);
                }
            }
        }
        _queryEnd();
    }
    _queryEnd();
    }

    // resolve entity-field intersections
    {
    QUERY_ID(i);
    QUERY_COMPONENT(CSolid, solid);
    QUERY_COMPONENT(CLocation, loc);
    _queryBegin();
    while (_queryNext()) {
        CCheats* cheats = _componentGet(i, CCheats_id);
        if (cheats && cheats->noclip) continue;

        for (int attempt = 0; attempt < 10; attempt++) {
            FieldNearest nearest = Field_findNearest(loc->x, loc->y);
            if (nearest.isInside || nearest.distanceSquared == 0) {
                loc->x = nearest.x + Math_cos(nearest.angle) * solid->radius;
                loc->y = nearest.y - Math_sin(nearest.angle) * solid->radius;
            } else if (nearest.distanceSquared < solid->radius * solid->radius) {
                int relativeNormal =
                    Math_angleTo(nearest.x, nearest.y, loc->x, loc->y);
                loc->x = nearest.x + Math_cos(relativeNormal) * solid->radius;
                loc->y = nearest.y - Math_sin(relativeNormal) * solid->radius;
            } else break;
        }
    }
    _queryEnd();
    }
}

void Entity_renderAll(void) {
    QUERY_ID(i);
    QUERY_COMPONENT(CLocation, loc);
    QUERY_COMPONENT(CAnimation, ca);
    _queryBegin();
    while (_queryNext()) {
        bool interactee = i == _playerInteractionPartner;

        if (Config_showCollision) {
            Draw_setColor(Color_fuchsia);
            CSolid* solid = _componentGet(i, CSolid_id);
            if (solid) Draw_circle(loc->x/16, loc->y/16, solid->radius/16);
        }

        // reset anim timer when entering a new state
        if (ca->animState != ca->prevAnimState) {
            ca->prevAnimState = ca->animState;
            ca->time = 0;
        }

        // play/draw animation
        AnimationID anim = Entity_getCurrentAnim(i);
        if (anim != -1) {
            if (interactee) Graphics_setModulationColor(255, 0, 0);
            SpriteQueue_addAnim(anim, loc->x/16, loc->y/16,
                    loc->zoff, &ca->time);
            if (interactee) Graphics_clearModulationColor();
        }
    }
    _queryEnd();

    {
    QUERY_ID(i);
    QUERY_COMPONENT(CLocation, loc);
    QUERY_COMPONENT(CSprite, cs);
    _queryBegin();
    while (_queryNext()) {
        // todo: somehow store interactee in ECS?
        // (via tag component?)
        bool interactee = i == _playerInteractionPartner;

        int z = cs->spritez + loc->zoff;
        if (cs->spriteBob > 0) {
            float bobTime = cs->bobTimer++ / (float) cs->bobDuration;
            z += Math_sin(bobTime * 360.f) * cs->spriteBob;
        }
        if (interactee) Graphics_setModulationColor(255, 0, 0);
        SpriteQueue_addSprite(cs->sprite,
                loc->x/16 + cs->spritex,
                loc->y/16 + cs->spritey, z);
        if (interactee) Graphics_clearModulationColor();
    }
    _queryEnd();
    }

    {
    QUERY_ID(i);
    QUERY_COMPONENT(CInteraction, intr);
    _queryBegin();
    while (_queryNext()) {
        bool interactee = i == _playerInteractionPartner;
        if (!interactee) continue;
        SpriteID spr = -1;
        if (intr->type == InteractionType_talk ||
            intr->type == InteractionType_shop) {
            spr = Sprite_find("ui/intrTalk");
        } else if (intr->type == InteractionType_chest) {
            spr = Sprite_find("ui/intrItem");
        }
        int player = Entity_getPlayer();
        if (player == -1) continue;
        CLocation* loc = _componentGet(player, CLocation_id);
        if (loc == NULL) continue;
        if (spr != -1) {
            SpriteQueue_addSprite(spr, loc->x/16, loc->y/16+1, 17);
        }
    }
    _queryEnd();
    }

    {
    QUERY_ID(i);
    QUERY_COMPONENT(CVillager, villager);
    QUERY_COMPONENT(CLocation, loc);
    _queryBegin();
    while (_queryNext()) {
        DialogID nextDlg = Dialog_findCandidate(villager->villager);
        int iconId = Dialog_getDisplayIcon(nextDlg);
        SpriteID spr = -1;
        if (iconId == 1) {
            spr = Sprite_find("ui/qmwhite");
        } else if (iconId == 2) {
            spr = Sprite_find("ui/qmgreen");
        } else if (iconId == 3) {
            spr = Sprite_find("ui/qmgray");
        };
        if (spr != -1) {
            SpriteQueue_addSprite(spr, loc->x/16, loc->y/16+1, 17);
        }
    }
    _queryEnd();
    }

    if (Config_showCollision) {
        Field_drawDebug();
        int playerID = Entity_getPlayer();
        CLocation* loc = _componentGet(playerID, CLocation_id);
        if (loc) {
            Field_drawDebugCollision(loc->x, loc->y);
        }
    }

    if (Config_showNavGrid) {
        QUERY_ID(i);
        QUERY_COMPONENT(CLocation, location);
        QUERY_COMPONENT(CPathfinding, pathfinding);
        _queryBegin();
        while (_queryNext()) {
            if (pathfinding->progress >= pathfinding->length) continue;
            int nextX = pathfinding->xs[pathfinding->progress] * 256;
            int nextY = pathfinding->ys[pathfinding->progress] * 256;
            if (Math_lengthSquared(location->x-nextX, location->y-nextY) < 4*16) {
                pathfinding->progress++;
                if (pathfinding->progress >= pathfinding->length) continue;
                int nextX = pathfinding->xs[pathfinding->progress] * 256;
                int nextY = pathfinding->ys[pathfinding->progress] * 256;
            }
            float dx = nextX - location->x;
            float dy = nextY - location->y;
            Math_normalizeXY(&dx, &dy);
            Entity_applyMotion(i, dx*pathfinding->speed, dy*pathfinding->speed);
            
            int prevX = location->x/256;
            int prevY = location->y/256;
            for (int i = 0; i < pathfinding->length; i++) {
                Draw_setColor(i < pathfinding->progress ? Color_olive : Color_green);
                Draw_line(prevX*16+8, prevY*16+8,
                    pathfinding->xs[i]*16+8, pathfinding->ys[i]*16+8);
                prevX = pathfinding->xs[i];
                prevY = pathfinding->ys[i];
            }
        }
        _queryEnd();
    }

    // int entCount = 0;
    // for (int i = 0; i < ECS_MAX_ENTITIES; i++) {
    //     if (_ecsEntityValid[i]) {
    //         entCount++;
    //     }
    // }

    // Draw_setTranslate(0, 0);
    // Draw_text(5, SCREEN_HEIGHT - 20, "%d/%d ents (%d%%)",
    //     entCount, ECS_MAX_ENTITIES, entCount * 100 / ECS_MAX_ENTITIES);
}

int Entity_spawn(int x, int y, int prefabID) {
    if (prefabID < 0 || prefabID > _prefabCount) return 0;
    // int id = 0;
    // while (id < MAX_ENTITIES && Entity_table[id].valid) id++;
    // if (id == MAX_ENTITIES) {
    //     Log_error("Max entities reached");
    //     return -1;
    // }

    int id = _entityCreate();
    if (!id) return 0;
    Prefab* prefab = &Entity_prefabs[prefabID];
    CDebugLabel* debugLabel = _componentAttach(id, CDebugLabel_id);
    snprintf(debugLabel->name, NAME_LENGTH, "%s.%d",
        prefab->name, ++prefab->cloneCount);
    CLocation* loc = _componentAttach(id, CLocation_id);
    loc->prevx = loc->x = x + prefab->spawnXOffs;
    loc->prevy = loc->y = y + prefab->spawnYOffs;
    if (prefab->collectible) {
        CItemCollectible* cic = _componentAttach(id, CItemCollectible_id);
        cic->item = prefab->item;
        cic->isGold = prefab->collectGold;
    }
    if (prefab->sprite != -1) {
        CSprite* cs = _componentAttach(id, CSprite_id);
        cs->sprite = prefab->sprite;
        cs->spritex = prefab->spritex;
        cs->spritey = prefab->spritey;
        cs->spritez = prefab->spritez;
        cs->spriteBob = prefab->spriteBob;
        cs->bobDuration = prefab->bobDuration;
    }
    if (prefab->animIdle != -1) {
        CAnimation* ca = _componentAttach(id, CAnimation_id);
        ca->animIdle = prefab->animIdle;
        for (int i = 0; i < 4; i++) {
            ca->animWalk[i] = prefab->animWalk[i];
            ca->animAttack[i] = prefab->animAttack[i];
            ca->animHurt[i] = prefab->animHurt[i];
            ca->animRoll[i] = prefab->animRoll[i];
            ca->animAlert[i] = prefab->animAlert[i];
        }
    }
    if (prefab->interaction) {
        CInteraction* ci = _componentAttach(id, CInteraction_id);
        ci->type = prefab->interaction;
    }
    if (prefab->rollSpeed || prefab->rollDuration || prefab->hurtDuration) {
        CStateParams* csp = _componentAttach(id, CStateParams_id);
        csp->rollSpeed = prefab->rollSpeed;
        csp->rollDuration = prefab->rollDuration;
        csp->hurtDuration = prefab->hurtDuration;
    }
    if (prefab->actor) {
        CActor* ca = _componentAttach(id, CActor_id);
        ca->lvl = prefab->lvl;
        ca->xp = prefab->xp;
        ca->faction = prefab->faction;
        ca->maxhp = prefab->maxhp;
        ca->hp = ca->maxhp;
        ca->atk = prefab->atk;
        ca->def = prefab->def;
        ca->defaultAttack = prefab->defaultAttack;
        ca->moveSpeed = prefab->moveSpeed;
    }
    if (prefab->controller == EntityController_player) {
        CPlayerController* cpc = _componentAttach(id, CPlayerController_id);
        _componentAttach(id, CIntent_id);
        _componentAttach(id, CCheats_id);
    } else if (prefab->controller == EntityController_enemy) {
        CEnemyController* cec = _componentAttach(id, CEnemyController_id);
        cec->alertRadius = prefab->alertRadius;
        cec->pathfinding = prefab->pathfinding;
        _componentAttach(id, CIntent_id);
    } else if (prefab->controller == EntityController_boss) {
        CBoss* boss = _componentAttach(id, CBoss_id);
        _componentAttach(id, CCheats_id);
    }
    if (prefab->breakable) {
        CBreakable* breakable = _componentAttach(id, CBreakable_id);
        breakable->breakHits = prefab->breakHits;
    }
    if (prefab->solid) {
        CSolid* cs = _componentAttach(id, CSolid_id);
        cs->pushable = prefab->pushable;
        cs->weight = prefab->weight;
        cs->radius = prefab->radius;
    }
    CMisc* misc = _componentAttach(id, CMisc_id);
    misc->lootTable = prefab->lootTable;
    misc->soundDie = prefab->soundDie;
    misc->particlesDie = prefab->particlesDie;
    misc->harvestedSprite = prefab->harvestedSprite;
    misc->tentacle = prefab->tentacle;
    if (prefab->hasMiniInv) {
        CMiniInventory* inv = _componentAttach(id, CMiniInventory_id);
        memcpy(inv->items, prefab->invitems, sizeof(inv->items));
        memcpy(inv->quantity, prefab->invqty, sizeof(inv->quantity));
        inv->dropInvOnDestroy = prefab->dropInvOnDestroy;
    }
    if (prefab->hintRadius > 0) {
        CHint* hint = _componentAttach (id, CHint_id);
        strncpy(hint->text, prefab->hintText, 128);
        hint->radius = prefab->hintRadius;
    }

    return id;
}

void Entity_destroy(int id) {
    _entityDestroy(id);
}

void Entity_destroyAll(void) {
    for (int i = 0; i < _ecsNextComponent; i++) {
        for (int j = 0; j < ECS_MAX_ENTITIES; j++) {
            _ecsComponents[i].valid[j] = false;
        }
    }
    for (int i = 0; i < ECS_MAX_ENTITIES; i++) {
        _ecsEntityValid[i] = false;
        _ecsEntityGeneration[i] = 0;
    }
}

int Entity_getPlayer(void) {
    QUERY_ID(i);
    QUERY_COMPONENT(CPlayerController, cpc);
    _queryBegin();
    while (_queryNext()) {
        _queryEnd();
        return i;
    }
    _queryEnd();
    return -1;
}

int Entity_findPrefab(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _prefabCount; i++) {
        if (strcmp(Entity_prefabs[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find prefab %s", name);
    return -1;
}

void Entity_dropItem(int x, int y, ItemID item) {
    if (item == -1) return;
    int prefab = Entity_findPrefab("item_drop");
    int entity = Entity_spawn(x, y, prefab);
    if (entity) {
        CItemCollectible* cic = _componentGetOrAttach(entity, CItemCollectible_id);
        cic->item = item;
        CSprite* cs = _componentGetOrAttach(entity, CSprite_id);
        cs->sprite = Item_getIcon(item);
    }
}

void Entity_dropGold(int x, int y, int amount) {
    for (int i = 0; i < amount; i++) {
        int prefab = Entity_findPrefab("crown_drop");
        int entity = Entity_spawn(x, y, prefab);
    }
}

void Entity_addStatusEffect(int id, StatusEffectID seID) {
    CStatusEffects* cse = _componentGetOrAttach(id, CStatusEffects_id);
    if (!cse) return;
    for (int i = 0; i < 4; i++) {
        if (cse->durations[i] == 0) {
            cse->activeIDs[i] = seID;
            cse->durations[i] = 20 * 60; // todo: durations?
            break;
        }
    }
}

int Entity_getStat(int id, Stat stat) {
    CActor* actor = _componentGet(id, CActor_id);
    if (!actor) return 0;
    int value;
    switch (stat) {
        case Stat_atk: value = actor->atk; break;
        case Stat_def: value = actor->def; break;
        default: value = 0;
    }

    // todo: better way to handle this
    //       (have some equips component or something?)
    bool isPlayer = _componentGet(id, CPlayerController_id) != NULL;
    if (stat == Stat_atk) {
        if (isPlayer) {
            ItemID weapon = Inventory_getPlayerEquip(EquipType_weapon);
            value += Item_getStat(weapon, Stat_atk);
        }
    } else if (stat == Stat_def) {
        if (isPlayer) {
            ItemID armorHead = Inventory_getPlayerEquip(EquipType_head);
            value += Item_getStat(armorHead, Stat_def);
            ItemID armorBody = Inventory_getPlayerEquip(EquipType_body);
            value += Item_getStat(armorBody, Stat_def);
        }
    } else if (stat == Stat_lck) {
        if (isPlayer) {
            ItemID armorAccessory = Inventory_getPlayerEquip(EquipType_accessory);
            value += Item_getStat(armorAccessory, Stat_lck);
        }
    }

    CStatusEffects* cse = _componentGet(id, CStatusEffects_id);
    if (cse) {
        for (int i = 0; i < 4; i++) {
            if (cse->durations[i] != 0) {
                value = StatusEffect_applyStat(cse->activeIDs[i], stat, value);
            }
        }
    }

    return value;
}

bool Entity_heal(int id, int amount) {
    CActor* actor = _componentGet(id, CActor_id);
    if (!actor) return false;
    if (actor->hp == actor->maxhp) return false;
    actor->hp = SDL_min(actor->maxhp, actor->hp + amount);
    return true;
}

int Entity_getMaxHP(int id) {
    CActor* actor = _componentGet(id, CActor_id);
    if (!actor) return 0;
    return actor->maxhp;
}

int Entity_getHP(int id) {
    CActor* actor = _componentGet(id, CActor_id);
    if (!actor) return 0;
    return actor->hp;
}

void Entity_setHP(int id, int hp) {
    CActor* actor = _componentGet(id, CActor_id);
    if (!actor) return;
    actor->hp = hp;
    // todo: doesnt handle 0 properly
}

StatusEffectID Entity_getStatusEffect(int id, int i) {
    CStatusEffects* cse = _componentGet(id, CStatusEffects_id);
    if (!cse) return -1;
    if (i < 0 || i > 4) return -1;
    if (cse->durations[i] == 0) return -1;
    return cse->activeIDs[i];
}

int Entity_getX(int id) {
    CLocation* loc = _componentGet(id, CLocation_id);
    if (!loc) return 0;
    return loc->x;
}

int Entity_getY(int id) {
    CLocation* loc = _componentGet(id, CLocation_id);
    if (!loc) return 0;
    return loc->y;
}

EntityState Entity_getState(int id) {
    if (_componentGet(id, CStateAttack_id)) {
        return EntityState_attack;
    } else if (_componentGet(id, CStateHurt_id)) {
        return EntityState_hurt;
    } else if (_componentGet(id, CStateRolling_id)) {
        return EntityState_rolling;
    } else if (_componentGet(id, CStateAlert_id)) {
        return EntityState_alert;
    } else {
        return EntityState_ready;
    }
}

VillagerID Entity_getVillager(int id) {
    CVillager* cv = _componentGet(id, CVillager_id);
    if (!cv) return -1;
    return cv->villager;
}

void Entity_setVillager(int id, VillagerID villagerID) {
    CVillager* cv = _componentGetOrAttach(id, CVillager_id);
    if (!cv) return;
    cv->villager = villagerID;
}

void Entity_applyMotion(int id, int mx, int my) {
    CMotion* cm = _componentGetOrAttach(id, CMotion_id);
    if (cm) {
        cm->movex += mx;
        cm->movey += my;
    }
}

void Entity_setAnimState(int id, AnimState astate) {
    CAnimation* ca = _componentGet(id, CAnimation_id);
    if (ca) {
        ca->animState = astate;
    }
}

// todo: should facing perhaps be separate from CAnimation?
Facing Entity_getFacing(int id) {
    CAnimation* ca = _componentGet(id, CAnimation_id);
    if (ca) {
        return ca->facing;
    }
    return Facing_right;
}

void Entity_setFacing(int id, Facing facing) {
    CAnimation* ca = _componentGet(id, CAnimation_id);
    if (ca) {
        ca->facing = facing;
    }
}

int Entity_getCurrentAnim(int id) {
    CAnimation* ca = _componentGet(id, CAnimation_id);
    if (!ca) return 0;

    // choose animation
    AnimationID anim = ca->animIdle;
    if (ca->animState == AnimState_walking && ca->animWalk[ca->facing] != -1) {
        anim = ca->animWalk[ca->facing];
    }
    if (ca->animState == AnimState_attack && ca->animAttack[ca->facing] != -1) {
        anim = ca->animAttack[ca->facing];
    }
    if (ca->animState == AnimState_hurt && ca->animHurt[ca->facing] != -1) {
        anim = ca->animHurt[ca->facing];
    }
    if (ca->animState == AnimState_roll && ca->animRoll[ca->facing] != -1) {
        anim = ca->animRoll[ca->facing];
    }
    if (ca->animState == AnimState_alert && ca->animAlert[ca->facing] != -1) {
        anim = ca->animAlert[ca->facing];
    }

    return anim;
}

void Entity_setPath(int id, int targetX, int targetY, int speed) {
    CPathfinding* pathfinding = _componentGetOrAttach(id, CPathfinding_id);
    CLocation* location = _componentGet(id, CLocation_id);
    if (!pathfinding || !location) return;

    pathfinding->targetX = targetX;
    pathfinding->targetY = targetY;
    // todo: ideally findPath shouldn't return first elem
    // setting progress to 1 is just a hack to bypass getting stuck
    pathfinding->progress = 1;
    pathfinding->speed = speed;

    pathfinding->length = NavGrid_findPath(
        location->x / 256, location->y / 256,
        targetX / 256, targetY / 256,
        MAX_PATHFINDING_LENGTH, pathfinding->xs, pathfinding->ys);

    if (pathfinding->length > MAX_PATHFINDING_LENGTH) {
        pathfinding->length = MAX_PATHFINDING_LENGTH;
        pathfinding->incomplete = true;
    }
}

void Entity_setGodMode(int id, int enable) {
    CCheats* cheats = _componentGetOrAttach(id, CCheats_id);
    if (cheats) {
        if (enable == 2) cheats->god = !cheats->god;
        else cheats->god = enable != 0;
    }
}

void Entity_setNoclipMode(int id, int enable) {
    CCheats* cheats = _componentGetOrAttach(id, CCheats_id);
    if (cheats) {
        if (enable == 2) cheats->noclip = !cheats->noclip;
        else cheats->noclip = enable != 0;
    }
}

void Entity_setMiniInvItems(int id, ItemID* items) {
    CMiniInventory* inv = _componentGetOrAttach(id, CMiniInventory_id);
    if (inv) {
        memcpy(inv->items, items, sizeof(inv->items));
    }
}

void Entity_setMiniInvQty(int id, int* qty) {
    CMiniInventory* inv = _componentGetOrAttach(id, CMiniInventory_id);
    if (inv) {
        memcpy(inv->quantity, qty, sizeof(inv->quantity));
    }
}

ItemID* Entity_getMiniInvItems(int id) {
    CMiniInventory* inv = _componentGetOrAttach(id, CMiniInventory_id);
    if (inv) {
        return inv->items;
    } else {
        return NULL;
    }
}

int* Entity_getMiniInvQty(int id) {
    CMiniInventory* inv = _componentGetOrAttach(id, CMiniInventory_id);
    if (inv) {
        return inv->quantity;
    } else {
        return NULL;
    }
}

void Entity_shake(int id, int times) {
    CShake* shake = _componentAttach(id, CShake_id);
    shake->timer = times * 8;
}

void Entity_setHintText(int id, const char* text) {
    CHint* hint = _componentGetOrAttach(id, CHint_id);
    strncpy(hint->text, text, 128);
}

void Entity_setHintRadius(int id, int radius) {
    CHint* hint = _componentGetOrAttach(id, CHint_id);
    hint->radius = radius;
}

void Entity_drawBossHP(void) {
    QUERY_ID(i);
    QUERY_COMPONENT(CBoss, boss);
    QUERY_COMPONENT(CActor, actor);
    _queryBegin();
    while (_queryNext()) {
        if (!boss->active) continue;
        SpriteID spr_frame[3];
        spr_frame[0] = Sprite_find("ui/bhp_frame_cap");
        spr_frame[1] = Sprite_find("ui/bhp_frame_mid");
        spr_frame[2] = Sprite_find("ui/bhp_frame_capr");
        SpriteID spr_fill[3];
        spr_fill[0] = Sprite_find("ui/bhp_fill_cap");
        spr_fill[1] = Sprite_find("ui/bhp_fill_mid");
        spr_fill[2] = Sprite_find("ui/bhp_fill_capr");
        int x = 0, y = 0;
        int width = actor->hp * 92 / 100;
        x = SCREEN_WIDTH/2 - 96/2;
        y = 16;
        Sprite_draw(spr_frame[0], x, y);
        Sprite_drawScaled(spr_frame[1], x+16, y, 16*4, 16);
        Sprite_draw(spr_frame[2], x + 16*5, y);
        if (width > 3) Sprite_drawScaled(spr_fill[1], x+4, y, width-2, 16);
        if (width > 90) Sprite_draw(spr_fill[2], x + 16*5 + 16-4, y);
        if (width > 2) Sprite_draw(spr_fill[0], x, y);
        // todo: draw The Slime King text also
        // some kind of frame for The Slime King text also?
        // BFont_drawTextExt(BFont_find("dialog"), SCREEN_WIDTH/2-50,8,100,0.5f,
        //     "Slime King");
        Sprite_draw(Sprite_find("ui/bhp_title"), x + 19, y - 11);
    }
    _queryEnd();
}

void StatusEffect_load(void) {
    Ini_readAsset("statusfx.ini");

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_statusEffectCount == MAX_STATUS_EFFECTS) {
            Log_error("Max status effects exceeded");
            Ini_clear();
            return;
        }

        StatusEffect* statusEffect = &_statusEffects[_statusEffectCount++];
        strncpy(statusEffect->name, name, NAME_LENGTH);

        // todo: handle different types

        statusEffect->stat = String_parseEnum(Ini_get(name, "stat"),
            "atk;def;mag;res;lck", 0);
        statusEffect->change = String_parseInt(Ini_get(name, "change"), 0);
        statusEffect->icon = Sprite_find(Ini_get(name, "icon"));
    }

    Ini_clear();
}

StatusEffectID StatusEffect_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _statusEffectCount; i++) {
        if (strcmp(_statusEffects[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find status effect %s", name);
    return -1;
}

int StatusEffect_applyStat(StatusEffectID seID, Stat stat, int value) {
    if (seID < 0 || seID > _statusEffectCount) return value;
    StatusEffect* statusEffect = &_statusEffects[seID];

    if (statusEffect->stat == stat) {
        return value + statusEffect->change;
    } else {
        return value;
    }
}

SpriteID StatusEffect_getIcon(StatusEffectID seID) {
    if (seID < 0 || seID > _statusEffectCount) return -1;
    return _statusEffects[seID].icon;
}

static void _playerUpdate(int i) {
    CIntent* intent = _componentGet(i, CIntent_id);
    if (!intent) return;
    float x = Input_getMoveX();
    float y = Input_getMoveY();
    float absx = fabsf(x), absy = fabsf(y);

    intent->intent = EntityIntent_none;
    intent->intentx = x;
    intent->intenty = y;

    if (Input_isPressed(InputButton_attack)) {
        intent->intent = EntityIntent_attack;
        intent->kind = 0;
    } else if (Input_isPressed(InputButton_skill1)) {
        intent->intent = EntityIntent_attack;
        intent->kind = 1;
    } else if (Input_isPressed(InputButton_skill2)) {
        intent->intent = EntityIntent_attack;
        intent->kind = 2;
    } else if (Input_isPressed(InputButton_skill3)) {
        intent->intent = EntityIntent_attack;
        intent->kind = 3;
    } else if (Input_isPressed(InputButton_interact)) {
        intent->intent = EntityIntent_interact;
        intent->intentPartner = _playerInteractionPartner;
    } else if (Input_isPressed(InputButton_roll)) {
        Math_normalizeXY(&intent->intentx, &intent->intenty);
        intent->intent = EntityIntent_roll;
    } else if (absx > 0.01f || absy > 0.01f) {
        intent->intent = EntityIntent_walk;
    }
}

static void _enemyUpdate(int i) {
    EntityState entState = Entity_getState(i);
    CActor* actor = _componentGet(i, CActor_id);
    CEnemyController* cec = _componentGet(i, CEnemyController_id);
    CIntent* intent = _componentGet(i, CIntent_id);
    CLocation* loc = _componentGet(i, CLocation_id);
    if (!actor || !cec || !intent || !loc) return;

    // todo: should alert logic be elsewhere?
    if (cec->alertRadius > 0 && cec->target == 0 &&
        entState == EntityState_ready
    ) {
        // todo: should check based on faction?
        int playerID = Entity_getPlayer();
        CLocation* playerLoc = _componentGet(playerID, CLocation_id);
        if (playerLoc) {
            int distSq = _entDistSq(loc, playerLoc);
            if (distSq <= cec->alertRadius*cec->alertRadius) {
                cec->target = playerID;
                _changeState(i, EntityState_alert);
            }
        }
    }

    // if nothing else, idle
    intent->intent = EntityIntent_none;
    intent->intentx = 0;
    intent->intenty = 0;

    if (cec->target) {
        // simple ai - move towards target if far, otherwise attack
        CLocation* otherLoc = _componentGet(cec->target, CLocation_id);
        if (!otherLoc) {
            cec->target = 0;
            return;
        }
        int distSq = _entDistSq(loc, otherLoc);
        int atkRadius = 0;
        if (actor->defaultAttack != -1) {
            Attack* atk = &_attacks[actor->defaultAttack];
            atkRadius = atk->range;
        }
        if (distSq <= atkRadius * atkRadius) {
            if (entState != EntityState_attack) {
                intent->intent = EntityIntent_attack;
            }
        } else {
            intent->intent = EntityIntent_walk;
        }
        if (distSq > 240*16*240*16) {
            cec->target = 0;
            _componentDetach(i, CPathfinding_id);
        } else if (cec->pathfinding && intent->intent == EntityIntent_walk &&
            distSq > 768*768) {
            Entity_setPath(i, otherLoc->x, otherLoc->y, -1);
        } else {
            intent->intentx = otherLoc->x - loc->x;
            intent->intenty = otherLoc->y - loc->y;
            Math_normalizeXY(&intent->intentx, &intent->intenty);
        }
    }
}

static void* _changeState(int i, EntityState newState) {
    // leave old state
    _componentDetach(i, CStateAttack_id);
    _componentDetach(i, CStateHurt_id);
    _componentDetach(i, CStateRolling_id);
    _componentDetach(i, CStateAlert_id);

    // enter new state
    void* component = NULL;
    CStateParams* csp = _componentGet(i, CStateParams_id);
    if (newState == EntityState_ready) {
        Entity_setAnimState(i, AnimState_idle);
    } else if (newState == EntityState_attack) {
        Entity_setAnimState(i, AnimState_attack);
        CStateAttack* csa = _componentAttach(i, CStateAttack_id);
        csa->attackTime = 0;
        csa->attackID = -1;
        csa->attackQueued = -1;
        component = csa;
    } else if (newState == EntityState_hurt) {
        Entity_setAnimState(i, AnimState_hurt);
        CStateHurt* csh = _componentAttach(i, CStateHurt_id);
        csh->hurtTime = 0;
        if (csp) csh->hurtDuration = csp->hurtDuration;
        component = csh;
    } else if (newState == EntityState_rolling) {
        Entity_setAnimState(i, AnimState_roll);
        CStateRolling* csr = _componentAttach(i, CStateRolling_id);
        csr->rollTime = 0;
        if (csp) {
            csr->rollDuration = csp->rollDuration;
            csr->rollSpeed = csp->rollSpeed;
        }
        component = csr;

        // todo: find better place for this?
        CLocation* location = _componentGet(i, CLocation_id);
        Particles_spawn(Particles_find("pt_dirtRoll"),
            location->x, location->y, 2);
    } else if (newState == EntityState_alert) {
        Entity_setAnimState(i, AnimState_alert);
        CStateAlert* csa = _componentAttach(i, CStateAlert_id);
        csa->alertTime = 0;
        component = csa;
    }

    // update state
    return component;
}

static int _findInteractionPartner(int entityID) {
    CLocation* loc = _componentGet(entityID, CLocation_id);
    if (!loc) return -1;
    QUERY_ID(j);
    QUERY_COMPONENT(CLocation, otherLoc);
    QUERY_COMPONENT(CInteraction, ci);
    _queryBegin();
    while (_queryNext()) {
        if (ci->type == InteractionType_none) continue;
        int distanceSq = _entDistSq(loc, otherLoc);
        if (distanceSq > 256 * 256) continue;

        _queryEnd();
        return j;
    }
    _queryEnd();

    return -1;
}

static bool _isInvincible(int i) {
    CCheats* cheats = _componentGet(i, CCheats_id);
    return _componentGet(i, CStateRolling_id) != NULL ||
        (cheats && cheats->god);
}

static void _playerCollect(int playerID, int collectibleID) {
    CBeingCollected* cbc = _componentAttach(collectibleID, CBeingCollected_id);
    cbc->collector = playerID;
    cbc->collectTime = 0;
    _componentDetach(collectibleID, CSolid_id);
    CSprite* cs = _componentGet(collectibleID, CSprite_id);
    if (cs) {
        cs->spritey -= 4;
        cs->spritez -= 4;
    }
}

static int _entDistSq(CLocation* a, CLocation* b) {
    return Math_lengthSquared(a->x - b->x, a->y - b->y);
}

static const char* _getDebugName(int i) {
    CDebugLabel* debugLabel = _componentGet(i, CDebugLabel_id);
    return debugLabel ? debugLabel->name : "<unnamed>";
}

//  ecs stuff
typedef int EcsComponent;
typedef int EcsEntity;
static EcsComponent _componentRegister(int width) {
    // todo: check array limit
    EcsComponent id = _ecsNextComponent++;
    ComponentData* component = &_ecsComponents[id];

    component->width = width;
    component->pool = calloc(ECS_MAX_ENTITIES, width);
    component->valid = calloc(ECS_MAX_ENTITIES, sizeof(bool));
    return id;
}

static EcsEntity _entityCreate(void) {
    for (int i = 0; i < ECS_MAX_ENTITIES; i++) {
        if (!_ecsEntityValid[i]) {
            int generation = ++_ecsEntityGeneration[i];
            if ((generation & 0xffff) == 0) {
                generation = ++_ecsEntityGeneration[i];
            }
            _ecsEntityValid[i] = true;
            return (generation << 16) | i;
        }
    }
    return 0;
}

static void _entityDestroy(EcsEntity e) {
    if (_entityValid(e)) {
        for (int i = 0; i < _ecsNextComponent; i++) {
            _componentDetach(e, i);
        }
        _ecsEntityValid[e & 0xffff] = false;
    }
}

static bool _entityValid(EcsEntity e) {
    int index = e & 0xffff;
    return e &&
        _ecsEntityValid[index] &&
        _ecsEntityGeneration[index] == e >> 16;
}

static void* _componentAttach(EcsEntity e, EcsComponent c) {
    if (_entityValid(e)) { // todo check component valid
        ComponentData* component = &_ecsComponents[c];
        int index = e & 0xffff;
        void* data = &((unsigned char*) component->pool)[index * component->width];
        memset(data, 0, component->width);
        component->valid[index] = true;
        return data;
    }
    return NULL;
}

static void _componentDetach(EcsEntity e, EcsComponent c) {
    if (_entityValid(e)) { // todo check component valid
        ComponentData* component = &_ecsComponents[c];
        int index = e & 0xffff;
        component->valid[index] = false;
    }
}

static void* _componentGet(EcsEntity e, EcsComponent c) {
    if (_entityValid(e)) { // todo check component valid
        ComponentData* component = &_ecsComponents[c];
        int index = e & 0xffff;
        if (component->valid[index]) {
            return &((unsigned char*) component->pool)[index * component->width];
        } else {
            return NULL;
        }
    }
    return NULL;
}

static void* _componentGetOrAttach(EcsEntity e, EcsComponent c) {
    if (_entityValid(e)) { // todo check component valid
        ComponentData* component = &_ecsComponents[c];
        int index = e & 0xffff;
        void* data = &((unsigned char*) component->pool)[index * component->width];
        if (!component->valid[index]) {
            memset(data, 0, component->width);
            component->valid[index] = true;
        }
        return data;
    }
    return NULL;
}

static void _queryAddColumn(EcsComponent c, void** s) {
    if (_ecsNextQuery >= ECS_MAX_QUERIES) {
        Log_error("max queries exceeded");
        return;
    }

    QueryData* query = &_ecsQueries[_ecsNextQuery];
    if (query->columnCount >= 4) {
        Log_error("max columns exceeded");
        abort();
    }
    // todo: check colymnCount
    query->columnTypes[query->columnCount] = c;
    query->columnStorage[query->columnCount] = s;
    query->columnCount++;
}

static void _queryAddId(EcsEntity* s) {
    if (_ecsNextQuery >= ECS_MAX_QUERIES) {
        Log_error("max queries exceeded");
        return;
    }

    QueryData* query = &_ecsQueries[_ecsNextQuery];
    query->idStorage = s;
}

static void _queryBegin(void) {
    if (_ecsNextQuery >= ECS_MAX_QUERIES) {
        Log_error("max queries exceeded");
        return;
    }

    QueryData* query = &_ecsQueries[_ecsNextQuery];
    if (query->columnCount == 0) {
        Log_warn("invalid query of 0 components");
    }
    _ecsNextQuery++;

    query->rows = calloc(ECS_MAX_ENTITIES, sizeof(EcsEntity));

    query->rowCount = 0;
    ComponentData* component = &_ecsComponents[query->columnTypes[0]];
    for (int i = 0; i < ECS_MAX_ENTITIES; i++) {
        if (component->valid[i]) {
            EcsEntity e = _ecsEntityGeneration[i] << 16 | i;
            query->rows[query->rowCount++] = e;
        }
    }

    for (int i = 1; i < query->columnCount; i++) {
        component = &_ecsComponents[query->columnTypes[i]];
        int begin = 0;
        // todo: consider swapping loop order
        //       component as inner loop might be faster?
        while (begin < query->rowCount) {
            EcsEntity e = query->rows[begin];
            if (component->valid[e & 0xffff]) {
                begin++;
            } else {
                query->rows[begin] = query->rows[--query->rowCount];
            }
        }
    }
}

static bool _queryNext(void) {
    // todo: check ecsNextQuery
    QueryData* query = &_ecsQueries[_ecsNextQuery - 1];
    if (query->index < query->rowCount) {
        EcsEntity e = query->rows[query->index];
        if (query->idStorage) *query->idStorage = e;
        for (int i = 0; i < query->columnCount; i++) {
            EcsComponent c = query->columnTypes[i];
            ComponentData* component = &_ecsComponents[c];
            int index = e & 0xffff;
            *query->columnStorage[i] =
                &((unsigned char*) component->pool)[index * component->width];
        }
        query->index++;
        return true;
    } else {
        return false;
    }
}

static void _queryEnd(void) {
    if (_ecsNextQuery <= 0) {
        Log_error("invalid _queryEnd");
        return;
    }
    free(_ecsQueries[_ecsNextQuery-1].rows);
    memset(&_ecsQueries[--_ecsNextQuery], 0, sizeof(QueryData));
}

static void _ecsTest(void) {
    // ... ECS TESTING ... //
    EcsComponent c1 = _componentRegister(sizeof(int));
    assert(c1 == 0);
    struct V2 { int x, y; };
    EcsComponent c2 = _componentRegister(sizeof(struct V2));
    assert(c2 == 1);
    // todo: test zero width component also? maybe make c1 zero width?

    EcsEntity e1 = _entityCreate();
    Log_debug("e1 = %08x", e1);
    assert(_entityValid(e1));
    EcsEntity e2 = _entityCreate();
    Log_debug("e2 = %08x", e2);
    assert(_entityValid(e2));
    _entityDestroy(e1);
    assert(!_entityValid(e1));
    EcsEntity e3 = _entityCreate();
    Log_debug("e3 = %08x", e3);
    assert(_entityValid(e3));
    assert(!_entityValid(e1));

    assert(!_componentGet(e1, c1));
    assert(!_componentGet(e1, c2));
    assert(!_componentGet(e2, c1));
    assert(!_componentGet(e2, c2));
    assert(!_componentGet(e3, c1));
    assert(!_componentGet(e3, c2));

    _componentAttach(e1, c1);

    assert(!_componentGet(e1, c1));
    assert(!_componentGet(e1, c2));
    assert(!_componentGet(e2, c1));
    assert(!_componentGet(e2, c2));
    assert(!_componentGet(e3, c1));
    assert(!_componentGet(e3, c2));

    int* e2_c1 = _componentAttach(e2, c1);
    assert(e2_c1);
    struct V2* e3_c2 = _componentAttach(e3, c2);
    assert(e3_c2);
    *e2_c1 = 32;
    *e3_c2 = (struct V2) { 12, 24 };

    assert(!_componentGet(e1, c1));
    assert(!_componentGet(e1, c2));
    assert(_componentGet(e2, c1) == e2_c1);
    assert(!_componentGet(e2, c2));
    assert(!_componentGet(e3, c1));
    assert(_componentGet(e3, c2) == e3_c2);

    assert(*(int*)_componentGet(e2, c1) == 32);
    assert(((struct V2*)_componentGet(e3, c2))->x == 12);
    assert(((struct V2*)_componentGet(e3, c2))->y == 24);

    struct V2* e2_c2 = _componentAttach(e2, c2);
    Log_debug("%08x -- %08x", e3_c2, e2_c2);
    assert(e2_c2);
    assert(e2_c2 != e3_c2);
    *e2_c2 = (struct V2) { 13, 25 };

    assert(_componentGet(e2, c2) == e2_c2);
    assert(((struct V2*)_componentGet(e3, c2))->x == 12);
    assert(((struct V2*)_componentGet(e3, c2))->y == 24);
    assert(((struct V2*)_componentGet(e2, c2))->x == 13);
    assert(((struct V2*)_componentGet(e2, c2))->y == 25);

    struct V2* p_c2;
    EcsEntity ent;
    _queryAddColumn(c2, (void**) &p_c2);
    _queryAddId(&ent);
    _queryBegin();
    int counter = 0;
    while (_queryNext()) {
        Log_debug("q0[%d] (ent %08x) { %d, %d }", counter++,
            ent, p_c2->x, p_c2->y);
    }
    _queryEnd();

    int* p_c1;
    _queryAddColumn(c2, (void**) &p_c2);
    _queryAddColumn(c1, (void**) &p_c1);
    _queryBegin();
    counter = 0;
    while (_queryNext()) {
        Log_debug("q1[%d] (c1 %d) { %d, %d }", counter++,
            *p_c1, p_c2->x, p_c2->y);
    }
    _queryEnd();

    _componentDetach(e2, c2);

    assert(!_componentGet(e1, c1));
    assert(!_componentGet(e1, c2));
    assert(_componentGet(e2, c1));
    assert(!_componentGet(e2, c2));
    assert(!_componentGet(e3, c1));
    assert(_componentGet(e3, c2));

    _componentAttach(e2, c2);
    assert(((struct V2*)_componentGet(e2, c2))->x == 0);
    assert(((struct V2*)_componentGet(e2, c2))->y == 0);

    // ... END TESTING ... //
}

void _ecsDump(EcsEntity e) {
    Log_debug("Entity %d (id %d, gen %d)", e, e&0xffff, e>>16);
    for (int i = 0; i < _ecsNextComponent; i++) {
        if (_componentGet(e, i)) {
            Log_debug("  has component %d", i);
        }
    }
}
