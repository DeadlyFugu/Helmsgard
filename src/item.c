#include "common.h"

#define MAX_ITEMS 256
#define MAX_ITEMTYPES 64
#define MAX_INVSLOTS 16
#define MAX_LOOTTABLES 64
#define MAX_LOOTITEMS 16
#define DESC_LENGTH 128

// ===== [[ Local Types ]] =====

typedef struct {
    char name[NAME_LENGTH];
    ItemTypeID type;
    SpriteID icon;
    int price;
    int prop1;
    int prop2;
    int lck;
    char desc[DESC_LENGTH];
    char displayName[NAME_LENGTH];
    AttackID skills[3];
    // todo: have specific properties (atk, def, lck, etc.) instead of propN
} Item;

typedef struct {
    char name[NAME_LENGTH];
    ItemCategory category;
} ItemType;

typedef struct {
    ItemID item;
    int quantity;
    EquipType equip;
} InventorySlot;

typedef struct {
    char name[NAME_LENGTH];
    int count;
    ItemID items[MAX_LOOTITEMS];
    int chance[MAX_LOOTITEMS];
    int quantity[MAX_LOOTITEMS];
} LootTable;

// ===== [[ Declarations ]] =====

// ===== [[ Static Data ]] =====

static Item _items[MAX_ITEMS];
static ItemType _itemTypes[MAX_ITEMTYPES];
static InventorySlot _inventory[MAX_INVSLOTS];
static LootTable _lootTables[MAX_LOOTTABLES];

static int _itemCount;
static int _itemTypeCount;
static int _lootTableCount;
static int _lootSpawnX;
static int _lootSpawnY;

// ===== [[ Implementations ]] =====

void Item_loadFrom(const char* assetpath) {
    Ini_readAsset(assetpath);

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_itemCount == MAX_ITEMS) {
            Log_error("Max items exceeded");
            Ini_clear();
            return;
        }

        Item* item = &_items[_itemCount++];
        strncpy(item->name, name, NAME_LENGTH);

        const char* desc = Ini_get(name, "desc");
        if (!desc) desc = "No description.";
        strncpy(item->desc, desc, DESC_LENGTH);
        const char* displayName = Ini_get(name, "name");
        if (!displayName) displayName = item->name;
        strncpy(item->displayName, displayName, NAME_LENGTH);
        
        // todo: a more stateful ini parser would probably be better
        //       like Ini_setSection(name); Ini_get("type");
        item->type = ItemType_find(Ini_get(name, "type"));
        item->icon = Sprite_find(Ini_get(name, "icon"));
        item->price = String_parseInt(Ini_get(name, "price"), -1);

        ItemCategory category = _itemTypes[item->type].category;
        switch (category) {
            case ItemCategory_weapon: {
                item->prop1 = String_parseInt(Ini_get(name, "atk"), 5);
                item->skills[0] = Attack_find(Ini_get(name, "skill1"));
                item->skills[1] = Attack_find(Ini_get(name, "skill2"));
                item->skills[2] = Attack_find(Ini_get(name, "skill3"));
            } break;
            case ItemCategory_armor: {
                item->prop1 = String_parseInt(Ini_get(name, "def"), 5);
                item->prop2 = String_parseEnum(Ini_get(name, "equip"),
                    "none;weapon;head;body;accessory", 0);
                item->lck = String_parseInt(Ini_get(name, "lck"), 0);
            } break;
            case ItemCategory_food: {
                item->prop1 = String_parseInt(Ini_get(name, "hp"), 5);
            } break;
            case ItemCategory_potion: {
                item->prop1 = StatusEffect_find(Ini_get(name, "effect"));
            } break;
            default: break;
        }
    }

    Ini_clear();
}

void ItemType_loadFrom(const char* assetpath) {
    // This is separate from -1 because we can't loop over type -1
    // using ItemType_next
    // could possibly have inventory display manually also check -1?
    if (_itemTypeCount == 0) {
        _itemTypes[0] = (ItemType) {
            .name = "unknown",
            .category = ItemCategory_misc
        };
        _itemTypeCount++;
    }

    Ini_readAsset(assetpath);

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_itemTypeCount == MAX_ITEMTYPES) {
            Log_error("Max item types exceeded");
            Ini_clear();
            return;
        }

        ItemType* itemType = &_itemTypes[_itemTypeCount++];
        strncpy(itemType->name, name, NAME_LENGTH);
        
        itemType->category = String_parseEnum(Ini_get(name, "category"),
                "weapon;armor;food;ingredient;potion;misc", ItemCategory_misc);
    }

    Ini_clear();
}

ItemID Item_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _itemCount; i++) {
        if (strcmp(_items[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find item %s", name);
    return -1;
}

const char* Item_getName(ItemID self) {
    if (self < 0 || self > _itemCount) return 0;
    return _items[self].name;
}

const char* Item_getDisplayName(ItemID self) {
    if (self < 0 || self > _itemCount) return 0;
    return _items[self].displayName;
}

const char* Item_getDescription(ItemID self) {
    if (self < 0 || self > _itemCount) return 0;
    return _items[self].desc;
}

ItemTypeID Item_getType(ItemID self) {
    if (self < 0 || self > _itemCount) return 0;
    return _items[self].type;
}

SpriteID Item_getIcon(ItemID self) {
    if (self < 0 || self > _itemCount) return -1;
    return _items[self].icon;
}

int Item_getStat(ItemID self, Stat stat) {
    if (self < 0 || self > _itemCount) return 0;
    
    Item* item = &_items[self];
    ItemCategory category = _itemTypes[item->type].category;

    if (category == ItemCategory_armor && stat == Stat_def) {
        return item->prop1;
    } else if (category == ItemCategory_weapon && stat == Stat_atk) {
        return item->prop1;
    }

    if (stat == Stat_lck) return item->lck;

    return 0;
}

bool Item_isStackable(ItemID self) {
    if (self < 0 || self > _itemCount) return false;
    ItemCategory category = ItemType_getCategory(_items[self].type);
    if (category == ItemCategory_armor ||
            category == ItemCategory_weapon) {
        return false;
    }
    return true;
}

int Item_getMaxStack(ItemID self) {
    return Item_isStackable(self) ? 99 : 1;
}

EquipType Item_getEquipType(ItemID self) {
    if (self < 0 || self > _itemCount) return false;
    ItemCategory category = ItemType_getCategory(_items[self].type);
    if (category == ItemCategory_weapon) {
        return EquipType_weapon;
    } else if (category == ItemCategory_armor) {
        return _items[self].prop2;
    }
    return EquipType_none;
}

bool Item_isConsumable(ItemID self) {
    if (self < 0 || self > _itemCount) return false;
    ItemCategory category = ItemType_getCategory(_items[self].type);
    return category == ItemCategory_food || ItemCategory_potion;
}

bool Item_consume(ItemID self, int entity) {
    if (self < 0 || self > _itemCount) return false;
    Item* item = &_items[self];
    ItemCategory category = ItemType_getCategory(item->type);
    if (category == ItemCategory_food) {
        return Entity_heal(entity, item->prop1);
    } else if (category == ItemCategory_potion) {
        Entity_addStatusEffect(entity, item->prop1);
        return true;
    } else {
        return false;
    }
}

AttackID Item_getSkill(ItemID self, int index) {
    if (self < 0 || self > _itemCount) return -1;
    if (index < 0 || index > 2) return -1;
    return _items[self].skills[index];
}

int Item_getPrice(ItemID self) {
    if (self < 0 || self > _itemCount) return -1;
    return _items[self].price;
}

ItemTypeID ItemType_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _itemTypeCount; i++) {
        if (strcmp(_itemTypes[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find item type %s", name);
    return -1;
}

const char* ItemType_getName(ItemTypeID self) {
    if (self < 0 || self > _itemTypeCount) return "Unknown (Error2)";
    return _itemTypes[self].name;
}

ItemCategory ItemType_getCategory(ItemTypeID self) {
    if (self < 0 || self > _itemTypeCount) return ItemCategory_misc;
    return _itemTypes[self].category;
}

ItemTypeID ItemType_next(ItemCategory category, ItemTypeID prev) {
    if (prev < -1) prev = -1;
    for (int i = prev + 1; i < _itemTypeCount; i++) {
        if (_itemTypes[i].category == category) return i;
    }
    return -1;
}

void Inventory_clear(void) {
    for (int i = 0; i < MAX_INVSLOTS; i++) {
        _inventory[i].item = -1;
        _inventory[i].quantity = 0;
        _inventory[i].equip = EquipType_none;
    }

    // player starting gear
    _inventory[0].item = Item_find("WoodSword");
    _inventory[0].quantity = 1;
    _inventory[0].equip = EquipType_weapon;
    _inventory[1].item = Item_find("Clothes");
    _inventory[1].quantity = 1;
    _inventory[1].equip = EquipType_body;
}

bool Inventory_addAll(ItemID item, int quantity) {
    // get max stack size
    int maxStack = Item_getMaxStack(item);
    if (quantity > maxStack) return false;

    // if stackable, try to find an existing slot first
    if (Item_isStackable(item)) {
        for (int i = 0; i < MAX_INVSLOTS; i++) {
            InventorySlot* slot = &_inventory[i];
            if (slot->item != item) continue;
            if (slot->quantity + quantity <= maxStack) {
                slot->quantity += quantity;
                for (int j = 0; j < quantity; j++)
                    Quest_signalEvent(QuestEvent_collect, item);
                return true;
            } else {
                return false;
            }
        }
    }

    // try to place in empty slot
    for (int i = 0; i < MAX_INVSLOTS; i++) {
        InventorySlot* slot = &_inventory[i];
        if (slot->quantity == 0) {
            slot->item = item;
            slot->quantity = quantity;
            slot->equip = EquipType_none;
            for (int j = 0; j < quantity; j++)
                Quest_signalEvent(QuestEvent_collect, item);
            return true;
        }
    }
    
    return false;
}

bool Inventory_remove(SlotID slotID, int quantity) {
    if (slotID < 0 || slotID >= MAX_INVSLOTS) return false;
    InventorySlot* slot = &_inventory[slotID];

    if (Item_isStackable(slot->item)) {
        if (slot->quantity > quantity) {
            slot->quantity -= quantity;
        } else if (slot->quantity == quantity) {
            slot->quantity = 0;
            slot->equip = EquipType_none;
        } else return false;
    } else if (quantity == 1) {
        slot->quantity = 0;
        slot->equip = EquipType_none;
    } else return false;
    
    return true;
}

ItemID Inventory_getItem(SlotID slotID) {
    if (slotID < 0 || slotID >= MAX_INVSLOTS) return -1;
    if (_inventory[slotID].quantity == 0) return -1;
    return _inventory[slotID].item;
}

int Inventory_getQuantity(SlotID slotID) {
    if (slotID < 0 || slotID >= MAX_INVSLOTS) return -1;
    return _inventory[slotID].quantity;
}

EquipType Inventory_getEquipStatus(SlotID slotID) {
    if (slotID < 0 || slotID >= MAX_INVSLOTS) return -1;
    return _inventory[slotID].equip;
}

void Inventory_set(SlotID slotID, ItemID item, int quantity) {
    if (slotID < 0 || slotID >= MAX_INVSLOTS) return;
    _inventory[slotID].item = item;
    _inventory[slotID].quantity = quantity;
    _inventory[slotID].equip = EquipType_none;
}

// same as set, but maintain previous equip status
// used for item upgrades
void Inventory_replace(SlotID slotID, ItemID item, int quantity) {
    if (slotID < 0 || slotID >= MAX_INVSLOTS) return;
    _inventory[slotID].item = item;
    _inventory[slotID].quantity = quantity;
}

SlotID Inventory_findNext(ItemTypeID type, int prev) {
    if (prev < -1) prev = -1;
    for (int i = prev + 1; i < MAX_INVSLOTS; i++) {
        if (_inventory[i].quantity == 0) continue;
        if (Item_getType(_inventory[i].item) == type) return i;
    }
    return -1;
}

ItemID Inventory_getPlayerEquip(int equipType) {
    for (int i = 0; i < MAX_INVSLOTS; i++) {
        if (_inventory[i].quantity == 0) continue;
        if (_inventory[i].equip == equipType) {
            return _inventory[i].item;
        }
    }
    return -1;
}

void Inventory_equip(SlotID slotID) {
    if (slotID < 0 || slotID >= MAX_INVSLOTS) return;
    InventorySlot* slot = &_inventory[slotID];
    if (slot->quantity == 0) return;
    EquipType equipType = Item_getEquipType(slot->item);
    
    // if equip type used elsewhere, first unequip
    for (int i = 0; i < MAX_INVSLOTS; i++) {
        if (_inventory[i].quantity == 0) continue;
        if (_inventory[i].equip == equipType) {
            _inventory[i].equip = EquipType_none;
        }
    }

    // make this slot equipped
    slot->equip = equipType;
}

void Inventory_unequip(SlotID slotID) {
    if (slotID < 0 || slotID >= MAX_INVSLOTS) return;
    InventorySlot* slot = &_inventory[slotID];
    slot->equip = EquipType_none;
}

SlotID Inventory_find(ItemID item) {
    for (int i = 0; i < MAX_INVSLOTS; i++) {
        InventorySlot* slot = &_inventory[i];
        if (slot->quantity > 0 && slot->item == item) {
            return i;
        }
    }
    return -1;
}

int Inventory_getSize(void) {
    return MAX_INVSLOTS;
}

void Inventory_compact(void) {
    for (int i = 0; i < MAX_INVSLOTS; i++) {
        InventorySlot* slot = &_inventory[i];
        if (slot->quantity == 0) {
            for (int j = i+1; j < MAX_INVSLOTS; j++) {
                InventorySlot* slot2 = &_inventory[j];
                if (slot2->quantity > 0) {
                    *slot = *slot2;
                    slot2->quantity = 0;
                    break;
                }
            }
        }
    }
}

void LootTable_init(void) {
    Ini_readAsset("loottables.ini");

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_lootTableCount == MAX_LOOTTABLES) {
            Log_error("Max loot tables exceeded");
            Ini_clear();
            return;
        }

        LootTable* lootTable = &_lootTables[_lootTableCount++];
        strncpy(lootTable->name, name, NAME_LENGTH);
        
        // Read array of items
        lootTable->count = String_parseIntArrayExt(Ini_get(name, "items"),
            lootTable->items, MAX_LOOTITEMS, Item_find);

        // Read array of chances, default to 100%
        int count2 = String_parseIntArray(Ini_get(name, "chance"),
            lootTable->chance, MAX_LOOTITEMS);
        if (count2 > 0 && count2 != lootTable->count) {
            Log_warn("Chance count doesnt match amount of items in %s", name);
        }
        for (int i = count2; i < lootTable->count; i++) {
            lootTable->chance[i] = 100;
        }

        // Read array of quantities, default to 1
        count2 = String_parseIntArray(Ini_get(name, "quantity"),
            lootTable->quantity, MAX_LOOTITEMS);
        if (count2 > 0 && count2 != lootTable->count) {
            Log_warn("Quantity count doesnt match amount of items in %s", name);
        }
        for (int i = count2; i < lootTable->count; i++) {
            lootTable->quantity[i] = 1;
        }
    }

    Ini_clear();
}

LootTableID LootTable_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _lootTableCount; i++) {
        if (strcmp(_lootTables[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find loot table %s", name);
    return -1;
}

void LootTable_drop(LootTableID self, int x, int y) {
    if (self < 0 || self >= _lootTableCount) return;
    LootTable* lootTable = &_lootTables[self];
    
    for (int i = 0; i < lootTable->count; i++) {
        int dice = rand() % 100;
        if (lootTable->chance[i] <= dice) continue;

        for (int j = 0; j < lootTable->quantity[i]; j++) {
            Entity_dropItem(x, y, lootTable->items[i]);
        }
    }
}
