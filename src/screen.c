#include "context.h"

static void alloc_player(void)
{
    i32 id = arena_alloc(&global_ctx.entity_arena);
    global_ctx.player.entity = arena_deref(&global_ctx.entity_arena, id);
    global_ctx.player.entity->position = vec2_create(200.0f, 300.0f);
    global_ctx.player.entity->prev_position = global_ctx.player.entity->position;
    global_ctx.player.entity->size = vec2_create(48.0f, 48.0f);
    global_ctx.player.entity->rotation = 0.0f;
    global_ctx.player.entity->hitbox_radius = 10.0f;
}

void screen_load(Screen screen)
{
    global_ctx.screen = screen;
    global_ctx.time = global_ctx.next_update_time = 0;
    global_ctx.next_create_time = 0;
    arena_clear(&global_ctx.entity_arena);
    arena_clear(&global_ctx.projectile_arena);
    arena_clear(&global_ctx.wall_arena);

    switch (screen)
    {
        case SCREEN_1:
        {
            ui_load(UI_GAME);
            alloc_player();
            i32 id = arena_alloc(&global_ctx.wall_arena);
            Wall* wall = arena_deref(&global_ctx.wall_arena, id);
            wall->position = vec2_create(400.0f, 400.0f);
            wall->size = vec2_create(100.0f, 100.0f);
            break;
        }
        case SCREEN_2:
        {
            ui_load(UI_GAME);
            alloc_player();
            i32 id = arena_alloc(&global_ctx.wall_arena);
            Wall* wall = arena_deref(&global_ctx.wall_arena, id);
            wall->position = vec2_create(800.0f, 400.0f);
            wall->size = vec2_create(100.0f, 100.0f);
            break;
        }
        default:
            break;
    }
}

void screen_update(f64 dt)
{
    switch (global_ctx.screen)
    {
        case SCREEN_1:
        {
            if (global_ctx.time > global_ctx.next_create_time)
            {
                for (i32 i = 0; i < 5; i++)
                {
                    Arena* arena = &global_ctx.projectile_arena;
                    f32 x = randf_range(0.0f, RESOLUTION_X);
                    i32 id = arena_alloc(arena);
                    Projectile* proj = arena_deref(arena, id);
                    proj->position = vec2_create(x, RESOLUTION_Y);
                    proj->velocity = vec2_create(0.0f, -200.0f);
                    flag_set(&proj->flags, FLAG_EXPIRE);
                    flag_set(&proj->flags, FLAG_COLLIDE_DELETE);
                    proj->size = vec2_create(20.0f, 20.0f);
                    proj->expire_time = global_ctx.time + 5.0f;
                    if (rand() % 2)
                    {
                        proj->hitbox_radius = 5.0f;
                    }
                    else
                    {
                        flag_set(&proj->flags, FLAG_RECT_HITBOX);
                        animation_init(&proj->animation, ANIMATION_SOLID_COLOR);
                        proj->rotation = randf() * PI;
                        proj->size = vec2_create(10.0f, 100.0f);
                        proj->hitbox_size = vec2_create(5.0f, 80.0f);
                    }
                }
                global_ctx.next_create_time = global_ctx.time + 0.1f;
            }
        }
        default:
            break;
    }
}
