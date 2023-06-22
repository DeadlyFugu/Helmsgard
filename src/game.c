#include "common.h"

#define MAX_TALL_GRASS 1024

// ===== [[ Local Types ]] =====

typedef enum {
    GameSubstate_interact,
    GameSubstate_console,
    GameSubstate_pause,
    GameSubstate_inventory,
    GameSubstate_dialog,
    GameSubstate_crafting,
    GameSubstate_hitstop,
    GameSubstate_chest,
    GameSubstate_gameOver,
    GameSubstate_demoWon,
    GameSubstate_shop
} GameSubstate;

typedef struct {
    int tx, ty;
    int side;
} TallGrass;

// ===== [[ Declarations ]] =====

static void _updateInteract(void);
static void _updateConsole(void);
static void _updatePause(void);
static void _updateInventory(void);
static void _updateDialog(void);
static void _updateCrafting(void);
static void _updateChest(void);
static void _updateGameOver(void);
static void _updateDemoWon(void);
static void _updateShop(void);

static void _renderInteract(void);
static void _renderConsole(void);
static void _renderPause(void);
static void _renderInventory(void);
static void _renderDialog(void);
static void _renderCrafting(void);
static void _renderChest(void);
static void _renderGameOver(void);
static void _renderDemoWon(void);
static void _renderShop(void);

static void _enterSubstate(GameSubstate newSubstate);
static void _updateWorld(void);
static void _renderWorld(void);
static void _renderHud(void);

static void _drawNinepatch(int x, int y, int w, int h);
static int _questCompare(const void* a, const void* b);

// ===== [[ Static Data ]] =====

static GameSubstate _substate;
static int _sel;
static int _camx, _camy;
static bool _camsnap;
static int _dialogPartner;
static DialogLineID _dialogLine;
static int _dialogTextTimer;
static RecipeID _craftRecipes[64];
static int _craftRecipeCount;
static int _hitstopTime;
static ItemID _chestItems[16];
static int _chestQtys[16];
static int _chestCount;
static int _invScroll;
static int _invScrollTarget;
static TallGrass _tallGrasses[MAX_TALL_GRASS];
static int _tallGrassCount;
static bool _showHintText;
static char _hintText[128];
static int _hintTextTimer;
static int _gold = 20;
static int _shopQtys[16] = { 20, 10, 15, 20, 2, 2, 1, 1 };

// ===== [[ Implementations ]] =====

void Game_init(void) {
}

void Game_reload(void) {
    Entity_destroyAll();
    Field_clear();
    _tallGrassCount=0;
    Region_load(2); // field is set within here
    // for (int i = 0; i < 5; i++)
    //     Entity_create(5400+2560, 2300+2560, 1);
    // for (int i = 0; i < 5; i++)
    //     Entity_create(6200+2560, 1200+2560, 1);
    // Entity_create(6100+2560, 3900+2560, 0);
    // Entity_create(5800+2560, 3900+2560, 2);
    // for (int i = 0; i < 5; i++)
    //     Entity_create(5500+2560, 3900+2560, 3);
    // for (int i = 0; i < 5; i++)
    //     Entity_dropItem(5400+2560, 2400+2560, Item_find("IronOre"));
    // for (int i = 0; i < 5; i++)
    //     Entity_dropItem(5500+2560, 3700+2560, Item_find("Feather"));
    Inventory_clear();
    Quest_reset();
    _camsnap = true;
}

void Game_enter(void) {
    _substate = GameSubstate_interact;
    _sel = 0;
    Game_reload();
}

void Game_leave(void) {
}

void Game_update(void) {
    switch (_substate) {
        case GameSubstate_interact: _updateInteract(); break;
        case GameSubstate_console: _updateConsole(); break;
        case GameSubstate_pause: _updatePause(); break;
        case GameSubstate_inventory: _updateInventory(); break;
        case GameSubstate_dialog: _updateDialog(); break;
        case GameSubstate_crafting: _updateCrafting(); break;
        case GameSubstate_hitstop: {
            if (--_hitstopTime <= 0) _enterSubstate(GameSubstate_interact);
        } break;
        case GameSubstate_chest: _updateChest(); break;
        case GameSubstate_gameOver: _updateGameOver(); break;
        case GameSubstate_demoWon: _updateDemoWon(); break;
        case GameSubstate_shop: _updateShop(); break;
    }
}

void Game_render(void) {
    switch (_substate) {
        case GameSubstate_interact: _renderInteract(); break;
        case GameSubstate_console: _renderConsole(); break;
        case GameSubstate_pause: _renderPause(); break;
        case GameSubstate_inventory: _renderInventory(); break;
        case GameSubstate_dialog: _renderDialog(); break;
        case GameSubstate_crafting: _renderCrafting(); break;
        case GameSubstate_hitstop: _renderInteract(); break;
        case GameSubstate_chest: _renderChest(); break;
        case GameSubstate_gameOver: _renderGameOver(); break;
        case GameSubstate_demoWon: _renderDemoWon(); break;
        case GameSubstate_shop: _renderShop(); break;
    }
}

void Game_setDialog(int entity) {
    _enterSubstate(GameSubstate_dialog);
    _dialogPartner = entity;
    VillagerID villager = Entity_getVillager(_dialogPartner);
    DialogID dlg = Dialog_findCandidate(villager);
    _dialogLine = Dialog_firstLine(dlg);
    _dialogTextTimer = 0;
    Dialog_execute(_dialogLine);
    Quest_signalEvent(QuestEvent_talk, villager);
}

void Game_setCrafting(void) {
    _enterSubstate(GameSubstate_crafting);
}

void Game_setHitstop(int frames) {
    if (frames > _hitstopTime) _hitstopTime = frames;
    _enterSubstate(GameSubstate_hitstop);
}

bool Game_shouldAnimate(void) {
    return _hitstopTime == 0;
}

void Game_setChest(int entity) {
    // todo: toctou-ish bug here if entity has items added after this
    //     executes (i.e. by another system in same frame, so fine for now)
    _dialogPartner = entity;
    _chestCount = 0;
    _sel = 0;
    ItemID* items = Entity_getMiniInvItems(entity);
    int* quantity = Entity_getMiniInvQty(entity);
    for (int i = 0; i < 16; i++) {
        if (items[i] != -1 && quantity[i] != 0) {
            _chestItems[_chestCount] = items[i];
            _chestQtys[_chestCount] = quantity[i];
            _chestCount++;
        }
    }
    _enterSubstate(GameSubstate_chest);
}

void Game_setShop(void) {
    _sel = 0;
    _chestCount = 8;
    _chestItems[0] = Item_find("Potato");
    _chestItems[1] = Item_find("Bread");
    _chestItems[2] = Item_find("Log");
    _chestItems[3] = Item_find("Berry");
    _chestItems[4] = Item_find("PotionAtkUp");
    _chestItems[5] = Item_find("PotionDefUp");
    _chestItems[6] = Item_find("IronHelmet");
    _chestItems[7] = Item_find("ChainmailArmor");
    for (int i = 0; i < 8; i++) {
        _chestQtys[i] = _shopQtys[i];
    }
    _enterSubstate(GameSubstate_shop);
}

void Game_addGrass(int x, int y, int side) {
    if (_tallGrassCount >= MAX_TALL_GRASS) return;

    TallGrass* tg = &_tallGrasses[_tallGrassCount++];
    tg->tx = x;
    tg->ty = y;
    tg->side = side;
}

void Game_setDefeat(void) {
    _enterSubstate(GameSubstate_gameOver);
}

void Game_setWon(void) {
    _enterSubstate(GameSubstate_demoWon);
}

void Game_setHintText(const char* text) {
    if (text == NULL) {
        _showHintText = false;
    } else {
        _showHintText = true;
        if (strncmp(_hintText, text, 128) != 0) {
            strncpy(_hintText, text, 128);
            _hintTextTimer = 0;
        }
    }
}

void Game_addGold(int amount) {
    _gold += amount;
}

static void _updateInteract(void) {
    if (Input_isReleased(InputButton_pause)) {
        _enterSubstate(GameSubstate_pause);
    }
    if (Input_isReleased(InputButton_hotbar)) {
        _enterSubstate(GameSubstate_console);
    }
    if (Input_isReleased(InputButton_inventory)) {
        _enterSubstate(GameSubstate_inventory);
        Inventory_compact();
    }
    _updateWorld();
}

static void _updateConsole(void) {
    Console_update();
    if (Console_shouldClose()) {
        _enterSubstate(GameSubstate_interact);
    }
}

static void _updatePause(void) {
    // todo: a better overall design might be feeding in events top down
    //       i.e. main game loop sends Event_buttonPause etc. events
    //       each game state can handle differently, passing on to substates
    //       could also merge update and render functions, with render event
    //       maybe enter/leave event also?
    //       debug trace event for info to show on F3?
    if (Input_isReleased(InputButton_back)) {
        _enterSubstate(GameSubstate_interact);
    }
    if (Input_isReleased(InputButton_pause)) {
        _enterSubstate(GameSubstate_interact);
    }
    if (Input_isReleased(InputButton_accept) ||
        (Input_isKeyboard() && Input_isPressed(InputButton_attack)) ||
        (Input_isKeyboard() && Input_isPressed(InputButton_interact))) {
        switch (_sel) {
            case 0: _enterSubstate(GameSubstate_interact); break;
            case 1: Main_stateChange(MainState_menu); break;
        }
    }

    if (Input_isPressed(InputButton_up) && _sel > 0) {
        _sel--; 
    }
    if (Input_isPressed(InputButton_down) && _sel < 1) {
        _sel++;
    }
}

static void _updateInventory(void) {
    if (Input_isReleased(InputButton_back)) {
        _enterSubstate(GameSubstate_interact);
    }
    if (Input_isReleased(InputButton_inventory)) {
        _enterSubstate(GameSubstate_interact);
    }

    if (Input_isPressed(InputButton_up) && _sel > 0) {
        _sel--; 
    }
    if (Input_isPressed(InputButton_down) && _sel < Inventory_getSize() - 1) {
        _sel++;
    }
    if (Input_isPressed(InputButton_accept) ||
        (Input_isKeyboard() && Input_isPressed(InputButton_attack)) ||
        (Input_isKeyboard() && Input_isPressed(InputButton_interact))) {
        ItemID selItem = Inventory_getItem(_sel);
        if (Item_getEquipType(selItem) != EquipType_none) {
            if (Inventory_getEquipStatus(_sel) == EquipType_none) {
                Inventory_equip(_sel);
            } else {
                Inventory_unequip(_sel);
            }
        } else if (Item_isConsumable(selItem)) {
            int player = Entity_getPlayer();
            if (player != -1) {
                if (Item_consume(selItem, player)) {
                    Inventory_remove(_sel, 1);
                }
            }
        }
    }

    int selPos = _sel * 23;
    if (_invScrollTarget < selPos - 100) {
        _invScrollTarget = selPos - 100;
    } else if (_invScrollTarget > selPos - 30) {
        _invScrollTarget = selPos - 30;
    }
    if (_invScrollTarget < 0) _invScrollTarget = 0;
    if (_invScroll/100 != _invScrollTarget) {
        int delta = _invScrollTarget * 100 - _invScroll;
        _invScroll += delta / 10;
        // Log_debug("%d %d", _invScroll, _invScrollTarget);
    }
}

static void _updateDialog(void) {
    if (Input_isPressed(InputButton_up) && _sel > 0) {
        _sel--; 
    }
    if (Input_isPressed(InputButton_down) &&
        _sel < Dialog_getChoiceCount(_dialogLine) - 1) {
        _sel++;
    }
    if (Input_isPressed(InputButton_accept) ||
        (Input_isKeyboard() && Input_isPressed(InputButton_attack)) ||
        (Input_isKeyboard() && Input_isPressed(InputButton_interact))) {
        if (_dialogTextTimer == 256) {
            _dialogLine = Dialog_getNext(_dialogLine, _sel);
            _dialogTextTimer = 0;
            if (_dialogLine == -1) _enterSubstate(GameSubstate_interact);
            else Dialog_execute(_dialogLine);
        } else {
            _dialogTextTimer = 256;
        }
    }
}

static void _updateCrafting(void) {
    if (Input_isReleased(InputButton_back)) {
        _enterSubstate(GameSubstate_interact);
    }
    if (Input_isReleased(InputButton_inventory)) {
        _enterSubstate(GameSubstate_interact);
    }
    if (Input_isPressed(InputButton_up) && _sel > 0) {
        _sel--; 
    }
    if (Input_isPressed(InputButton_down) && _sel < _craftRecipeCount - 1) {
        _sel++;
    }
    if (Input_isPressed(InputButton_accept) ||
        (Input_isKeyboard() && Input_isPressed(InputButton_attack)) ||
        (Input_isKeyboard() && Input_isPressed(InputButton_interact))) {
        if (Recipe_isCraftable(_craftRecipes[_sel])) {
            Recipe_craft(_craftRecipes[_sel]);
            Quest_signalEvent(QuestEvent_craft, _craftRecipes[_sel]);
        }
    }
}

static void _updateChest(void) {
    if (Input_isReleased(InputButton_back)) {
        _enterSubstate(GameSubstate_interact);
    }
    if (Input_isReleased(InputButton_inventory)) {
        _enterSubstate(GameSubstate_interact);
    }
    if (Input_isPressed(InputButton_up) && _sel > 0) {
        _sel--; 
    }
    if (Input_isPressed(InputButton_down) && _sel < _chestCount - 1) {
        _sel++;
    }
    if (Input_isPressed(InputButton_accept) ||
        (Input_isKeyboard() && Input_isPressed(InputButton_attack)) ||
        (Input_isKeyboard() && Input_isPressed(InputButton_interact))) {
        if (_sel >= _chestCount) return;
        ItemID item = _chestItems[_sel];
        int quantity = _chestQtys[_sel];
        if (Inventory_addAll(item, quantity)) {
            for (int i = _sel; i < _chestCount-1; i++) {
                _chestItems[i] = _chestItems[i+1];
                _chestQtys[i] = _chestQtys[i+1];
            }
            _chestCount--;
            for (int i = _chestCount; i < 16; i++) {
                _chestItems[i] = -1;
                _chestQtys[i] = 0;
            }
            Entity_setMiniInvItems(_dialogPartner, _chestItems);
            Entity_setMiniInvQty(_dialogPartner, _chestQtys);
            if (_chestCount <= 0) {
                _enterSubstate(GameSubstate_interact);
            }
            if (_sel >= _chestCount) _sel = _chestCount-1;
        }
    }
}

static void _updateGameOver(void) {
    _updateWorld();
    if (_sel++<50) return;
    if (Input_isReleased(InputButton_back)) {
        Main_stateChange(MainState_menu);
    }
}

static void _updateDemoWon(void) {
    if (_sel++<50) return;
    if (Input_isReleased(InputButton_back)) {
        Main_stateChange(MainState_menu);
    }
    if (Input_isReleased(InputButton_accept)) {
        _enterSubstate(GameSubstate_interact);
    }
}

static void _updateShop(void) {
    if (Input_isReleased(InputButton_back)) {
        _enterSubstate(GameSubstate_interact);
    }
    if (Input_isReleased(InputButton_inventory)) {
        _enterSubstate(GameSubstate_interact);
    }
    if (Input_isPressed(InputButton_up) && _sel > 0) {
        _sel--; 
    }
    if (Input_isPressed(InputButton_down) && _sel < _chestCount - 1) {
        _sel++;
    }
    if (Input_isPressed(InputButton_accept) ||
        (Input_isKeyboard() && Input_isPressed(InputButton_attack)) ||
        (Input_isKeyboard() && Input_isPressed(InputButton_interact))) {
        if (_sel >= _chestCount) return;
        ItemID item = _chestItems[_sel];
        int quantity = _chestQtys[_sel];
        int price = Item_getPrice(item);
        if (quantity > 0 && price <= _gold && Inventory_addAll(item, 1)) {
            _chestQtys[_sel]--;
            _shopQtys[_sel] = _chestQtys[_sel];
            _gold -= price;
        }
    }
}

static void _renderInteract(void) {
    _renderWorld();
    _renderHud();
}

static void _renderConsole(void) {
    _renderWorld();
    Console_render();
}

static void _renderPause(void) {
    _renderWorld();
    Draw_setColor(Color_dkgray);
    //Draw_rect(0, 0, 140, 32*3+20);
    const int bx = (SCREEN_WIDTH-76)/2;
    const int by = (SCREEN_HEIGHT-(16*3+20))/2;
    _drawNinepatch(bx, by, 76, 16*3+20);
    Draw_setColor(Color_white);
    // BFontID fnt = BFont_find("inv_detail1");
    // BFont_drawText(fnt, bx+10, by+10, "## PAUSED ##");
    BFontID bf = BFont_find("dialog");
    BFont_drawText(bf, bx+10, by+10, "-- Paused --");
    BFont_drawText(bf, bx+10, by+26, "  Resume");
    BFont_drawText(bf, bx+10, by+26+16, "  Quit");
    BFont_drawText(bf, bx+10, by+26+16*_sel, ">");
}

static void _renderInventory(void) {
    _renderWorld();

    // show hearts/status effects also
    {
        SpriteID heartFull = Sprite_find("ui/heartfull");
        SpriteID heartHalf = Sprite_find("ui/hearthalf");
        SpriteID heartNone = Sprite_find("ui/heartnone");

        int playerID = Entity_getPlayer();
        if (playerID == -1) return;
        int maxhp = Entity_getMaxHP(playerID);
        int hp = Entity_getHP(playerID);
        for (int i = 0; i < maxhp / 2; i++) {
            SpriteID icon = heartFull;
            if (hp <= i*2) {
                icon = heartNone;
            } else if (hp == i*2+1) {
                icon = heartHalf;
            }
            Sprite_draw(icon, 9 + 15*i, 9);
        }

        int iconx = 8;
        for (int i = 0; i < 4; i++) {
            StatusEffectID se = Entity_getStatusEffect(playerID, i);
            if (se != -1) {
                SpriteID icon = StatusEffect_getIcon(se);
                Sprite_draw(icon, iconx, 32);
                iconx+=16;
            }
        }
    }

    int xt = SCREEN_WIDTH/2-160;
    // Draw_setColor((SDL_Color) {217, 160, 102, 255});
    // Draw_rect(0, 0, 400, 32*8+20);
    Draw_setTranslate(xt, 0);

    SpriteID invPanel = Sprite_find("ui/invPanel");
    Sprite_draw(invPanel, 30, 39);
    SpriteID invPanelBack = Sprite_find("ui/invPanelBack");
    Sprite_draw(invPanelBack, 30+9, 39+126);
    SpriteID invTabs = Sprite_find("ui/invTabs");
    Sprite_draw(invTabs, 30, 15);

    SpriteID invSlot = Sprite_find("ui/invSlot");
    SpriteID invEmpty = Sprite_find("ui/invEmpty");

    BFontID fntDark = BFont_find("inv_detail1");
    BFontID fnt = BFont_find("inv_detail2");
    BFontID fntBig = BFont_find("inv_big");

    Draw_setTranslate(xt, -_invScroll/100);
    const int stride = 23;
    const int labelsX = 63;
    const int labelsY = 67;
    const int iconsX = 42;
    const int iconsY = 65;
    const int slotsX = 40;
    const int slotsY = 62;

    Draw_setClip(xt+10+30, 23+39, 131, 154);
    for (int i = 0; i < Inventory_getSize(); i++) {
        int quantity = Inventory_getQuantity(i);
        if (quantity == 0) {
            Sprite_draw(invEmpty, slotsX, slotsY+stride*i);
        } else {
            Sprite_draw(invSlot, slotsX, slotsY+stride*i);
            ItemID item = Inventory_getItem(i);
            const char* name = Item_getDisplayName(item);
            BFont_drawText(fnt, labelsX, labelsY+stride*i,
                "%s", name);
            Sprite_draw(Item_getIcon(item), iconsX, iconsY+stride*i);

            const char* group = ItemType_getName(Item_getType(item));
            BFont_drawText(fnt, 64, 75+stride*i, "(%s)", group);

            EquipType equip = Inventory_getEquipStatus(i);
            if (equip != EquipType_none) {
                // const char* equipNames[] = {
                //     "<none>", "Weapon", "Head", "Torso", "Accessory"
                // };
                BFont_drawText(fnt, 132, 75+stride*i, "Equipped");
            } else {
                BFont_drawText(fnt, 152, 75+stride*i, "*%d", quantity);
            }
        }
    }
    
    BFont_drawText(fnt, labelsX-4, labelsY+stride*_sel, "X");
    Draw_clearClip();
    Draw_setTranslate(xt, 0);

    ItemID item = Inventory_getItem(_sel);
    if (item != -1) {
        const char* name = Item_getDisplayName(item);
        BFont_drawTextExt(fntBig, 187, 48, 93, 0.5f, "%s", name);

        SpriteID icon = Item_getIcon(item);
        Sprite_draw(icon, 218+8, 87+8);

        const char* desc = Item_getDescription(item);
        BFont_drawTextExt(fntDark, 187, 162, 93, 0.5f, "%s", desc);
    }

    int playerID = Entity_getPlayer();
    if (playerID == -1) return;
    // Entity* player = &Entity_table[playerID];
    // Draw_setColor(Color_white);
    // Draw_text(10, SCREEN_HEIGHT - 20, "lv.%d (%d/50 exp)", player->lvl, player->xp);

    Draw_setTranslate(0, 0);
    _drawNinepatch(SCREEN_WIDTH-80, 4, 80-8, 16);
    BFontID bf_dialog = BFont_find("dialog");
    BFont_drawText(bf_dialog, SCREEN_WIDTH-80+4, 8, "%4d crowns", _gold);
}

static void _renderDialog(void) {
    _renderWorld();

    VillagerID villager = Entity_getVillager(_dialogPartner);
    const char* dialog = (_dialogLine != -1) ?
        Dialog_getMessage(_dialogLine) :
        Villager_getDialog(villager);
    if (!dialog || !dialog[0]) dialog = "It looks like it might start raining soon.";

    const char* title = Villager_getTitle(villager);
    if (!title) title = "Charlie";


    if (_dialogTextTimer/2 < 128) {
        _dialogTextTimer++;
        if (strlen(dialog) <= _dialogTextTimer/2) _dialogTextTimer = 256;
    }
    Draw_setColor(Color_ltgray);
    _drawNinepatch(10, SCREEN_HEIGHT - 90, SCREEN_WIDTH - 20, 80);
    Draw_setColor(Color_white);
    // Draw_text(20, SCREEN_HEIGHT - 80, "%.*s", _dialogTextTimer/2, dialog);
    BFontID bf_dialog = BFont_find("dialog");
    BFont_drawTextExt(bf_dialog,
        20, SCREEN_HEIGHT - 80, SCREEN_WIDTH - 20 - 100, 0.0f,
        "%.*s", _dialogTextTimer/2, dialog
    );

    if (!Dialog_getHideName(_dialogLine)) {
        Draw_setColor(Color_gray);
        _drawNinepatch(35, SCREEN_HEIGHT - 115+3+4, 100, 30-6);
        Draw_setColor(Color_white);
        // Draw_text(40, SCREEN_HEIGHT - 110, title);
        BFont_drawText(bf_dialog, 40+8, SCREEN_HEIGHT - 110+6+4, title);
    }

    int choiceCount = Dialog_getChoiceCount(_dialogLine);
    if (choiceCount > 0 && _dialogTextTimer == 256) {
        Draw_setColor(Color_dkgray);
        _drawNinepatch(SCREEN_WIDTH - 110, SCREEN_HEIGHT - 140, 100, 40);
        Draw_setColor(Color_white);
        for (int i = 0; i < choiceCount; i++) {
            BFont_drawText(bf_dialog,
                SCREEN_WIDTH - 100, SCREEN_HEIGHT - 130 + i*15,
                "%s %s", _sel == i ? ">" : "  ",
                Dialog_getChoice(_dialogLine, i));
        }
    }
}

static void _renderCrafting(void) {
    _renderWorld();
    Draw_setColor(Color_dkgray);
    _drawNinepatch(40, 40, SCREEN_WIDTH-80, 16*_craftRecipeCount+20);
    BFontID bf = BFont_find("dialog");
    BFontID bfgray = BFont_find("dialog_gray");
    for (int i = 0; i < _craftRecipeCount; i++) {
        RecipeID recipe = _craftRecipes[i];
        ItemID item = Recipe_getOutput(recipe);
        const char* name = Item_getDisplayName(item);
        bool craftable = Recipe_isCraftable(recipe);
        BFont_drawText(craftable?bf:bfgray, 50, 50+16*i, "     %s", name);
        Sprite_draw(Item_getIcon(item), 50+8-2, 50-2+16*i);
    }
    RecipeID selRecipe = _craftRecipes[_sel];
    if (selRecipe != -1) {
        int y = _craftRecipeCount*16+32+40;
        int height = 0;
        for (int i = 0; i < 8; i++) {
            if (Recipe_getInput(selRecipe, i) == -1) break;
            if (Recipe_getInputQuantity(selRecipe, i) == 0) break;
            height += 16;
        }
        _drawNinepatch(40+16, y-4, 140, height+4);
        for (int i = 0; i < 8; i++) {
            ItemID item = Recipe_getInput(selRecipe, i);
            if (item == -1) break;
            const char* name = Item_getDisplayName(item);
            int hasQty = Inventory_getQuantity(Inventory_find(item));
            if (hasQty < 0) hasQty = 0;
            int reqQty = Recipe_getInputQuantity(selRecipe, i);
            if (reqQty == 0) break;
            bool sufficient = hasQty >= reqQty;
            BFont_drawText(sufficient?bf:bfgray, 50, y+16*i,
                "     %s   %d/%d", name, hasQty, reqQty);
            Sprite_draw(Item_getIcon(item), 50+8-2, y-2+16*i);
        }
    }
    Draw_setColor(Color_red);
    BFont_drawText(bf, 50, 50+16*_sel, ">");
}

static void _renderChest(void) {
    _renderWorld();
    Draw_setColor(Color_dkgray);
    _drawNinepatch(40, 40, SCREEN_WIDTH-80, 16*_chestCount+20);
    BFontID bf = BFont_find("dialog");
    for (int i = 0; i < _chestCount; i++) {
        ItemID item = _chestItems[i];
        const char* name = Item_getDisplayName(item);
        int qty = _chestQtys[i];
        Draw_setColor(Color_white);
        BFont_drawText(bf, 50, 50+16*i, "     %s  x%d", name, qty);
        Sprite_draw(Item_getIcon(item), 50+8-2, 50-2+16*i);
    }
    Draw_setColor(Color_red);
    BFont_drawText(bf, 50, 50+16*_sel, ">");
}

static void _renderGameOver(void) {
    _renderWorld();
    int x = SCREEN_WIDTH/2-100, y = SCREEN_HEIGHT/2-40;
    _drawNinepatch(x, y, 200, 80);
    BFontID bf = BFont_find("dialog");
    BFont_drawTextExt(bf, x+10, y+10, 180, 0.5f, "GAME OVER");
    BFont_drawText(bf, x+10, y+30, "Press ESCAPE to return to menu.");
}

static void _renderDemoWon(void) {
    int x = SCREEN_WIDTH/2-100, y = SCREEN_HEIGHT/2-40;
    _drawNinepatch(x, y, 200, 80);
    BFontID bf = BFont_find("dialog");
    BFont_drawTextExt(bf, x+10, y+10, 180, 0.5f, "END OF DEMO");
    BFont_drawText(bf, x+10, y+30, "Press ESCAPE to return to menu.");
    BFont_drawText(bf, x+10, y+50, "Press ENTER to continue playing.");
}

static void _renderShop(void) {
    _renderWorld();
    Draw_setColor(Color_dkgray);
    _drawNinepatch(40, 40, SCREEN_WIDTH-80, 16*_chestCount+20);
    BFontID bf = BFont_find("dialog");
    BFontID bfgray = BFont_find("dialog_gray");
    for (int i = 0; i < _chestCount; i++) {
        ItemID item = _chestItems[i];
        const char* name = Item_getDisplayName(item);
        int qty = _chestQtys[i];
        int cost = Item_getPrice(item);
        if (qty > 0) {
            Draw_setColor(Color_white);
            bool aff = cost <= _gold;
            BFont_drawText(aff?bf:bfgray, 50, 50+16*i, "     %s  x%d", name, qty);
            BFont_drawText(aff?bf:bfgray, SCREEN_WIDTH-80-50, 50+16*i, "%4d crowns", cost);
            Sprite_draw(Item_getIcon(item), 50+8-2, 50-1+16*i);
        } else {
            Draw_setColor(Color_gray);
            BFont_drawText(bfgray, 50, 50+16*i, "    -- sold out --");
        }
    }
    Draw_setColor(Color_red);
    BFont_drawText(bf, 50, 50+16*_sel, ">");
    Draw_setColor(Color_white);
    _drawNinepatch(SCREEN_WIDTH-80, 4, 80-8, 16);
    BFontID bf_dialog = BFont_find("dialog");
    BFont_drawText(bf_dialog, SCREEN_WIDTH-80+4, 8, "%4d crowns", _gold);
}

static void _enterSubstate(GameSubstate newSubstate) {
    if (_substate == GameSubstate_console) {
        Console_leave();
    }

    _sel = 0;

    if (newSubstate == GameSubstate_console) {
        Console_enter();
    } else if (newSubstate == GameSubstate_crafting) {
        // populate recipe cache
        _craftRecipeCount = 0;
        RecipeID next = Recipe_next(-1);
        while (next != -1 && _craftRecipeCount < 64) {
            if (Recipe_isUnlocked(next)) {
                _craftRecipes[_craftRecipeCount++] = next;
            }
            next = Recipe_next(next);
        }
    } else if (newSubstate == GameSubstate_inventory) {
        _invScroll = 0;
        _invScrollTarget = 0;
    }

    _substate = newSubstate;
}

static void _updateWorld(void) {
    Entity_updateAll();
    Particles_update();

    // update camera
    int playerID = Entity_getPlayer();
    if (playerID != -1) {
        int px = Entity_getX(playerID);
        int py = Entity_getY(playerID);
        EntityState pstate = Entity_getState(playerID);
        int dx = px - _camx;
        int dy = py - _camy;
        float f = pstate == EntityState_rolling ? 0.07f : 0.1f;
        if (_camsnap) {
            f = 1.0f;
            _camsnap = false;
        }
        _camx += (int) ceilf(dx * f);
        _camy += (int) ceilf(dy * f);
    }
}

static void _renderWorld(void) {
    Draw_setTranslate(
        -_camx/16 + SCREEN_WIDTH / 2,
        -_camy/16 + SCREEN_HEIGHT / 2);
    Region_render(0);
    Region_render(1);
    SpriteQueue_clear();
    Entity_renderAll();
    Particles_draw();
    SpriteID tgBack[4];
    tgBack[0] = Sprite_find("tgBackMid");
    tgBack[1] = Sprite_find("tgBackLeft");
    tgBack[2] = Sprite_find("tgBackRight");
    tgBack[3] = Sprite_find("tgBackSingle");
    SpriteID tgFront[4];
    tgFront[0] = Sprite_find("tgFrontMid");
    tgFront[1] = Sprite_find("tgFrontLeft");
    tgFront[2] = Sprite_find("tgFrontRight");
    tgFront[3] = Sprite_find("tgFrontSingle");
    for (int i = 0; i < _tallGrassCount; i++) {
        TallGrass* tg = &_tallGrasses[i];
        SpriteQueue_addSprite(tgBack[tg->side], tg->tx*16, tg->ty*16+8, 0);
        SpriteQueue_addSprite(tgFront[tg->side], tg->tx*16, tg->ty*16+16, 0);
    }
    SpriteQueue_render();
    Region_render(2);

    // ---- DEBUG NAVGRID STUFF
    if (Config_showNavGrid) {
        int playerID = Entity_getPlayer();
        if (playerID != -1) {
            int px = Entity_getX(playerID);
            int py = Entity_getY(playerID);
            int pathXs[100];
            int pathYs[100];
            int pathLength = NavGrid_findPath(px / 256, py / 256, 53, 27,
                100, pathXs, pathYs);
            int prevX = px/256;
            int prevY = py/256;
            for (int i = 0; i < SDL_min(pathLength, 100); i++) {
                Draw_setColor(Color_red);
                Draw_line(prevX*16+8, prevY*16+8,
                    pathXs[i]*16+8, pathYs[i]*16+8);
                prevX = pathXs[i];
                prevY = pathYs[i];
            }
        }
    }

    Draw_setTranslate(0, 0);
}

static void _renderHud(void) {
    SpriteID heartFull = Sprite_find("ui/heartfull");
    SpriteID heartHalf = Sprite_find("ui/hearthalf");
    SpriteID heartNone = Sprite_find("ui/heartnone");

    int playerID = Entity_getPlayer();
    if (playerID == -1) return;
    int maxhp = Entity_getMaxHP(playerID);
    int hp = Entity_getHP(playerID);
    for (int i = 0; i < maxhp / 2; i++) {
        SpriteID icon = heartFull;
        if (hp <= i*2) {
            icon = heartNone;
        } else if (hp == i*2+1) {
            icon = heartHalf;
        }
        Sprite_draw(icon, 9 + 15*i, 9);
    }

    int iconx = 8;
    for (int i = 0; i < 4; i++) {
        StatusEffectID se = Entity_getStatusEffect(playerID, i);
        if (se != -1) {
            SpriteID icon = StatusEffect_getIcon(se);
            Sprite_draw(icon, iconx, 32);
            iconx+=16;
        }
    }

    BFontID bf = BFont_find("dialog");
    BFontID bfgray = BFont_find("dialog_gray");
    if (_showHintText) {
        if (_hintTextTimer/2 < 128) _hintTextTimer++;
        int width = Draw_getTextWidth("%.*s", _hintTextTimer/2, _hintText) * 0.85f;
        int height = Draw_getTextHeight("%.*s", _hintTextTimer/2, _hintText);
        // Draw_setColor(Color_dkgray);
        _drawNinepatch(14, SCREEN_HEIGHT-46, width+12, height+12);
        // Draw_setColor(Color_white);
        BFont_drawText(bf, 20, SCREEN_HEIGHT-40, "%.*s", _hintTextTimer/2, _hintText);
    }

    QuestID activeQuests[8];
    int activeQuestCount = 0;
    for (int i = 0; i < 8; i++) {
        QuestID quest = Quest_getActive(i);
        if (quest == -1) break;
        activeQuests[activeQuestCount++] = quest;
    }
    qsort(activeQuests, activeQuestCount, sizeof(QuestID), _questCompare);

    int y = 0;
    for (int i = 0; i < activeQuestCount; i++) {
        QuestID quest = activeQuests[i];

        // draw background
        int width = Draw_getTextWidth(Quest_getTitle(quest));
        int height = 12;
        if (i == 0) {
            for (int j = 0; j < 4; j++) {
                const char* obj = Quest_getObjectiveStr(quest, j);
                if (!obj) break;
                bool complete = Quest_getObjectiveComplete(quest, j);
                int ow = Draw_getTextWidth(obj);
                if (ow > width) width = ow;
                height += 12;
                if (!complete) break;
            }
        }
        _drawNinepatch(20-4,40+y*12-4, width+4, height+4);

        BFont_drawText(bf, 20, 40+y++*12, "%s", Quest_getTitle(quest));
        if (i == 0) {
            for (int j = 0; j < 4; j++) {
                const char* obj = Quest_getObjectiveStr(quest, j);
                if (!obj) break;
                bool complete = Quest_getObjectiveComplete(quest, j);
                if (complete) Draw_setColor(Color_gray);
                else Draw_setColor(Color_white);
                BFont_drawText(complete?bfgray:bf, 40, 40+y++*12, "%s", obj);
                if (!complete) break;
            }
            Draw_setColor(Color_white);
        }
        y++;
    }

    Entity_drawBossHP();
}

static void _drawNinepatch(int x, int y, int w, int h) {
    SpriteID spr_base = Sprite_find("menu/9ptl");

    // todo: big hack, just assuming next sprites are sequential
    SpriteID spr_parts[9];
    for (int i = 0; i < 9; i++) {
        spr_parts[i] = spr_base + i;
    }

    // calculate inner margins
    int xm1 = x + 8;
    int xm2 = x + w - 8;
    int ym1 = y + 8;
    int ym2 = y + h - 8;

    // draw corners
    Sprite_draw(spr_parts[0], x, y);
    Sprite_draw(spr_parts[2], xm2, y);
    Sprite_draw(spr_parts[6], x, ym2);
    Sprite_draw(spr_parts[8], xm2, ym2);

    // draw edges
    if (xm1 < xm2) {
        Sprite_drawScaled(spr_parts[1], xm1, y, xm2-xm1, 8);
        Sprite_drawScaled(spr_parts[7], xm1, ym2, xm2-xm1, 8);
    }
    if (ym1 < ym2) {
        Sprite_drawScaled(spr_parts[3], x, ym1, 8, ym2-ym1);
        Sprite_drawScaled(spr_parts[5], xm2, ym1, 8, ym2-ym1);
    }

    // draw mid
    if (xm1 < xm2 && ym1 < ym2) {
        Sprite_drawScaled(spr_parts[4], xm1, ym1, xm2-xm1, ym2-ym1);
    }
}

static int _questCompare(const void* a, const void* b) {
    QuestID qa = *(QuestID*) a;
    QuestID qb = *(QuestID*) b;
    return Quest_getPriority(qb) - Quest_getPriority(qa);
}
