#include "../context.h"

void screen_screen_2_load(void)
{
    ui_load(UI_GAME);
    player_init(0.0, 0.0);
    i32 id = arena_alloc(&global_ctx.wall_arena);
    Wall* wall = arena_deref(&global_ctx.wall_arena, id);
    wall->position = vec2_create(800.0f, 400.0f);
    wall->size = vec2_create(100.0f, 100.0f);
}

void screen_screen_2_update(f64 dt)
{
}
