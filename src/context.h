#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdio.h>
#include <math.h>
#include <glad/glad.h>
#include <sys/time.h>
#include <GLFW/glfw3.h>
#include "util.h"

#define RESOLUTION_X 1920
#define RESOLUTION_Y 1080
#define NUM_OBJECTS 1000

typedef enum {
    FLAG_RECT_HITBOX = 1 << 0,
} EntityFlag;

typedef struct {
    vec2 position;
    vec2 velocity;
    vec2 size;
    f32 rotation;
    u32 flags;
    union {
        f32 hitbox_radius;
        vec2 hitbox_size;
    };
} Entity;

typedef struct {
    Entity entities[NUM_OBJECTS];
    i32 dense[NUM_OBJECTS];
    i32 sparse[NUM_OBJECTS];
    i32 size;
} Pool;

typedef struct {
    i32 window_width;
    i32 window_height;

    Pool pool;

    f64 time;
    f64 dt;

} GlobalContext;

extern GlobalContext global_ctx;

bool key_pressed(i32 key);

void    pool_init(void);
i32     pool_alloc(void);
Entity* pool_ref(i32 id);
void    pool_free(i32 id);

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
void game_update(void);

#endif
