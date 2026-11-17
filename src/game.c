#include "context.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    f64 next_create_time;
} GameContext;

extern GlobalContext global_ctx;
static GameContext game_ctx;

bool flag_test(Entity* entity, EntityFlag flag)
{
    return !!(entity->flags & flag);
}

void flag_set(Entity* entity, EntityFlag flag)
{
    entity->flags |= flag;
}

void flag_clear(Entity* entity, EntityFlag flag)
{
    entity->flags &= ~flag;
}

void flag_toggle(Entity* entity, EntityFlag flag)
{
    entity->flags ^= flag;
}

void game_init(void)
{
    arena_init(&global_ctx.projectile_arena, sizeof(Entity), MAX_NUM_PROJECTILES);

    Entity* entity = &global_ctx.player;
    entity->position = (vec2){200.0f, 300.0f};
    entity->size = (vec2){48.0f, 48.0f};
    entity->rotation = 0.0f;
    entity->hitbox_radius = 10.0f;
}

bool collide(Entity* e1, Entity* e2)
{
    return vec2_distance(e1->position, e2->position) < (e1->hitbox_radius + e2->hitbox_radius);
}

static void entity_update(Entity* entity, f64 dt)
{
    animation_update(&entity->animation, dt);
    entity->position = vec2_add(entity->position, vec2_scale(entity->velocity, dt));
}

void game_update(f64 dt)
{
    Arena* arena = &global_ctx.projectile_arena;

    if (global_ctx.time > game_ctx.next_create_time)
    {
        for (i32 i = 0; i < 10; i++)
        {
            f32 x = randf_range(0.0f, RESOLUTION_X);
            i32 id = arena_alloc(arena);
            Entity* entity = arena_deref(arena, id);
            entity->position = vec2_create(x, RESOLUTION_Y);
            entity->velocity = vec2_create(0.0f, -200.0f);
            flag_set(entity, FLAG_EXPIRE);
            entity->size = vec2_create(20.0f, 20.0f);
            entity->expire_time = global_ctx.time + 5.0f;
            entity->hitbox_radius = 5.0f;
        }
        game_ctx.next_create_time = global_ctx.time + 0.1f;
    }

    Entity* player = &global_ctx.player;

    f32 speed = 400.0f;
    player->velocity = VEC2_ZERO;

    if (key_pressed(GLFW_KEY_SPACE))
        speed = 60.0f;
    if (key_pressed(GLFW_KEY_D))
        player->velocity.x += 1.0f;
    if (key_pressed(GLFW_KEY_A))
        player->velocity.x -= 1.0f;
    if (key_pressed(GLFW_KEY_W))
        player->velocity.y += 1.0f;
    if (key_pressed(GLFW_KEY_S))
        player->velocity.y -= 1.0f;

    player->velocity = vec2_scale_to(player->velocity, speed);

    i32 i = 0;
    while (i < arena->size)
    {
        Entity* entity = arena_at(arena, i);
        if (collide(entity, player))
        {
            flag_set(entity, FLAG_DELETE);
        }
        if (flag_test(entity, FLAG_DELETE) || (flag_test(entity, FLAG_EXPIRE) && global_ctx.time > entity->expire_time))
        {
            arena_free(arena, arena_id(arena, i));
            continue;
        }
        entity_update(entity, dt);
        i++;
    }
    entity_update(player, dt);
}
