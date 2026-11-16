#include "context.h"

typedef struct {
    i32 player_id;
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
        return -1;
    }

    i32 dense_idx = pool->size++;
    i32 sparse_idx = pool->dense[dense_idx];
    pool->sparse[sparse_idx] = dense_idx;
    return sparse_idx;
}

Entity* pool_ref(i32 sparse_idx)
{
    return global_ctx.pool.entities + sparse_idx;
}

void pool_free(i32 sparse_idx)
{
    Pool* pool = &global_ctx.pool;

    i32 dense_idx = pool->sparse[sparse_idx];
    i32 dense_idx_swap = pool->size - 1;
    i32 sparse_idx_swap = pool->sparse[dense_idx_swap];

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

    player->velocity = vec2_scale_to(player->velocity, speed * global_ctx.dt);

    Pool* pool = &global_ctx.pool;
    for (i32 i = 0; i < pool->size; i++)
    {
        Entity* entity = pool_ref(i);
        entity->position = vec2_add(entity->position, entity->velocity);
    }
}
