#include "common.h"

// ===== [[ Defines ]] =====

#define MAX_PARTICLE_SYSTEMS 64
#define MAX_PARTICLE_INSTANCES 64
#define MAX_PARTICLES_PER_INSTANCE 32
#define MAX_PARTICLE_SPRITES 8

// ===== [[ Local Types ]] =====

typedef struct {
    char name[NAME_LENGTH];
    int spriteCount;
    SpriteID sprites[MAX_PARTICLE_SPRITES];
    bool animated; // if false, choose sprite randomly
    int duration;
    int amount; // amount to spawn
    int spawnRadius; // radius around center to randomly spawn in
    float minVelX, maxVelX, minVelY, maxVelY; // starting velocity
    float accX, accY; // constant acceleration
    float drag; // constant friction, 0-1
} ParticleSystem;

typedef struct {
    bool valid;
    ParticlesID system;
    int age;
    int particleCount;
    int xbase, ybase, zbase;
    float xs[MAX_PARTICLES_PER_INSTANCE];
    float ys[MAX_PARTICLES_PER_INSTANCE];
    float vxs[MAX_PARTICLES_PER_INSTANCE];
    float vys[MAX_PARTICLES_PER_INSTANCE];
} ParticleInstance;

// ===== [[ Declarations ]] =====

static float _randFloatBetween(float min, float max);

// ===== [[ Static Data ]] =====

static ParticleSystem _particleSystems[MAX_PARTICLE_SYSTEMS];
static int _particleSystemCount;
static ParticleInstance _particleInstances[MAX_PARTICLE_INSTANCES];

// ===== [[ Implementations ]] =====

void Particles_load(void) {
    Ini_readAsset("particles.ini");

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_particleSystemCount == MAX_PARTICLE_SYSTEMS) {
            Log_error("Max particle systems exceeded");
            Ini_clear();
            return;
        }

        ParticleSystem* particles = &_particleSystems[_particleSystemCount++];
        strncpy(particles->name, name, NAME_LENGTH);

        particles->spriteCount = String_parseIntArrayExt(
            Ini_get(name, "sprites"), particles->sprites,
            MAX_PARTICLE_SPRITES, Sprite_find);
        
        particles->animated = String_parseBool(Ini_get(name, "animated"), false);
        particles->duration = String_parseInt(Ini_get(name, "duration"), 60);
        particles->amount = String_parseInt(Ini_get(name, "amount"), 10);
        particles->spawnRadius = String_parseInt(Ini_get(name, "spawnRadius"), 0);
        particles->minVelX = String_parseFloat(Ini_get(name, "minVelX"), 0);
        particles->maxVelX = String_parseFloat(Ini_get(name, "maxVelX"), 0);
        particles->minVelY = String_parseFloat(Ini_get(name, "minVelY"), 0);
        particles->maxVelY = String_parseFloat(Ini_get(name, "maxVelY"), 0);
        particles->accX = String_parseFloat(Ini_get(name, "accX"), 0);
        particles->accY = String_parseFloat(Ini_get(name, "accY"), 0);
        particles->drag = String_parseFloat(Ini_get(name, "drag"), 0);
    }

    Ini_clear();
}

ParticlesID Particles_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _particleSystemCount; i++) {
        if (strcmp(_particleSystems[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find particle system %s", name);
    return -1;
}

void Particles_spawn(ParticlesID particles, int x, int y, int z) {
    if (particles == -1) return;
    int instId = -1;
    for (int i = 0; i < MAX_PARTICLE_INSTANCES; i++) {
        if (!_particleInstances[i].valid) {
            instId = i;
            break;
        }
    }
    if (instId == -1) return;

    ParticleInstance* instance = &_particleInstances[instId];
    instance->valid = true;
    instance->system = particles;
    instance->age = 0;
    instance->xbase = x / 16;
    instance->ybase = y / 16;
    instance->zbase = z;

    ParticleSystem* system = &_particleSystems[particles];
    instance->particleCount = system->amount;
    if (instance->particleCount > MAX_PARTICLES_PER_INSTANCE) {
        instance->particleCount = MAX_PARTICLES_PER_INSTANCE;
    }

    float halfRadius = system->spawnRadius / 2.0f;
    for (int i = 0; i < instance->particleCount; i++) {
        instance->xs[i] = _randFloatBetween(-halfRadius, halfRadius);
        instance->ys[i] = _randFloatBetween(-halfRadius, halfRadius);
        instance->vxs[i] = _randFloatBetween(system->minVelX, system->maxVelX);
        instance->vys[i] = _randFloatBetween(system->minVelY, system->maxVelY);
    }
}

void Particles_update(void) {
    for (int i = 0; i < MAX_PARTICLE_INSTANCES; i++) {
        ParticleInstance* instance = &_particleInstances[i];
        ParticleSystem* system = &_particleSystems[instance->system];
        if (!instance->valid) continue;

        if (instance->age >= system->duration) {
            instance->valid = false;
            continue;
        } else {
            instance->age++;
        }

        float dragFactor = 1 - system->drag;
        for (int i = 0; i < instance->particleCount; i++) {
            instance->xs[i] += instance->vxs[i];
            instance->ys[i] += instance->vys[i];
            instance->vxs[i] = (instance->vxs[i] + system->accX) * dragFactor;
            instance->vys[i] = (instance->vys[i] + system->accY) * dragFactor;
        }
    }
}

void Particles_draw(void) {
    for (int i = 0; i < MAX_PARTICLE_INSTANCES; i++) {
        ParticleInstance* instance = &_particleInstances[i];
        ParticleSystem* system = &_particleSystems[instance->system];
        if (!instance->valid) continue;
        if (system->spriteCount == 0) continue;

        for (int i = 0; i < instance->particleCount; i++) {
            int index;
            if (system->animated) {
                index = (system->spriteCount * instance->age) / system->duration;
                if (index >= system->spriteCount) index -= 1;
            } else {
                index = i % system->spriteCount;
            }
            SpriteID spr = system->sprites[index];
            SpriteQueue_addSprite(spr,
                instance->xbase + instance->xs[i],
                instance->ybase,
                instance->zbase - instance->ys[i]);
        }
    }
}

static float _randFloatBetween(float min, float max) {
    // based on gamerand
    static uint32_t state0 = 0x2e249079;
    static uint32_t state1 = 0x2e249079 ^ 0x49616e42;
    state0 = ( state0 << 16 ) + ( state0 >> 16 );
    state0 += state1;
    state1 += state0;
    float delta = max - min;
    return min + delta * (state0 / (float) UINT32_MAX);
}
