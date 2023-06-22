#include "common.h"

#define CUTE_TILED_NO_EXTERNAL_TILESET_WARNING
#define CUTE_TILED_IMPLEMENTATION
#include "zz_tiled.h"

// ===== [[ Defines ]] =====

#define MAX_EDGELOOP_VERTICES 1024

// ===== [[ Local Types ]] =====

typedef enum {
    Edge_left = 1 << 0,
    Edge_right = 1 << 1,
    Edge_up = 1 << 2,
    Edge_down = 1 << 3,
    Edge_leftSeen = 1 << 4,
    Edge_rightSeen = 1 << 5,
    Edge_upSeen = 1 << 6,
    Edge_downSeen = 1 << 7
} Edge;

typedef struct {
    bool loaded;
    int width;
    int height;
    short* bg1;
    short* bg2;
    short* fg;
    short* obj;
    short* col;
    //LootTable* vaseLootTables[4];
    //LootTable* chestLootTables[4];
    RegionLogicParams logicParams[4];
    int exits[4];
    bool dark; // always dark?
    int mode; // 1: old test, 2: show tsedit
} Region;

// ===== [[ Declarations ]] =====

static void _buildField(void);
static void _buildEdgeLoop(unsigned char* adjs, int sx, int sy, int se);

// ===== [[ Static Data ]] =====

static SDL_Texture* _texTileset;
Region _region;

// ===== [[ Implementations ]] =====

void Region_load(int id) {
    // read region ini
    char inipath[256];
    snprintf(inipath, 256, "regions/region%02d.ini", id);
    Ini_readAsset(inipath);
    int mode = String_parseInt(Ini_get("", "mode"), 1);
    Ini_clear();

    // read tiled map
    char filepath[256];
    snprintf(filepath, 256, "assets/regions/region%02d.json", id);
    
    cute_tiled_map_t* map = cute_tiled_load_map_from_file(filepath, NULL);
    if (map == NULL) {
        printf("failed to load region %d\n", id);
        return;
    }

    Region* region = &_region;

    region->loaded = true;
    region->width = map->width;
    region->height = map->height;
    region->mode = mode;
    // todo: size check?

    if (mode == 1) {
        cute_tiled_layer_t* layer = map->layers;
        while (layer) {
            short** dst = NULL;
            if (strcmp(layer->name.ptr, "bg") == 0) {
                dst = &region->bg1;
            } else if (strcmp(layer->name.ptr, "bg2") == 0) {
                dst = &region->bg2;
            } else if (strcmp(layer->name.ptr, "obj") == 0) {
                dst = &region->obj;
            } else if (strcmp(layer->name.ptr, "fg") == 0) {
                dst = &region->fg;
            } else if (strcmp(layer->name.ptr, "col") == 0) {
                dst = &region->col;
            } else {
                layer = layer->next;
                continue;
            }

            int size = region->width * region->height;
            short* data = calloc(size, sizeof(short));
            for (int i = 0; i < size; i++) {
                data[i] = (short) layer->data[i];
            }
            *dst = data;

            layer = layer->next;
        }
    } else if (mode == 2 || mode == 3) {
        cute_tiled_layer_t* layer = map->layers;

        while (layer) {
            if (strcmp(layer->type.ptr, "tilelayer") == 0) {
                if (mode == 2 || strcmp(layer->name.ptr, "ground") == 0) {
                    int size = region->width * region->height;
                    if (mode == 2) region->bg1 = calloc(size, sizeof(short));
                    region->col = calloc(size, sizeof(short));
                    for (int i = 0; i < size; i++) {
                        int tile = layer->data[i];
                        if (mode == 2) region->bg1[i] = tile;
                        region->col[i] = tile <= 16 || tile == 29;
                    }
                } else {
                    short** dst = NULL;
                    if (strcmp(layer->name.ptr, "bg") == 0) {
                        dst = &region->bg1;
                    } else if (strcmp(layer->name.ptr, "bg2") == 0) {
                        dst = &region->bg2;
                    } else if (strcmp(layer->name.ptr, "fg") == 0) {
                        dst = &region->fg;
                    }

                    if (dst) {
                        int size = region->width * region->height;
                        short* data = calloc(size, sizeof(short));
                        int baseTile = layer->data[0] - 1;
                        for (int i = 0; i < size; i++) {
                            short tstile = layer->data[i];
                            if (tstile > baseTile) tstile -= baseTile;
                            data[i] = tstile;
                        }
                        *dst = data;
                    }
                }
            } else if (strcmp(layer->type.ptr, "objectgroup") == 0) {
                cute_tiled_object_t* obj = layer->objects;
                while (obj) {
                    // Log_debug("object '%s' (%s) at %f,%f",
                    //     obj->name.ptr,
                    //     obj->type.ptr,
                    //     obj->x, obj->y);
                    if (obj->gid) {
                        int prefab = Entity_findPrefab(obj->type.ptr);
                        int eid = Entity_spawn(obj->x * 16, obj->y * 16, prefab);
                        for (int i = 0; i < obj->property_count; i++) {
                            cute_tiled_property_t* prop = &obj->properties[i];
                            if (strcmp(prop->name.ptr, "npcName") == 0) {
                                VillagerID villagerID = Villager_find(prop->data.string.ptr);
                                Entity_setVillager(eid, villagerID);
                            } else if (strcmp(prop->name.ptr, "invitems") == 0) {
                                ItemID items[16];
                                int miniInvCount = String_parseIntArrayExt(
                                    prop->data.string.ptr, items, 16, Item_find);
                                for (int i = miniInvCount; i < 16; i++) {
                                    items[i] = -1;
                                }
                                Entity_setMiniInvItems(eid, items);
                            } else if (strcmp(prop->name.ptr, "invqty") == 0) {
                                int qty[16] = {0};
                                int c2 = String_parseIntArray(
                                    prop->data.string.ptr, qty, 16);
                                for (int i = c2; i < 16; i++) {
                                    qty[i] = 1;
                                }
                                Entity_setMiniInvQty(eid, qty);
                            } else if (strcmp(prop->name.ptr, "hintText") == 0) {
                                // spaces are messed up
                                // so use underscores instead and replace here
                                char buf[128];
                                strncpy(buf, prop->data.string.ptr, 128);
                                for (int i = 0; i < 128; i++) {
                                    if (buf[i] == '_') buf[i] = ' ';
                                }
                                Entity_setHintText(eid, buf);
                            } else if (strcmp(prop->name.ptr, "hintRadius") == 0) {
                                Entity_setHintRadius(eid, prop->data.integer * 16);
                            }
                        }
                    }
                    obj = obj->next;
                }
            } else {
                Log_warn("Ignoring layer %s", layer->name.ptr);
            }
            layer = layer->next;
        }

        // tall grass
        for (int y = 0; y < region->height; y++) {
            for (int x = 0; x < region->width; x++) {
                if (region->bg1[y * region->width + x] == 18) {
                    // todo: access may be out of bounds, but okay as long
                    // as edge tile is never grass
                    bool has_left = region->bg1[y*region->width+x-1] == 18;
                    bool has_right = region->bg1[y*region->width+x+1] == 18;
                    int side = 0;
                    if (!has_left) side += 1;
                    if (!has_right) side += 2;
                    Game_addGrass(x, y, side);
                }
            }
        }
    }

    if (mode == 1) {
        _texTileset = Loading_loadTexture("assets/images/tiles_dungeon_v1.1.bmp");
    } else if (mode == 2) {
        _texTileset = Loading_loadTexture("assets/regions/_tsedit.bmp");
    } else if (mode == 3) {
        _texTileset = Loading_loadTexture("assets/images/tsDEMO.bmp");
    }

    _buildField();

    // build navmesh
    // todo: this is mostly repeated from buildField
    bool solids[REGION_WIDTH * REGION_HEIGHT];
    for (int y = 0; y < REGION_HEIGHT; y++) {
        for (int x = 0; x < REGION_WIDTH; x++) {
            solids[y * REGION_WIDTH + x] = Region_isTileSolid(x, y);
        }
    }
    NavGrid_set(REGION_WIDTH, REGION_HEIGHT, solids);
}

void Region_render(int layerID) {
    const short* layer = NULL;
    switch (layerID) {
        case 0: layer = _region.bg1; break;
        case 1: layer = _region.bg2; break;
        case 2: layer = _region.fg; break;
    }
    if (layer == NULL) return;

    // tileset width
    int tsw = _region.mode == 1 ? 20 : 16;

    for (int x = 0; x < _region.width; x++) {
        for (int y = 0; y < _region.height; y++) {
            int id = layer[y * _region.width + x];
            if (id == 0) continue;
            if (id < 0) id += 256;
            SDL_Rect src = { ((id-1)%tsw)*16, ((id-1)/tsw)*16, 16, 16 };
            SDL_Rect dst = { x * 16, y * 16, 16, 16 };
            Draw_translatePoint(&dst.x, &dst.y);
            SDL_RenderCopy(Main_renderer, _texTileset, &src, &dst);
        }
    }
}

bool Region_isTileSolid(int tx, int ty) {
    if (tx < 0 || tx >= _region.width) return true;
    if (ty < 0 || ty >= _region.height) return true;

    int id = _region.col[ty * _region.width + tx];
    return id != 0;
}

// build all edge loops into field
static void _buildField(void) {
    // mark all solids
    unsigned char solids[REGION_WIDTH * REGION_HEIGHT];
    for (int y = 0; y < REGION_HEIGHT; y++) {
        for (int x = 0; x < REGION_WIDTH; x++) {
            solids[y * REGION_WIDTH + x] = Region_isTileSolid(x, y);
        }
    }

    // check adjacents for non-solids
    unsigned char adjs[REGION_WIDTH * REGION_HEIGHT];
    for (int y = 0; y < REGION_HEIGHT; y++) {
        for (int x = 0; x < REGION_WIDTH; x++) {
            // for solids we mark no edges, edges only on non-solid tiles
            if (solids[y * REGION_WIDTH + x]) {
                adjs[y * REGION_WIDTH + x] = 0;
                continue;
            } 

            // set adjacent bitfield
            unsigned char result = 0;
            if (x == 0) result |= Edge_left;
            else if (solids[y * REGION_WIDTH + (x - 1)]) result |= Edge_left;
            if (x == REGION_WIDTH - 1) result |= Edge_right;
            else if (solids[y * REGION_WIDTH + (x + 1)]) result |= Edge_right;
            if (y == 0) result |= Edge_up;
            else if (solids[(y - 1) * REGION_WIDTH + x]) result |= Edge_up;
            if (y == REGION_WIDTH - 1) result |= Edge_down;
            else if (solids[(y + 1) * REGION_WIDTH + x]) result |= Edge_down;
            adjs[y * REGION_WIDTH + x] = result;
        }
    }

    // build edge loops
    for (int y = 0; y < REGION_HEIGHT; y++) {
        for (int x = 0; x < REGION_WIDTH; x++) {
            // skip if no unseen edges
            int tadj = adjs[y * REGION_WIDTH + x];
            if ((tadj & 15) == tadj >> 4) continue;

            if ((tadj & Edge_left) && !(tadj & Edge_leftSeen)) {
                _buildEdgeLoop(adjs, x, y, 0);
                // recompute tadj for next if stmt
                tadj = adjs[y * REGION_WIDTH + x];
            }
            if ((tadj & Edge_up) && !(tadj & Edge_upSeen)) {
                _buildEdgeLoop(adjs, x, y, 1);
                tadj = adjs[y * REGION_WIDTH + x];
            }
            if ((tadj & Edge_right) && !(tadj & Edge_rightSeen)) {
                _buildEdgeLoop(adjs, x, y, 2);
                tadj = adjs[y * REGION_WIDTH + x];
            }
            if ((tadj & Edge_down) && !(tadj & Edge_downSeen)) {
                _buildEdgeLoop(adjs, x, y, 3);
            }
        }
    }
}

// lut for edge masks and rotating edges
// todo: maybe call the clockwise-indexed edges 'sides' instead?
static const Edge edgeMask[] = {
    Edge_left, Edge_up, Edge_right, Edge_down
};
static const char* edgeNames[] = { "left", "up", "right", "down" };

// tile offset for checking along straight edge
static const int dx1[] = { 0, 1, 0, -1 };
static const int dy1[] = { -1, 0, 1, 0 };

// tile offset for checking around reflex angle
static const int dx2[] = { -1, 1, 1, -1 };
static const int dy2[] = { -1, -1, 1, 1 };

// base edge vertex offset
static const int bex[] = { 0, 0, 1, 1 };
static const int bey[] = { 1, 0, 0, 1 };

// build a single edge loop into the field
static void _buildEdgeLoop(unsigned char* adjs, int sx, int sy, int se) {
    // current position
    int x = sx, y = sy;
    int edge = se;

    // edgeloop
    int vtxCount = 0;
    FieldPoint points[MAX_EDGELOOP_VERTICES];

    while (true) {
        // mark edge as seen
        // Log_debug("MARK %d,%d as %s", x, y, edgeNames[edge]);
        adjs[y * REGION_WIDTH + x] |= edgeMask[edge] << 4;

        // find next edge counter-clockwise
        bool emit = true;
        if (adjs[y * REGION_WIDTH + x] & edgeMask[(edge + 1) % 4]) {
            // acute vertex in same tile
            edge = (edge + 1) % 4;

            // emit vertex
            if (vtxCount == MAX_EDGELOOP_VERTICES) {
                Log_warn("exceeded edgeloop vtx limit");
                return;
            }
        } else if (adjs[(y + dy1[edge]) * REGION_WIDTH + (x+dx1[edge])] &
            edgeMask[edge]
        ) {
            // extend current edge
            x += dx1[edge];
            y += dy1[edge];

            // note: never emit in this case, we want to extend
            // we assume that the edge loop started in a corner
            // (otherwise this wont emit the final vtx)
            // should be okay, _buildField should never invoke this
            // from a non-corner tile
            emit = false;
        } else if (adjs[(y + dy2[edge]) * REGION_WIDTH + (x+dx2[edge])] &
            edgeMask[(edge + 3) % 4]
        ) {
            // reflex vertex
            x += dx2[edge];
            y += dy2[edge];
            edge = (edge + 3) % 4;
        } else {
            // no continuation for edge loop
            Log_warn("failed to complete edge loop, should be impossible");
            return;
        }

        // emit vertex
        if (emit) {
            if (vtxCount == MAX_EDGELOOP_VERTICES) {
                Log_warn("exceeded edgeloop vtx limit");
                return;
            }
            // Log_debug("EMIT %d,%d", x + bex[edge], y + bey[edge]);
            points[vtxCount].x = (x + bex[edge]) * 256;
            points[vtxCount].y = (y + bey[edge]) * 256;
            vtxCount++;
        }

        // exit if loop complete
        if (adjs[y * REGION_WIDTH + x] & edgeMask[edge] << 4) {
            break;
        }
    }

    // build edgeloop into field
    Field_addPolygon(vtxCount, points, true);
}
