#include "SDL_render.h"
#include "common.h"

// ===== [[ Defines ]] =====

#define MAX_SPRITE_IMAGES 32
#define MAX_SPRITES 512
#define MAX_ANIMATIONS 256
#define MAX_QUEUE_ENTRIES 2048

// ===== [[ Local Types ]] =====

typedef struct {
    char assetpath[MAX_ASSETPATH_LENGTH];
    SDL_Texture* texture;
} SpriteImage;

typedef int SpriteImageID;

typedef struct {
    char name[NAME_LENGTH];
    int image;
    int x, y;
    int w, h;
    int ox, oy; // origin for drawing
    SDL_RendererFlip flip;
} Sprite;

typedef struct {
    char name[NAME_LENGTH];
    short frameCount;
    bool isPingPong;
    int frameSprites[8];
    int frameDurations[8]; // in 16ms units (i.e. 60fps frames)
    int totalDuration;
    int forwardDuration; // for ping pong only, duration minus last frame
    int reverseDelta; // for ping pong only, time offset for reverse
} Animation;

typedef enum {
    QueueEntryKind_sprite,
    QueueEntryKind_animation
} QueueEntryKind;

typedef struct {
    QueueEntryKind kind;
    int x;
    int y;
    int z;
    bool hasModColor;
    int modColor[3];
    union {
        struct {
            SpriteID spriteID;
        } asSprite;
        struct {
            AnimationID animationID;
            int time;
        } asAnimation;
    } data;
} QueueEntry;

// ===== [[ Declarations ]] =====

static SpriteImageID _findSpriteImage(const char* assetpath);
static QueueEntry* _queueAllocate(QueueEntryKind kind, int x, int y, int z);

// ===== [[ Static Data ]] =====

static SpriteImage _spriteImages[MAX_SPRITE_IMAGES];
static Sprite _sprites[MAX_SPRITES];
static Animation _animations[MAX_ANIMATIONS];

static int _spriteImageCount;
static int _spriteCount;
static int _animationCount;
static int _animationGlobalTimer;

static bool _hasModColor;
static int _modColor[3];

static QueueEntry _queueEntries[MAX_QUEUE_ENTRIES];
static int _queueEntryCount;
static bool _maxQueueWarningShown;

// ===== [[ Implementations ]] =====

void Sprite_setSpriteImage(const char* assetpath, SDL_Texture* texture) {
    if (_spriteImageCount == MAX_SPRITE_IMAGES) {
        puts("error: too many sprite images");
        return;
    }

    SpriteImageID id = _spriteImageCount++;
    strncpy(_spriteImages[id].assetpath, assetpath, MAX_ASSETPATH_LENGTH);
    _spriteImages[id].texture = texture;
}

void Sprite_loadFrom(const char* assetpath) {
    Ini_readAsset(assetpath);

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_spriteCount == MAX_SPRITES) {
            Log_error("Max sprites exceeded");
            Ini_clear();
            return;
        }

        Sprite* sprite = &_sprites[_spriteCount++];
        strncpy(sprite->name, name, NAME_LENGTH);
        char imagePath[MAX_ASSETPATH_LENGTH];
        snprintf(imagePath, MAX_ASSETPATH_LENGTH,
                "%s", Ini_get(name, "image"));
        sprite->image = _findSpriteImage(imagePath);

        sprite->w = 16;
        sprite->h = 16;
        String_parseInt2(Ini_get(name, "offset"), &sprite->x, &sprite->y);
        String_parseInt2(Ini_get(name, "size"), &sprite->w, &sprite->h);
        String_parseInt2(Ini_get(name, "origin"), &sprite->ox, &sprite->oy);

        sprite->flip = String_parseEnum(Ini_get(name, "flip"),
            "none;x;y;xy", 0);

        // todo: mark ini fields on access, then call some validate function
        //       that warns for any ini field not accessed (likely typo)
        //       Ini_warnUnusedIn(section)?
    }

    Ini_clear();
}

void Animation_loadFrom(const char* assetpath) {
    Ini_readAsset(assetpath);

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_animationCount == MAX_ANIMATIONS) {
            Log_error("Max animations exceeded");
            Ini_clear();
            return;
        }

        Animation* animation = &_animations[_animationCount++];
        strncpy(animation->name, name, NAME_LENGTH);
        
        animation->isPingPong =
                String_parseBool(Ini_get(name, "pingpong"), false);
        
        const char* spritesString = Ini_get(name, "sprites");
        animation->frameCount = String_parseIntArrayExt(
            spritesString, animation->frameSprites, 8, Sprite_find);

        const char* durationsString = Ini_get(name, "durations");
        int durationsCount = String_parseIntArray(
            durationsString, animation->frameDurations, 8);
        if (durationsCount < animation->frameCount) {
            for (int j = durationsCount; j < animation->frameCount; j++) {
                animation->frameDurations[j] = 1;
            }
        }

        int total = 0;
        for (int i = 0; i < animation->frameCount; i++) {
            total += animation->frameDurations[i];
        }
        animation->totalDuration = total;

        if (animation->isPingPong && total > 0) {
            animation->forwardDuration =
                    total - animation->frameDurations[animation->frameCount - 1];
            animation->reverseDelta = animation->forwardDuration - 1 + total;

            animation->totalDuration = animation->forwardDuration +
                    total - animation->frameDurations[0];
        }
    }

    Ini_clear();
}

SpriteID Sprite_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _spriteCount; i++) {
        if (strcmp(_sprites[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find sprite %s", name);
    return -1;
}

void Sprite_draw(SpriteID self, int x, int y) {
    Sprite_drawScaled(self, x, y, -1, -1);
}

void Sprite_drawScaled(SpriteID self, int x, int y, int w, int h) {
    if (self < 0 || self > _spriteCount) return;
    Sprite* sprite = &_sprites[self];
    if (sprite->image == -1) return;
    SDL_Texture* texture = _spriteImages[sprite->image].texture;

    if (!texture) return;
    if (_hasModColor) SDL_SetTextureColorMod(texture,
            _modColor[0], _modColor[1], _modColor[2]);
    SDL_Rect src = { sprite->x, sprite->y, sprite->w, sprite->h };
    if (w == -1) w = sprite->w;
    if (h == -1) h = sprite->h;
    SDL_Rect dst = { x - sprite->ox, y - sprite->oy, w, h };
    Draw_translatePoint(&dst.x, &dst.y);
    SDL_RenderCopyEx(Main_renderer, texture, &src, &dst, 0, NULL, sprite->flip);
    if (_hasModColor) SDL_SetTextureColorMod(texture, 255, 255, 255);
}

AnimationID Animation_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _animationCount; i++) {
        if (strcmp(_animations[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find animation %s", name);
    return -1;
}

int Animation_getDuration(AnimationID self) {
    if (self < 0 || self > _animationCount) return 0;
    return _animations[self].totalDuration;
}

void Animation_draw(AnimationID self, int x, int y, int* timePointer) {
    if (self < 0 || self > _animationCount) return;
    Animation* animation = &_animations[self];

    int time;
    if (Game_shouldAnimate()) {
        time = timePointer ? (*timePointer)++ : _animationGlobalTimer;
    } else {
        time = timePointer ? *timePointer : _animationGlobalTimer;
    }
    time = time % animation->totalDuration;
    if (animation->isPingPong && time > animation->forwardDuration) {
        time = animation->reverseDelta - time;
    }

    int baseTime = time;

    int frame = 0;
    while (frame < 8 && animation->frameDurations[frame] <= time) {
        time -= animation->frameDurations[frame];
        frame++;
    }

    Sprite_draw(animation->frameSprites[frame], x, y);
}

void Animation_update(void) {
    _animationGlobalTimer++;
}

void Graphics_setModulationColor(int r, int g, int b) {
    _hasModColor = true;
    _modColor[0] = r;
    _modColor[1] = g;
    _modColor[2] = b;
}

void Graphics_clearModulationColor(void) {
    _hasModColor = false;
}

void SpriteQueue_clear(void) {
    _queueEntryCount = 0;
}

void SpriteQueue_addSprite(SpriteID spriteID, int x, int y, int z) {
    QueueEntry* entry = _queueAllocate(QueueEntryKind_sprite, x, y, z);
    if (!entry) return;
    entry->data.asSprite.spriteID = spriteID;
}

void SpriteQueue_addAnim(AnimationID animationID, int x, int y, int z, int* time) {
    QueueEntry* entry = _queueAllocate(QueueEntryKind_animation, x, y, z);
    if (!entry) return;
    entry->data.asAnimation.animationID = animationID;
    entry->data.asAnimation.time = *time;
    if (Game_shouldAnimate()) (*time)++;
}

void SpriteQueue_render(void) {
    // Selection sort entries into order
    int smallestIndex;
    int smallestY;
    for (int i = 0; i < _queueEntryCount; i++) {
        smallestIndex = i;
        smallestY = _queueEntries[i].y;
        for (int j = i + 1; j < _queueEntryCount; j++) {
            int jY = _queueEntries[j].y;
            if (jY < smallestY) {
                smallestIndex = j;
                smallestY = jY;
            }
        }
        if (smallestIndex != i) {
            QueueEntry iCopy = _queueEntries[i];
            _queueEntries[i] = _queueEntries[smallestIndex];
            _queueEntries[smallestIndex] = iCopy;
        }
    }

    // Clear drawing state
    _hasModColor = false;

    // Draw entries in this order
    for (int i = 0; i < _queueEntryCount; i++) {
        QueueEntry *entry = &_queueEntries[i];
        int x = entry->x;
        int y = entry->y - entry->z;
        if (entry->hasModColor) {
            _hasModColor = true;
            memcpy(_modColor, entry->modColor, sizeof(int[3]));
        }
        if (entry->kind == QueueEntryKind_sprite) {
            Sprite_draw(entry->data.asSprite.spriteID, x, y);
        } else if (entry->kind == QueueEntryKind_animation) {
            Animation_draw(entry->data.asAnimation.animationID, x, y,
                &entry->data.asAnimation.time);
        }
        _hasModColor = false;
    }
}

static SpriteImageID _findSpriteImage(const char* assetpath) {
    for (int i = 0; i < _spriteImageCount; i++) {
        if (strcmp(_spriteImages[i].assetpath, assetpath) == 0) return i;
    }

    // if failed, load and try again
    Loading_loadSpriteImage(assetpath);
    for (int i = 0; i < _spriteImageCount; i++) {
        if (strcmp(_spriteImages[i].assetpath, assetpath) == 0) return i;
    }
    return -1;
}

static QueueEntry* _queueAllocate(QueueEntryKind kind, int x, int y, int z) {
    if (_queueEntryCount == MAX_QUEUE_ENTRIES) {
        // todo: some kind of WARN_ONCE macro for this?
        if (!_maxQueueWarningShown) {
            Log_warn("max sprite queue sized reached");
            _maxQueueWarningShown = true;
        }

        return NULL;
    }

    QueueEntry* result = &_queueEntries[_queueEntryCount++];
    result->kind = kind;
    result->x = x;
    result->y = y;
    result->z = z;
    result->hasModColor = _hasModColor;
    if (_hasModColor) {
        memcpy(result->modColor, _modColor, sizeof(int[3]));
    }
    return result;
}
