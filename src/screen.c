#include "context.h"
#include "screen.h"

void screen_load(Screen screen)
{
    arena_clear(&global_ctx.entity_arena);
    arena_clear(&global_ctx.projectile_arena);
    arena_clear(&global_ctx.wall_arena);
    global_ctx.screen_update_callback = NULL;
    global_ctx.screen_render_callback = NULL;
    global_ctx.screen = screen;
    global_ctx.time = global_ctx.next_update_time = 0;
    global_ctx.next_create_time = 0;
    global_ctx.player.entity = arena_dummy(&global_ctx.entity_arena);
    ui_load(UI_NONE);

    switch (screen)
    {
        case SCREEN_CORE:
        {
            break;
        }
        case SCREEN_ROOM_1:
        {
            screen_room_1_load();
            global_ctx.screen_update_callback = screen_room_1_update;
            global_ctx.screen_render_callback = screen_room_1_render;
            break;
        }
        case SCREEN_1:
        {
            screen_screen_1_load();
            global_ctx.screen_update_callback = screen_screen_1_update;
            break;
        }
        case SCREEN_2:
        {
            screen_screen_2_load();
            global_ctx.screen_update_callback = screen_screen_2_update;
            break;
        }
        default:
            break;
    }
}

void screen_update(f64 dt)
{
    if (global_ctx.screen_update_callback != NULL)
        global_ctx.screen_update_callback(dt);
}

void screen_render(void)
{
    if (global_ctx.screen_render_callback != NULL)
        global_ctx.screen_render_callback();
}
