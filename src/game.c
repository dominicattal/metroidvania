#include "context.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    i32 player_id;
    f64 next_create_time;
} GameContext;

extern GlobalContext global_ctx;
GameContext game_ctx;

void pool_init(void)
{
    for (i32 i = 0; i < NUM_OBJECTS; i++)
        global_ctx.pool.dense[i] = i;
}

i32 pool_alloc(void)
{
    Pool* pool = &global_ctx.pool;
    if (pool->size == NUM_OBJECTS)
    {
        log(ERROR, "Pool alloc failed");
        return NUM_OBJECTS;
    }

    i32 dense_idx = pool->size++;
    i32 sparse_idx = pool->dense[dense_idx];
    pool->sparse[sparse_idx] = dense_idx;
    memset(global_ctx.pool.entities + sparse_idx, 0, sizeof(Entity));
    return sparse_idx;
}

Entity* pool_ref(i32 sparse_idx)
{
    return global_ctx.pool.entities + sparse_idx;
}

void pool_free(i32 sparse_idx)
{
    if (sparse_idx == NUM_OBJECTS)
        return;

    Pool* pool = &global_ctx.pool;

    i32 dense_idx = pool->sparse[sparse_idx];
    i32 dense_idx_swap = pool->size - 1;
    i32 sparse_idx_swap = pool->dense[dense_idx_swap];

    pool->sparse[sparse_idx] = dense_idx_swap;
    pool->sparse[sparse_idx_swap] = dense_idx;
    pool->dense[dense_idx] = sparse_idx_swap;
    pool->dense[dense_idx_swap] = sparse_idx;

    --pool->size;
}

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
    pool_init();

    game_ctx.player_id = pool_alloc();
    Entity* entity = pool_ref(game_ctx.player_id);
    entity->position = (vec2){200.0f, 300.0f};
    entity->size = (vec2){50.0f, 50.0f};
    entity->rotation = 0.0f;
    entity->hitbox_radius = 20.0f;
}

void game_update(void)
{
    if (global_ctx.time > game_ctx.next_create_time)
    {
        f32 x = randf_range(0.0f, RESOLUTION_X);
        i32 id = pool_alloc();
        Entity* entity = pool_ref(id);
        entity->position = vec2_create(x, RESOLUTION_Y);
        entity->velocity = vec2_create(0.0f, -200.0f);
        flag_set(entity, FLAG_EXPIRE);
        entity->size = vec2_create(20.0f, 20.0f);
        entity->expire_time = global_ctx.time + 5.0f;
        entity->hitbox_radius = 5.0f;
        game_ctx.next_create_time = global_ctx.time + 0.1f;
    }

    Entity* player = pool_ref(game_ctx.player_id);

    f32 speed = 200.0f;
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

    Pool* pool = &global_ctx.pool;
    i32 i = 0;
    while (i < pool->size)
    {
        Entity* entity = pool->entities + pool->dense[i];
        if (flag_test(entity, FLAG_EXPIRE) && global_ctx.time > entity->expire_time)
        {
            pool_free(pool->dense[i]);
            continue;
        }
        entity->position = vec2_add(entity->position, vec2_scale(entity->velocity, global_ctx.dt));
        i++;
    }
}
