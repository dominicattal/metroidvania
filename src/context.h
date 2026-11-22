#ifndef CONTEXT_H
#define CONTEXT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "util.h"
#include "screen.h"

#define SCREEN_DATA_SIZE 512
#define RESOLUTION_X 1920
#define RESOLUTION_Y 1080
#define MAX_NUM_PROJECTILES 10000
#define MAX_NUM_WALLS 100
#define MAX_NUM_BEAMS 100
#define MAX_NUM_ENTITIES 100
#define MAX_NUM_ANIMATED_RECTS 100
#define MAX_TIMED_TEXTS 20

#define MAX_PROJECTILE_DATA_SIZE 128

#define BLANK (Color){0.0f, 0.0f, 0.0f, 0.0f}
#define WHITE (Color){1.0f, 1.0f, 1.0f, 1.0f}
#define RED (Color){1.0f, 0.0f, 0.0f, 1.0f}
#define BLUE (Color){0.0f, 0.0f, 1.0f, 1.0f}

// add 1 for the player
#define MAX_NUM_RECTS (MAX_NUM_PROJECTILES + MAX_NUM_BEAMS + MAX_NUM_ANIMATED_RECTS + MAX_NUM_ENTITIES + MAX_NUM_WALLS + 1)

typedef struct TimedText TimedText;
typedef struct Projectile Projectile;
typedef struct Beam Beam;

typedef void (*ScreenUpdateCallback)(f64);
typedef void (*TimedTextEndCallback)(TimedText*);
typedef void (*ProjectileUpdateCallback)(Projectile*, f64);
typedef void (*BeamUpdateCallback)(Beam*, f64);

typedef enum {
    SOUND_TEST,
    SOUND_BREAK_DOOR,
    NUM_SOUNDS
} Sound;

typedef enum {
    FONT_DEFAULT,
    FONT_OUTLINE,
    NUM_FONTS
} Font;

typedef enum {
    KEY_MOVE_UP,
    KEY_MOVE_LEFT,
    KEY_MOVE_DOWN,
    KEY_MOVE_RIGHT,
    KEY_MOVE_SLOW,
    KEY_EXPLOSION,
    NUM_KEYBINDS
} Key;

typedef enum {
    ANIMATION_SOUL,
    ANIMATION_EXPLOSION,
    ANIMATION_WASD,
    ANIMATION_E,
    ANIMATION_CROSS,
    NUM_ANIMATIONS,
    ANIMATION_SOLID_COLOR
} AnimationType;

typedef enum {
    ANIMATED_RECT_FLAG_DELETE   = 1 << 0, // Whether animated rect should be deleted next frame
} AnimatedRectFlag;

typedef enum {
    TIMED_TEXT_FLAG_FADE_OUT    = 1 << 0, // Whether the timed text should fdae out
} TimedTextFlag;

typedef enum {
    PROJ_FLAG_RECT_HITBOX       = 1 << 0, // Whether proj uses rectanglular hitbox or radius hitbox
    PROJ_FLAG_EXPIRE            = 1 << 1, // Whether proj uses expire_time
    PROJ_FLAG_DELETE            = 1 << 2, // Whether proj should be deleted next frame
    PROJ_FLAG_COLLIDE_DELETE    = 1 << 3, // Whether proj should be deleted after collision
    PROJ_FLAG_EXPLOSION_DELETE  = 1 << 4, // Whether proj should be be deleted after player explosion
    PROJ_FLAG_GRAZED            = 1 << 5, // Whether a projectile has been grazed

    // Generic flags that have context-defined meaning
    PROJ_FLAG_GENERIC_1         = 1 << 31
} ProjectileFlag;

typedef enum {
    BEAM_FLAG_DELETE            = 1 << 0, // Whether beam should be deleted next frame
    BEAM_FLAG_COLLIDE_DELETE    = 1 << 1, // Whether beam should be deleted after collision
    BEAM_FLAG_COLLIDE           = 1 << 2, // Whether beam should check collision with player
} BeamFlag;

typedef enum {
    PLAYER_FLAG_CAN_MOVE        = 1 << 0, // Whether player can move
    PLAYER_FLAG_CAN_EXPLODE     = 1 << 1, // Whether player can use explosion
    PLAYER_FLAG_EXPLODED        = 1 << 2, // Whether player used explosion this frame
} PlayerFlag;

typedef enum {
    WALL_FLAG_DELETE            = 1 << 0, // Whether wall should be deleted next frame
    WALL_FLAG_EXPLOSION_DELETE  = 1 << 1, // Whether wall should be be deleted after player explosion
} WallFlag;

typedef enum {
    FLAG_TALKED_TO_CORE, // Whether player has talked to core
    NUM_GAME_FLAGS
} GameFlag;

typedef enum {
    UI_NONE,
    UI_MAIN_MENU,
    UI_GAME,
    NUM_UIS
} UI;

typedef enum {
    SCREEN_NONE,
    SCREEN_CORE,
    SCREEN_ROOM_1,
    SCREEN_1,
    SCREEN_2,
    NUM_SCREENS
} Screen;

typedef struct {
    f32 r, g, b, a;
} Color;

typedef struct {
    AnimationType type;
    Color color;
    f64 duration;
    i32 frame;
    bool looping;
    bool finished;
} Animation;

typedef struct {
    Animation animation;
    f64 end_time;
    vec2 position;
    vec2 size;
    u32 flags;
} AnimatedRect;

typedef struct TimedText {
    TimedTextEndCallback end_callback;
    char* start;
    char* end;
    f64 time_between_chars;
    f64 next_char_time;
    f64 end_time;
    Sound sound;
    Color color;
    Font font;
    u32 flags;
    f32 x, y, scale;
} TimedText;

typedef struct {
    // Walls are drawn with position at the bottom left corner
    vec2 position;
    vec2 size;
    Color color;
    u32 flags;
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

typedef struct Beam {
    Animation animation;
    BeamUpdateCallback update_callback;
    vec2 position;
    vec2 size;
    vec2 hitbox_size;
    f64 rotation;
    f64 expire_time;
    u32 flags;
} Beam;

typedef struct Projectile {
    Animation animation;
    // Entities are drawn with position at their center
    ProjectileUpdateCallback update_callback;
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
    f64 graze_hitbox_radius;
    vec2 explosion_position;
    f64 explosion_radius;
    f64 explosion_next_time;
    u32 flags;
} Player;

typedef struct {

    GLFWwindow* window;
    GLFWmonitor* monitor;
    const GLFWvidmode* mode;

    i32 window_width;
    i32 window_height;
    bool fullscreen;

    Arena animated_rect_arena;
    Arena wall_arena;
    Arena beam_arena;
    Arena entity_arena;
    Arena projectile_arena;
    Arena timed_text_arena;

    void* projectile_data;

    Player player;

    Screen screen;
    ScreenUpdateCallback screen_update_callback;
    UI ui;

    f64 next_create_time;

    f64 time;
    f64 next_update_time;

    bool game_flags[NUM_GAME_FLAGS];
    i32 keybinds[NUM_KEYBINDS];
    bool key_pressed[GLFW_KEY_LAST+1];

    bool draw_hitboxes;

    vec2 camera_center;
    bool camera_locked;

} GlobalContext;

extern GlobalContext global_ctx;

bool glfw_key_down(i32 key);
// cannot use in game logic bc key_pressed updates every window refresh
// but game may not update every window refresh (unless vsynx is on)
bool glfw_key_pressed(i32 key);
bool key_down(Key key);
bool key_pressed(Key key);

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
f32  calculate_text_width(char* text, f32 scale, Font font);
void render_text(char* text, f32 x, f32 y);
void render_text_ex(char* start, char* end, f32 x, f32 y, f32 scale, Color color, Font font);
void ui_render_text(char* text, f32 x, f32 y);
void ui_render_text_ex(char* start, char* end, f32 x, f32 y, f32 scale, Color color);
void render_entities(void);

TimedText* timed_text_create(char* start, f64 start_time, f64 end_time, f64 time_between_chars);

void toggle_camera_lock(void);

Entity* entity_create(f64 x, f64 y, f64 w, f64 h);
AnimatedRect* animated_rect_create(AnimationType type, f64 x, f64 y, f64 w, f64 h);
Projectile* projectile_create(f64 x, f64 y, f64 w, f64 h);
Beam* beam_create(f64 x, f64 y, f64 w, f64 h, f64 rot);
void* projectile_data(Projectile* proj);
Projectile* projectile_dup(Projectile* proj);
Wall* wall_create(f64 x, f64 y, f64 w, f64 h);

bool flag_test(u32* flags, u32 flag);
void flag_set(u32* flags, u32 flag);
void flag_clear(u32* flags, u32 flag);
void flag_toggle(u32* flags, u32 flag);

void game_init(void);
void game_update(f64 dt);
void game_key_callback(i32 key, i32 action);

void player_init(f64 x, f64 y);

void screen_load(Screen screen);
void screen_update(f64 dt);

void ui_init(void);
void ui_load(UI ui);
void ui_update(f64 dt);
void ui_draw(void);

#endif
