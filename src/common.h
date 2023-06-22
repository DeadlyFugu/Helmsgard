#pragma once

// ===== [[ Common Includes ]] =====

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <SDL.h>
//#include <SDL_ttf.h>
//#include "zz_sdlfc.h"

// ===== [[ Defines ]] =====

#define FAERJOLD_VERSION "23.06"
#define SCREEN_WIDTH 426
#define SCREEN_HEIGHT 240

#define DEG_TO_RAD(f) ((f) * 0.0174533f)
#define RAD_TO_DEG(f) ((f) * 57.2958f)
#define Math_sin(f) (sinf(DEG_TO_RAD(f)))
#define Math_cos(f) (cosf(DEG_TO_RAD(f)))
#define Math_tan(f) (cosf(DEG_TO_RAD(f)))
#define Math_asin(f) (RAD_TO_DEG(asinf(f)))
#define Math_acos(f) (RAD_TO_DEG(acosf(f)))
#define Math_atan(f) (RAD_TO_DEG(atanf(f)))
#define Math_atan2(y, x) (RAD_TO_DEG(atan2f((y), (x))))

#define NAME_LENGTH 32
#define MAX_ASSETPATH_LENGTH 64
#define MAX_ENTITIES 256

#define REGION_WIDTH 128
#define REGION_HEIGHT 128

#define LOAD_END() \
        (LoadingRequest) { .type = 0 }
#define LOAD_TEXTURE(_out, _name) \
        (LoadingRequest) { .type = 1, .out = &_out, .name = _name }
#define LOAD_FONT(_out, _name, _size) \
        (LoadingRequest) { .type = 2, .out = &_out, .name = _name, .param = _size }
#define LOAD_REGION(_id) \
        (LoadingRequest) { .type = 3, .param = _id }
#define LOAD_SYSTEM(_id) \
        (LoadingRequest) { .type = 4, .param = _id }
#define LOAD_DUMMY() \
        (LoadingRequest) { .type = 5 }
#define LOAD_SOUND(_out, _name) \
        (LoadingRequest) { .type = 6, .out = &_out, .name = _name }
#define LOAD_MUSIC(_out, _name) \
        (LoadingRequest) { .type = 7, .out = &_out, .name = _name }
#define LOAD_SPRITE_IMAGE(_name) \
        (LoadingRequest) { .type = 8, .name = _name }

#define countof(_array) (sizeof(_array) / sizeof(_array[0]))

// ===== [[ Handle Types ]] =====

typedef int AnimationID;
typedef int AttackID;
typedef int BFontID;
typedef int DialogID;
typedef int DialogLineID;
typedef int ItemID;
typedef int ItemTypeID;
typedef int LootTableID;
typedef int MusicID;
typedef int ParticlesID;
typedef int QuestID;
typedef int RecipeID;
typedef int SlotID;
typedef int SoundID;
typedef int SpriteID;
typedef int StatusEffectID;
typedef int VillagerID;
typedef struct archive* Archive;

// ===== [[ Enum Types ]] =====

typedef enum {
    AnimState_idle,
    AnimState_walking,
    AnimState_attack,
    AnimState_hurt,
    AnimState_roll,
    AnimState_alert
} AnimState;

typedef enum {
    EntityController_none,
    EntityController_player,
    EntityController_enemy,
    EntityController_boss
} EntityController;

typedef enum {
    EntityIntent_none,
    EntityIntent_walk,
    EntityIntent_attack,
    EntityIntent_interact,
    EntityIntent_roll
} EntityIntent;

typedef enum {
    EntityState_ready, // idle, walking
    EntityState_attack,
    EntityState_hurt,
    EntityState_lifted,
    EntityState_rolling,
    EntityState_alert
} EntityState;

typedef enum {
    EquipType_none,
    EquipType_weapon,
    EquipType_head,
    EquipType_body,
    EquipType_accessory
} EquipType;

typedef enum {
    Facing_left,
    Facing_right,
    Facing_up,
    Facing_down
} Facing;

typedef enum {
    InputButton_accept, // A
    InputButton_back, // B
    InputButton_interact, // B
    InputButton_roll, // A
    InputButton_inventory, // Y
    InputButton_attack, // X
    InputButton_pause, // Start
    InputButton_journal, // Select
    InputButton_skill1, // Dpad Left
    InputButton_skill2, // Dpad Up
    InputButton_skill3, // Dpad Right
    InputButton_hotbar, // Dpad Down
    InputButton_left,
    InputButton_right,
    InputButton_up,
    InputButton_down,

    InputButton_COUNT
} InputButton;

typedef enum {
    InteractionType_none,
    InteractionType_debug,
    InteractionType_lift,
    InteractionType_talk,
    InteractionType_craft,
    InteractionType_chest,
    InteractionType_harvest,
    InteractionType_shop
} InteractionType;

typedef enum {
    ItemCategory_weapon,
    ItemCategory_armor,
    ItemCategory_food,
    ItemCategory_ingredient,
    ItemCategory_potion,
    ItemCategory_misc
} ItemCategory;

typedef enum {
    InputSource_keyboard,
    InputSource_controller
} InputSource;

typedef enum {
    MainState_invalid,
    MainState_loading,
    MainState_title,
    MainState_menu,
    MainState_game,
    MainState_editor
} MainState;

typedef enum {
    QuestEvent_talk,
    QuestEvent_defeat,
    QuestEvent_collect,
    QuestEvent_craft,
    QuestEvent_defeatBoss
} QuestEvent;

typedef enum {
    Stat_atk,
    Stat_def,
    Stat_mag,
    Stat_res,
    Stat_lck
} Stat;

// ===== [[ Struct Types ]] =====

typedef struct {
    int x;
    int y;
} FieldPoint;

typedef struct {
    int x;
    int y;
    int angle;
    int distanceSquared;
    bool isInside;
} FieldNearest;

typedef struct {
    bool invert;
    int operator; // 0: AND, 1: OR, 2: XOR
} RegionLogicParams;

// ===== [[ Declarations ]] =====

void Animation_loadFrom(const char* assetpath);
AnimationID Animation_find(const char* name);
int Animation_getDuration(AnimationID self);
// draw animation at given spot
// if time is NULL, will use global timer
// else will use time stored and update it
void Animation_draw(AnimationID self, int x, int y, int* timePointer);
void Animation_update(void);

void Attack_loadFrom(const char* assetpath);
int Attack_find(const char* name);

void Audio_startup(void);

void BFont_load(void);
BFontID BFont_find(const char* name);
void BFont_drawText(BFontID font, int x, int y, const char* string, ...);
void BFont_drawTextExt(BFontID font, int x, int y,
        int maxWidth, float halign,
        const char* string, ...);

extern const SDL_Color Color_aqua;
extern const SDL_Color Color_black;
extern const SDL_Color Color_blue;
extern const SDL_Color Color_dkgray;
extern const SDL_Color Color_fuchsia;
extern const SDL_Color Color_gray;
extern const SDL_Color Color_green;
extern const SDL_Color Color_lime;
extern const SDL_Color Color_ltgray;
extern const SDL_Color Color_maroon;
extern const SDL_Color Color_navy;
extern const SDL_Color Color_olive;
extern const SDL_Color Color_orange;
extern const SDL_Color Color_purple;
extern const SDL_Color Color_red;
extern const SDL_Color Color_silver;
extern const SDL_Color Color_teal;
extern const SDL_Color Color_white;
extern const SDL_Color Color_yellow;

void Command_execute(const char* command);

void Console_enter(void);
void Console_leave(void);
void Console_update(void);
void Console_render(void);
bool Console_shouldClose(void);

extern int Config_displayMode;
extern char Config_assetDirectory[256];
extern bool Config_muteMusic;
extern bool Config_muteSounds;
extern bool Config_disableAudio;
extern bool Config_showPerf;
extern bool Config_showCollision;
extern bool Config_showNavGrid;
extern float Config_deadzoneX;
extern float Config_deadzoneY;
extern int Config_logLevel;
void Config_load(void);

// todo: maybe dont expose triggers, only expose DialogLine?
void Dialog_load(void);
DialogID Dialog_find(const char* name);
DialogLineID Dialog_findLine(const char* name);
DialogID Dialog_findCandidate(VillagerID villager);
DialogLineID Dialog_firstLine(DialogID dialog);
const char* Dialog_getMessage(DialogLineID dialogLine);
bool Dialog_getHideName(DialogLineID dialogLine);
DialogLineID Dialog_getNext(DialogLineID dialogLine, int choice);
int Dialog_getChoiceCount(DialogLineID dialogLine);
const char* Dialog_getChoice(DialogLineID dialogLine, int choice);
void Dialog_execute(DialogLineID dialogLine);
int Dialog_getDisplayIcon(DialogID dialog);

//void Draw_setFont(FC_Font* font);
void Draw_setColor(SDL_Color color);
void Draw_setAlpha(float alpha);
void Draw_setTranslate(int x, int y);
void Draw_setClip(int x, int y, int w, int h);
void Draw_clearClip(void);
void Draw_translatePoint(int* x, int* y);
void Draw_point(int x, int y);
void Draw_line(int x1, int y1, int x2, int y2);
void Draw_rect(int x, int y, int w, int h);
void Draw_rectOutline(int x, int y, int w, int h);
void Draw_circle(int x, int y, int radius);
void Draw_text(int x, int y, const char* string, ...);
void Draw_textAligned(int x, int y, float halign, float valign,
        const char* string, ...);
int Draw_getTextWidth(const char* string, ...);
int Draw_getTextHeight(const char* string, ...);

void Editor_init(void);
void Editor_enter(void);
void Editor_leave(void);
void Editor_update(void);
void Editor_render(void);
void Editor_setRegion(int regionID);

//extern Entity Entity_table[256];
void Entity_loadPrefabsFrom(const char* assetpath);
void Entity_updateAll(void);
void Entity_renderAll(void);
int Entity_spawn(int x, int y, int prefabID);
void Entity_destroy(int id);
void Entity_destroyAll(void);
int Entity_getPlayer(void);
int Entity_findPrefab(const char* name);
void Entity_dropItem(int x, int y, ItemID item);
void Entity_dropGold(int x, int y, int amount);
void Entity_addStatusEffect(int id, StatusEffectID seID);
int Entity_getStat(int id, Stat stat);
bool Entity_heal(int id, int amount);
int Entity_getMaxHP(int id);
int Entity_getHP(int id);
void Entity_setHP(int id, int hp);
StatusEffectID Entity_getStatusEffect(int id, int i);
int Entity_getX(int id);
int Entity_getY(int id);
EntityState Entity_getState(int id);
VillagerID Entity_getVillager(int id);
void Entity_setVillager(int id, VillagerID villagerID);
void Entity_applyMotion(int id, int mx, int my);
void Entity_setAnimState(int id, AnimState astate);
Facing Entity_getFacing(int id);
void Entity_setFacing(int id, Facing facing);
AnimationID Entity_getCurrentAnim(int id);
void Entity_setPath(int id, int targetX, int targetY, int speed);
void Entity_setGodMode(int id, int enable); // 2 - toggle
void Entity_setNoclipMode(int id, int enable); // 2 - toggle
void Entity_setMiniInvItems(int id, ItemID* items);
void Entity_setMiniInvQty(int id, int* qty);
ItemID* Entity_getMiniInvItems(int id);
int* Entity_getMiniInvQty(int id);
void Entity_shake(int id, int times);
void Entity_setHintText(int id, const char* text);
void Entity_setHintRadius(int id, int radius);
void Entity_drawBossHP(void);

void Field_clear(void);
void Field_addRect(int x, int y, int w, int h);
void Field_addPolygon(int count, FieldPoint* points, bool ccw);
FieldNearest Field_findNearest(int x, int y);
void Field_drawDebug(void);
void Field_drawDebugCollision(int x, int y);

void Game_init(void);
void Game_enter(void);
void Game_leave(void);
void Game_update(void);
void Game_render(void);
void Game_reload(void);
void Game_setDialog(int entity);
void Game_setCrafting(void);
void Game_setHitstop(int frames);
bool Game_shouldAnimate(void);
void Game_setChest(int entity);
void Game_setShop(void);
void Game_addGrass(int x, int y, int side); // todo: separate World module?
void Game_setDefeat(void);
void Game_setWon(void);
void Game_setHintText(const char* text);
void Game_addGold(int amount);

void Graphics_setModulationColor(int r, int g, int b);
void Graphics_clearModulationColor(void);

void Ini_clear(void);
bool Ini_readFile(const char* filepath);
bool Ini_readAsset(const char* assetpath);
bool Ini_writeFile(const char* filepath);
const char* Ini_getSectionName(int index); // used for looping
const char* Ini_get(const char* section, const char* key);
bool Ini_set(const char* section, const char* key, const char* value);

void Input_update(void);
void Input_processEvent(SDL_Event* event);

bool Input_isPressed(InputButton button);
bool Input_isReleased(InputButton button);
bool Input_isDown(InputButton button);
float Input_getDuration(InputButton button);
void Input_clear(InputButton button);
void Input_clearAll(void);
float Input_getMoveX(void);
float Input_getMoveY(void);
InputSource Input_getInputSource(void);
bool Input_isKeyboard(void);
bool Input_isController(void);
void Input_setTextInput(char* buffer, int size); // if buffer is NULL, disable
void Input_setTextInputPos(int x, int y);
const char* Input_getImeText(void);

void Inventory_clear(void);
bool Inventory_addAll(ItemID item, int quantity);
bool Inventory_remove(SlotID slotID, int quantity);
ItemID Inventory_getItem(SlotID slotID);
int Inventory_getQuantity(SlotID slotID);
EquipType Inventory_getEquipStatus(SlotID slotID);
void Inventory_set(SlotID slotID, ItemID item, int quantity);
void Inventory_replace(SlotID slotID, ItemID item, int quantity);
SlotID Inventory_findNext(ItemTypeID type, int prev); // -1 for none
ItemID Inventory_getPlayerEquip(int equipType);
void Inventory_equip(SlotID slotID);
void Inventory_unequip(SlotID slotID);
SlotID Inventory_find(ItemID item);
int Inventory_getSize(void);
void Inventory_compact(void);

void Item_loadFrom(const char* assetpath);
ItemID Item_find(const char* name);
const char* Item_getName(ItemID self);
const char* Item_getDisplayName(ItemID self);
const char* Item_getDescription(ItemID self);
ItemTypeID Item_getType(ItemID self);
SpriteID Item_getIcon(ItemID self);
int Item_getStat(ItemID self, Stat stat);
bool Item_isStackable(ItemID self);
int Item_getMaxStack(ItemID self);
EquipType Item_getEquipType(ItemID self);
bool Item_isConsumable(ItemID self);
bool Item_consume(ItemID self, int entity);
AttackID Item_getSkill(ItemID self, int index);
int Item_getPrice(ItemID self);

// todo: maybe a name like ItemGrouping would be clearer?
// todo: should these maybe not be publicly available? just use Item_* fns?
void ItemType_loadFrom(const char* assetpath);
ItemTypeID ItemType_find(const char* name);
const char* ItemType_getName(ItemTypeID self);
ItemCategory ItemType_getCategory(ItemTypeID self);
ItemTypeID ItemType_next(ItemCategory category, ItemTypeID prev); // -1 for none

void Loading_enter(void);
void Loading_leave(void);
void Loading_update(void);
void Loading_render(void);
SDL_Texture* Loading_loadTexture(const char* name);
void Loading_loadSpriteImage(const char* name);

void Log_error(const char* format, ...);
void Log_warn(const char* format, ...);
void Log_info(const char* format, ...);
void Log_debug(const char* format, ...);

void LootTable_init(void);
LootTableID LootTable_find(const char* name);
void LootTable_drop(LootTableID self, int x, int y);

extern SDL_Window* Main_window;
extern SDL_Renderer* Main_renderer;
void Main_stateChange(MainState nextState);

int Math_sanitizeAngle(int a);
int Math_lerp(int a, int b, float f);
int Math_angleLengthX(int angle, int length);
int Math_angleLengthY(int angle, int length);
int Math_angleTo(int fromX, int fromY, int toX, int toY);
int Math_lengthSquared(int x, int y);
float Math_clampf(float value, float min, float max);
void Math_normalizeXY(float* x, float* y);

void Menu_enter(void);
void Menu_leave(void);
void Menu_update(void);
void Menu_render(void);

void Music_loadFrom(const char* assetpath);
MusicID Music_find(const char* name);
void Music_play(MusicID self);
void Music_stop(void);

void NavGrid_set(int width, int height, bool* data);
int NavGrid_findPath(int fromX, int fromY, int toX, int toY,
    int maxLength, int* pathXs, int* pathYs);

void Particles_load(void);
ParticlesID Particles_find(const char* name);
void Particles_spawn(ParticlesID particles, int x, int y, int z);
void Particles_update(void);
void Particles_draw(void);

void Quest_load(void);
void Quest_reset(void);
QuestID Quest_find(const char* name);
void Quest_begin(QuestID quest);
const char* Quest_getTitle(QuestID quest);
QuestID Quest_getActive(int slot);
const char* Quest_getObjectiveStr(QuestID quest, int obj);
bool Quest_getObjectiveComplete(QuestID quest, int obj);
void Quest_signalEvent(QuestEvent event, int arg);
bool Quest_isBegun(QuestID quest);
bool Quest_isAwaitingTalk(QuestID quest, VillagerID villager);
bool Quest_isCompleted(QuestID quest);
int Quest_getPriority(QuestID quest);
void Quest_bump(QuestID quest);

void Recipe_load(void);
RecipeID Recipe_find(const char* name);
RecipeID Recipe_next(RecipeID recipe);
ItemID Recipe_getOutput(RecipeID recipe);
int Recipe_getOutputQuantity(RecipeID recipe);
ItemID Recipe_getInput(RecipeID recipe, int i);
int Recipe_getInputQuantity(RecipeID recipe, int i);
ItemID Recipe_getBase(RecipeID recipe);
bool Recipe_isCraftable(RecipeID recipe);
void Recipe_craft(RecipeID recipe);
void Recipe_unlock(RecipeID recipe);
bool Recipe_isUnlocked(RecipeID recipe);

void Region_load(int id);
void Region_render(int layerID);
bool Region_isTileSolid(int tx, int ty);

void SDLAssert(bool cond);

void Sound_loadFrom(const char* assetpath);
SoundID Sound_find(const char* name);
void Sound_play(SoundID self);

// todo: this should probably be local within graphics.c
void Sprite_setSpriteImage(const char* assetpath, SDL_Texture* texture);
void Sprite_loadFrom(const char* assetpath);
SpriteID Sprite_find(const char* name);
void Sprite_draw(SpriteID self, int x, int y);
void Sprite_drawScaled(SpriteID self, int x, int y, int w, int h);

void SpriteQueue_clear(void);
void SpriteQueue_addSprite(SpriteID spriteID, int x, int y, int z);
void SpriteQueue_addAnim(AnimationID animationID, int x, int y, int z, int* time);
void SpriteQueue_render(void);

void StatusEffect_load(void);
StatusEffectID StatusEffect_find(const char* name);
int StatusEffect_applyStat(StatusEffectID seID, Stat stat, int value);
SpriteID StatusEffect_getIcon(StatusEffectID seID);

int String_parseInt(const char* string, int def);
bool String_parseBool(const char* string, bool def);
float String_parseFloat(const char* string, float def);
void String_parseInt2(const char* string, int* a, int* b);
int String_parseIntArray(const char* string, int* array, int max);
int String_parseIntArrayExt(const char* string, int* array, int max,
        int (*fnConvert)(const char*));
void String_parseIntDirs(const char* string, int* array,
        int (*fnConvert)(const char*), int def);
int String_parseEnum(const char* string, const char* values, int def);
char* String_trimLocal(char* string);
int String_split(const char* string, char delim, char** parts, int max, int len);

void Title_enter(void);
void Title_leave(void);
void Title_update(void);
void Title_render(void);

void Villager_load(void);
VillagerID Villager_find(const char* name);
const char* Villager_getDialog(VillagerID villagerID);
const char* Villager_getTitle(VillagerID villagerID);

Archive Archive_open(const char* path);
void Archive_close(Archive self);
size_t Archive_getSize(Archive self, const char* name);
size_t Archive_read(Archive self, const char* name, size_t size, void* dst);
