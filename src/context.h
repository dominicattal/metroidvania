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
#define MAX_NUM_WALLS 100
#define MAX_NUM_ENEMIES 10

// add 1 for the player
#define MAX_NUM_ENTITIES (MAX_NUM_PROJECTILES + MAX_NUM_ENEMIES + MAX_NUM_WALLS + 1)

typedef enum {
    SOUND_TEST,
    NUM_SOUNDS
} Sound;

typedef enum {
    ANIMATION_SOUL,
    NUM_ANIMATIONS,
    ANIMATION_SOLID_COLOR
} AnimationType;

typedef enum {
    FLAG_RECT_HITBOX     = 1 << 0,
    FLAG_EXPIRE          = 1 << 1,
    FLAG_DELETE          = 1 << 2,
    FLAG_COLLIDE_EXPIRE  = 1 << 3,
} EntityFlag;

typedef struct {
    AnimationType type;
    f64 duration;
    i32 frame;
} Animation;

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

    i32 window_width;
    i32 window_height;

    Arena wall_arena;
    Arena projectile_arena;

    Entity player;

    f64 time;

    bool draw_hitboxes;
    bool draw_entities;

} GlobalContext;

extern GlobalContext global_ctx;

bool key_pressed(i32 key);

void animation_init(Animation* animation, AnimationType type);
void animation_update(Animation* animation, f64 dt);

void audio_init(void);
void audio_update(void);
void audio_play(Sound sound);

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
