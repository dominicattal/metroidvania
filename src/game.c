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
    arena_init(&global_ctx.wall_arena, sizeof(Entity), MAX_NUM_WALLS);
    arena_init(&global_ctx.projectile_arena, sizeof(Entity), MAX_NUM_PROJECTILES);

    Entity* entity = &global_ctx.player;
    entity->animation.type = ANIMATION_SOLID_COLOR;
    entity->position = vec2_create(200.0f, 300.0f);
    entity->size = vec2_create(48.0f, 48.0f);
    entity->rotation = 0.0f;
    flag_set(entity, FLAG_RECT_HITBOX);
    //entity->hitbox_radius = 10.0f;
    entity->hitbox_size = vec2_create(15.0f, 15.0f);

    i32 id = arena_alloc(&global_ctx.wall_arena);
    Entity* wall = arena_deref(&global_ctx.wall_arena, id);
    flag_set(wall, FLAG_RECT_HITBOX);
    wall->position = vec2_create(400.0f, 400.0f);
    wall->size = vec2_create(10.0f, 100.0f);
    wall->hitbox_size = vec2_create(10.0f, 100.0f);
}

static bool collide(Entity* e1, Entity* e2)
{
    bool e1_rect = flag_test(e1, FLAG_RECT_HITBOX);
    bool e2_rect = flag_test(e2, FLAG_RECT_HITBOX);
    if (!e1_rect && !e2_rect)
        return vec2_distance(e1->position, e2->position) < (e1->hitbox_radius + e2->hitbox_radius);

    if (e1_rect ^ e2_rect)
    {
        vec2 hitbox_size = (e1_rect) ? e1->hitbox_size : e2->hitbox_size;
        f32 hitbox_radius = (e1_rect) ? e2->hitbox_radius : e1->hitbox_radius;
    }
    return false;
}

static void entity_update(Entity* entity, f64 dt)
{
    animation_update(&entity->animation, dt);
    entity->position = vec2_add(entity->position, vec2_scale(entity->velocity, dt));
}

static void update_entities(f64 dt)
{
    Arena* arena;
    Entity* player = &global_ctx.player;

    i32 i = 0;
    arena = &global_ctx.projectile_arena;
    while (i < arena->size)
    {
        Entity* entity = arena_at(arena, i);
        if (flag_test(entity, FLAG_DELETE) || (flag_test(entity, FLAG_EXPIRE) && global_ctx.time > entity->expire_time))
        {
            arena_free(arena, arena_id(arena, i));
            continue;
        }
        entity_update(entity, dt);
        i++;
    }

    entity_update(player, dt);
    if (player->position.x - player->size.x / 2 < 0)
        player->position.x = player->size.x / 2;
    if (player->position.x + player->size.x / 2 > RESOLUTION_X)
        player->position.x = RESOLUTION_X - player->size.x / 2;
    if (player->position.y - player->size.y / 2 < 0)
        player->position.y = player->size.y / 2;
    if (player->position.y + player->size.y / 2 > RESOLUTION_Y)
        player->position.y = RESOLUTION_Y - player->size.y / 2;
}

static void collide_entities(void)
{
    Arena* arena;
    Entity* player = &global_ctx.player;
    arena = &global_ctx.projectile_arena;

    i32 i = 0;
    while (i < arena->size)
    {
        Entity* entity = arena_at(arena, i);
        if (collide(entity, player) && flag_test(entity, FLAG_COLLIDE_EXPIRE))
        {
            flag_set(entity, FLAG_DELETE);
        }
        i++;
    }
}

void game_update(f64 dt)
{
    Arena* arena = &global_ctx.projectile_arena;

    //if (global_ctx.time > game_ctx.next_create_time)
    //{
    //    for (i32 i = 0; i < 10; i++)
    //    {
    //        f32 x = randf_range(0.0f, RESOLUTION_X);
    //        i32 id = arena_alloc(arena);
    //        Entity* entity = arena_deref(arena, id);
    //        entity->position = vec2_create(x, RESOLUTION_Y);
    //        entity->velocity = vec2_create(0.0f, -200.0f);
    //        flag_set(entity, FLAG_EXPIRE);
    //        entity->size = vec2_create(20.0f, 20.0f);
    //        entity->expire_time = global_ctx.time + 5.0f;
    //        entity->hitbox_radius = 5.0f;
    //    }
    //    game_ctx.next_create_time = global_ctx.time + 0.1f;
    //}

    Entity* player = &global_ctx.player;

    f32 speed = 400.0f;
    player->velocity = VEC2_ZERO;

    if (key_pressed(GLFW_KEY_SPACE))
        speed = 150.0f;
    if (key_pressed(GLFW_KEY_D))
        player->velocity.x += 1.0f;
    if (key_pressed(GLFW_KEY_A))
        player->velocity.x -= 1.0f;
    if (key_pressed(GLFW_KEY_W))
        player->velocity.y += 1.0f;
    if (key_pressed(GLFW_KEY_S))
        player->velocity.y -= 1.0f;

    player->velocity = vec2_scale_to(player->velocity, speed);
    collide_entities();
    update_entities(dt);
}
