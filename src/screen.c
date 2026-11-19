#include "context.h"

void screen_load(Screen screen)
{
    global_ctx.screen = screen;
    global_ctx.time = global_ctx.next_update_time = 0;
    global_ctx.next_create_time = 0;
    arena_clear(&global_ctx.projectile_arena);
    arena_clear(&global_ctx.wall_arena);

    switch (screen)
    {
        case SCREEN_1:
        {
            i32 id = arena_alloc(&global_ctx.wall_arena);
            Wall* wall = arena_deref(&global_ctx.wall_arena, id);
            wall->position = vec2_create(400.0f, 400.0f);
            wall->size = vec2_create(100.0f, 100.0f);
            break;
        }
        case SCREEN_2:
        {
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
                for (i32 i = 0; i < 10; i++)
                {
                    Arena* arena = &global_ctx.projectile_arena;
                    f32 x = randf_range(0.0f, RESOLUTION_X);
                    i32 id = arena_alloc(arena);
                    Entity* entity = arena_deref(arena, id);
                    entity->position = vec2_create(x, RESOLUTION_Y);
                    entity->velocity = vec2_create(0.0f, -200.0f);
                    flag_set(entity, FLAG_EXPIRE);
                    flag_set(entity, FLAG_COLLIDE_DELETE);
                    entity->size = vec2_create(20.0f, 20.0f);
                    entity->expire_time = global_ctx.time + 5.0f;
                    entity->hitbox_radius = 5.0f;
                }
                global_ctx.next_create_time = global_ctx.time + 0.1f;
            }
        }
        default:
            break;
    }
}
