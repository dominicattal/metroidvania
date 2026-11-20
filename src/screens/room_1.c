#include "../context.h"

typedef struct {
    TimedText text;
    bool player_moved;
} Context;

static Context ctx;

void screen_room_1_load(void)
{
    ui_load(UI_NONE);
    player_init(0.0, 0.0);
    Wall* wall = arena_deref(&global_ctx.wall_arena, arena_alloc(&global_ctx.wall_arena));
    wall->position = vec2_create(30.0f, 30.0f);
    wall->size = vec2_create(8.0f, 60.0f);
    wall = arena_deref(&global_ctx.wall_arena, arena_alloc(&global_ctx.wall_arena));
    wall->position = vec2_create(-38.0f, 30.0f);
    wall->size = vec2_create(8.0f, 60.0f);
    wall = arena_deref(&global_ctx.wall_arena, arena_alloc(&global_ctx.wall_arena));
    wall->position = vec2_create(-38.0f, 38.0f);
    wall->size = vec2_create(76.0f, 8.0f);
    wall = arena_deref(&global_ctx.wall_arena, arena_alloc(&global_ctx.wall_arena));
    wall->position = vec2_create(-38.0f, -30.0f);
    wall->size = vec2_create(76.0f, 8.0f);

    TimedText* test = arena_deref(&global_ctx.timed_text_arena, arena_alloc(&global_ctx.timed_text_arena));
    char* msg = "ABCDEFGHIJKLMNOP";
    timed_text_init(test, msg, global_ctx.time, global_ctx.time + 100.0, 0.1);
    test->x = 50.0f;
    test->y = 50.0f;
    test->font = FONT_OUTLINE;
}

void screen_room_1_update(f64 dt)
{
    if (!ctx.player_moved && (key_down(GLFW_KEY_A) || key_down(GLFW_KEY_S) || key_down(GLFW_KEY_D) || key_down(GLFW_KEY_W)))
    {
        TimedText* text = arena_deref(&global_ctx.timed_text_arena, arena_alloc(&global_ctx.timed_text_arena));
        char* msg = "Squirming won't help you";
        timed_text_init(text, msg, global_ctx.time + 5.0, global_ctx.time + 15.0, 0.1);
        text->x = -calculate_text_width(msg, 1.0f, text->font) / 2.0f;
        text->y = 100.0f;
        ctx.player_moved = true;
    }
}
