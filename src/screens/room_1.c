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
    Wall* wall = wall_create(30.0f, 30.0f, 8.0f, 60.0f);
    wall->color = RED;
    wall = wall_create(-38.0f, 30.0f, 8.0f, 60.0f);
    wall->color = RED;
    wall = wall_create(-38.0f, 38.0f, 76.0f, 8.0f);
    wall->color = RED;
    wall = wall_create(-38.0f, -30.0f, 76.0f, 8.0f);
    wall->color = RED;
}

void screen_room_1_update(f64 dt)
{
    if (!ctx.player_moved && (key_down(GLFW_KEY_A) || key_down(GLFW_KEY_S) || key_down(GLFW_KEY_D) || key_down(GLFW_KEY_W)))
    {
        char* msg = "Squirming won't help you";
        TimedText* text = timed_text_create(msg, global_ctx.time, global_ctx.time + 15.0, 0.1);
        text->x = -calculate_text_width(msg, 1.0f, text->font) / 2.0f;
        text->y = 100.0f;
        text->font = FONT_OUTLINE;
        ctx.player_moved = true;
    }
}
