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
#define MAX_NUM_ENTITIES 100

// add 1 for the player
#define MAX_NUM_RECTS (MAX_NUM_PROJECTILES + MAX_NUM_ENTITIES + MAX_NUM_WALLS + 1)

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
    FLAG_RECT_HITBOX     = 1 << 0, // Whether entity uses rectanglular hitbox or radius hitbox
    FLAG_EXPIRE          = 1 << 1, // Whether the entity expires using expire_time
    FLAG_DELETE          = 1 << 2, // Whether the entity should be deleted next frame
    FLAG_COLLIDE_DELETE  = 1 << 3, // Whether the entity should be deleted after collision
    FLAG_GRAZED          = 1 << 4, // Whether a projectile has been grazed
} Flag;

typedef enum {
    UI_MAIN_MENU,
    UI_GAME,
    NUM_UIS
} UI;

typedef enum {
    SCREEN_NONE,
    SCREEN_1,
    SCREEN_2,
    NUM_SCREENS
} Screen;

typedef struct {
    AnimationType type;
    f64 duration;
    i32 frame;
} Animation;

typedef struct {
    // Walls are drawn with position at the bottom left corner
    vec2 position;
    vec2 size;
} Wall;

typedef struct {
    Animation animation;
    vec2 position;
    vec2 prev_position;
    vec2 velocity;
    vec2 size;
    f64 rotation;
    u32 flags;
    f64 hitbox_radius;
} Entity;

typedef struct {
    Animation animation;
    // Entities are drawn with position at their center
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
} Projectile;

typedef struct {
    Entity* entity;
    i32 lives;
    i32 graze;
    f32 graze_hitbox_radius;
} Player;

typedef struct {

    GLFWwindow* window;
    GLFWmonitor* monitor;
    const GLFWvidmode* mode;

    i32 window_width;
    i32 window_height;
    bool fullscreen;

    Arena wall_arena;
    Arena entity_arena;
    Arena projectile_arena;

    Player player;

    Screen screen;
    UI ui;

    f64 next_create_time;

    f64 time;
    f64 next_update_time;

    bool key_pressed[GLFW_KEY_LAST+1];
    bool draw_hitboxes;
    bool draw_entities;

} GlobalContext;

extern GlobalContext global_ctx;

bool key_down(i32 key);
bool key_pressed(i32 key);

void animation_init(Animation* animation, AnimationType type);
void animation_update(Animation* animation, f64 dt);

void audio_init(void);
void audio_update(void);
void audio_play(Sound sound);

void render_init(void);
void render_cleanup(void);
void render(void);
void render_rect(vec2 pos, vec2 size, Color color);
void render_rect_ex(vec2 pos, vec2 size, Color color, Animation* animation);
f32  calculate_text_height(f32 scale);
f32  calculate_text_width(char* text, f32 scale);
void render_text(char* text, f32 x, f32 y);
void render_text_ex(char* text, f32 x, f32 y, f32 scale, Color color);
void render_entities(void);

bool flag_test(u32* flags, Flag flag);
void flag_set(u32* flags, Flag flag);
void flag_clear(u32* flags, Flag flag);
void flag_toggle(u32* flags, Flag flag);

void game_init(void);
void game_update(f64 dt);

void screen_load(Screen screen);
void screen_update(f64 dt);

void ui_init(void);
void ui_load(UI ui);
void ui_update(f64 dt);
void ui_draw(void);

#endif
