#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdio.h>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "util.h"

#define RESOLUTION_X 1920
#define RESOLUTION_Y 1080
#define MAX_NUM_PROJECTILES 10000
#define MAX_NUM_ENEMIES 10

// add 1 for the player
#define MAX_NUM_ENTITIES (MAX_NUM_PROJECTILES + MAX_NUM_ENEMIES + 1)

typedef enum {
    ANIMATION_SOUL,
    NUM_ANIMATIONS
} AnimationType;

typedef struct {
    AnimationType type;
    f64 duration;
    i32 frame;
} Animation;

typedef enum {
    FLAG_RECT_HITBOX = 1 << 0,
    FLAG_EXPIRE      = 1 << 1,
    FLAG_DELETE      = 1 << 2,
} EntityFlag;

typedef struct {
    Animation animation;
    vec2 position;
    vec2 velocity;
    vec2 size;
    f64 expire_time;
    f64 rotation;
    u32 flags;
    union {
        f64 hitbox_radius;
        vec2 hitbox_size;
    };
} Entity;

typedef struct {
    Entity* entities;
    i32* dense;
    i32* sparse;
    i32 size;
    i32 capacity;
} Pool;

typedef struct {

    i32 window_width;
    i32 window_height;

    Pool projectile_pool;

    Entity player;

    f64 time;

    bool draw_hitboxes;

} GlobalContext;

extern GlobalContext global_ctx;

bool key_pressed(i32 key);

void    pool_init(Pool* pool, i32 capacity);
i32     pool_alloc(Pool* pool);
Entity* pool_ref(Pool* pool, i32 id);
void    pool_free(Pool* pool, i32 id);

void animation_init(Animation* animation, AnimationType type);
void animation_update(Animation* animation, f64 dt);

void render_init(void);
void render(void);
void render_text(char* text, f32 x, f32 y);
void render_text_ex(char* text, f32 x, f32 y, f32 scale, Color color);
void render_entities(void);

bool flag_test(Entity* entity, EntityFlag flag);
void flag_set(Entity* entity, EntityFlag flag);
void flag_clear(Entity* entity, EntityFlag flag);
void flag_toggle(Entity* entity, EntityFlag flag);

void game_init(void);
void game_update(f64 dt);

#endif
