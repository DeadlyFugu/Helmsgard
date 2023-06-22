#include "common.h"

// ===== [[ Defines ]] =====

#define MAX_RECIPES 256
#define MAX_RECIPE_INPUTS 8

// ===== [[ Local Types ]] =====

typedef struct {
    char name[NAME_LENGTH];
    ItemID output;
    int outputQuantity;
    int inputCount;
    ItemID inputs[MAX_RECIPE_INPUTS];
    int inputQuantities[MAX_RECIPE_INPUTS];
    ItemID upgrade;
    bool unlock;
} Recipe;

// ===== [[ Declarations ]] =====

// ===== [[ Static Data ]] =====

static Recipe _recipes[MAX_RECIPES];
static int _recipeCount;

// ===== [[ Implementations ]] =====

void Recipe_load(void) {
    Ini_readAsset("recipes.ini");

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_recipeCount == MAX_RECIPES) {
            Log_error("Max recipes exceeded");
            Ini_clear();
            return;
        }

        Recipe* recipe = &_recipes[_recipeCount++];
        strncpy(recipe->name, name, NAME_LENGTH);

        recipe->output = Item_find(Ini_get(name, "output"));
        recipe->outputQuantity = String_parseInt(Ini_get(name, "outputQty"), 1);
        recipe->inputCount = String_parseIntArrayExt(Ini_get(name, "inputs"),
            recipe->inputs, MAX_RECIPE_INPUTS, Item_find);
        int count2 = String_parseIntArray(Ini_get(name, "quantity"),
            recipe->inputQuantities, MAX_RECIPE_INPUTS);
        if (count2 > 0 && count2 != recipe->inputCount) {
            Log_warn("Quantity count doesnt match amount of items in %s", name);
        }
        for (int i = count2; i < recipe->inputCount; i++) {
            recipe->inputQuantities[i] = 1;
        }
        recipe->upgrade = Item_find(Ini_get(name, "upgrade"));
        recipe->unlock = String_parseBool(Ini_get(name, "unlock"), false);
    }

    Ini_clear();
}

RecipeID Recipe_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _recipeCount; i++) {
        if (strcmp(_recipes[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find recipe %s", name);
    return -1;
}

RecipeID Recipe_next(RecipeID recipe) {
    if (recipe < -1 || recipe >= _recipeCount - 1) return -1;
    return recipe + 1;
}

ItemID Recipe_getOutput(RecipeID recipe) {
    if (recipe < 0 || recipe >= _recipeCount) return -1;
    return _recipes[recipe].output;
}

int Recipe_getOutputQuantity(RecipeID recipe) {
    if (recipe < 0 || recipe >= _recipeCount) return -1;
    return _recipes[recipe].outputQuantity;
}

ItemID Recipe_getInput(RecipeID recipe, int i) {
    if (recipe < 0 || recipe >= _recipeCount) return -1;
    if (i < 0 || i > _recipes[recipe].inputCount) return -1;
    return _recipes[recipe].inputs[i];
}

int Recipe_getInputQuantity(RecipeID recipe, int i) {
    if (recipe < 0 || recipe >= _recipeCount) return 0;
    if (i < 0 || i > _recipes[recipe].inputCount) return 0;
    return _recipes[recipe].inputQuantities[i];
}

ItemID Recipe_getBase(RecipeID recipe) {
    if (recipe < 0 || recipe >= _recipeCount) return -1;
    return _recipes[recipe].upgrade;
}

bool Recipe_isCraftable(RecipeID recipe) {
    if (recipe < 0 || recipe >= _recipeCount) return false;
    Recipe* data = &_recipes[recipe];
    for (int i = 0; i < data->inputCount; i++) {
        SlotID slot = Inventory_find(data->inputs[i]);
        if (slot == -1) return false;
        if (Inventory_getQuantity(slot) < data->inputQuantities[i]) {
            return false;
        }
    }
    if (data->upgrade != -1) {
        if (Inventory_find(data->upgrade) == -1) return false;
    }
    return true;
}

void Recipe_craft(RecipeID recipe) {
    if (!Recipe_isCraftable(recipe)) return;
    Recipe* data = &_recipes[recipe];
    for (int i = 0; i < data->inputCount; i++) {
        SlotID slot = Inventory_find(data->inputs[i]);
        Inventory_remove(slot, data->inputQuantities[i]);
    }
    if (data->upgrade != -1) {
        SlotID slot = Inventory_find(data->upgrade);
        if (Inventory_getQuantity(slot) == 1) {
            Inventory_replace(slot, data->output, 1);
        } else {
            Inventory_remove(slot, 1);
            Inventory_addAll(data->output, 1);
        }
    } else {
        Inventory_addAll(data->output, data->outputQuantity);
    }
}

void Recipe_unlock(RecipeID recipe) {
    if (recipe < 0 || recipe >= _recipeCount) return;
    _recipes[recipe].unlock = true;
}

bool Recipe_isUnlocked(RecipeID recipe) {
    if (recipe < 0 || recipe >= _recipeCount) return false;
    return _recipes[recipe].unlock;
}
