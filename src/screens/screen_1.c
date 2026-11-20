#include "../context.h"

void screen_screen_1_load(void)
{
    ui_load(UI_GAME);
    player_init(0.0, 0.0);
    i32 id = arena_alloc(&global_ctx.wall_arena);
    Wall* wall = arena_deref(&global_ctx.wall_arena, id);
    wall->position = vec2_create(400.0f, 400.0f);
    wall->size = vec2_create(100.0f, 100.0f);
}

void screen_screen_1_update(f64 dt)
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
