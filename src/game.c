#include "context.h"
#include <stdlib.h>
#include <string.h>

extern GlobalContext global_ctx;

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
    arena_init(&global_ctx.wall_arena, sizeof(Wall), MAX_NUM_WALLS);
    arena_init(&global_ctx.projectile_arena, sizeof(Entity), MAX_NUM_PROJECTILES);

    Entity* entity = &global_ctx.player;
    entity->animation.type = ANIMATION_SOUL;
    entity->position = vec2_create(200.0f, 300.0f);
    entity->prev_position = entity->position;
    entity->size = vec2_create(48.0f, 48.0f);
    entity->rotation = 0.0f;
    entity->hitbox_radius = 10.0f;

    screen_load(SCREEN_1);
}

static bool collide(Entity* e1, Entity* e2)
{
    bool e1_rect = flag_test(e1, FLAG_RECT_HITBOX);
    bool e2_rect = flag_test(e2, FLAG_RECT_HITBOX);
    if (!e1_rect && !e2_rect)
        return vec2_distance(e1->position, e2->position) < (e1->hitbox_radius + e2->hitbox_radius);

    if (e1_rect ^ e2_rect)
    {
        if (e2_rect)
        {
            Entity* tmp = e1;
            e1 = e2;
            e2 = e1;
        }
        // e1 rect, e2 circle
        // axis 1
        f64 tan_theta = tan(e1->rotation + PI / 2);
        f64 b = e1->position.y - tan_theta * e1->position.x;
        f64 axis_1_dis = fabs(tan_theta * e2->position.x - e2->position.y + b) / sqrt(tan_theta * tan_theta + 1);
        // axis 2
        tan_theta = tan(e1->rotation);
        b = e1->position.y - tan_theta * e1->position.x;
        f64 axis_2_dis = fabs(tan_theta * e2->position.x - e2->position.y + b) / sqrt(tan_theta * tan_theta + 1);
        //log(DEBUG, "%f %f %f %f", axis_1_dis, axis_2_dis);
        return axis_1_dis < e1->hitbox_size.x + e2->hitbox_radius
            && axis_2_dis < e1->hitbox_size.y + e2->hitbox_radius;
    }

    // Dont bother testing OBB x OBB
    return false;
}

static bool collide_projectile_wall(Projectile* proj, Wall* wall)
{
    f32 ex, ey, ew, eh, dx, dy, wx, wy, sx, sy;
    ex = proj->position.x;
    ey = proj->position.y;
    ew = proj->size.x / 2;
    eh = proj->size.y / 2;
    wx = wall->position.x;
    wy = wall->position.y;
    sx = wall->size.x;
    sy = wall->size.y;
    return ex + ew > wx && ex - ew < wx + sx && ey + eh > wy && ey - eh < wy + sy;
}

static bool collide_entity_wall(Entity* entity, Wall* wall)
{
    f32 ex, ey, ew, eh, dx, dy, wx, wy, sx, sy;
    ex = entity->position.x;
    ey = entity->position.y;
    ew = entity->size.x / 2;
    eh = entity->size.y / 2;
    wx = wall->position.x;
    wy = wall->position.y;
    sx = wall->size.x;
    sy = wall->size.y;
    if (!(ex + ew > wx && ex - ew < wx + sx && ey + eh > wy && ey - eh < wy + sy))
        return false;

    ex = entity->prev_position.x;
    ey = entity->prev_position.y;
    dx = entity->position.x - entity->prev_position.x;
    dy = entity->position.y - entity->prev_position.y;
    if (ex < wx && dx > 0 && ey - eh < wy + sy && ey + eh > wy) {
        entity->position.x = wx - ew;
        entity->velocity.x = 0;
    } else if (ex > wx + sx && dx < 0 && ey - eh < wy + sy && ey + eh > wy) {
        entity->position.x = wx + sx + ew;
        entity->velocity.x = 0;
    } else if (ey < wy && dy > 0 && ex - ew < wx + sx && ex + ew > wx) {
        entity->position.y = wy - eh;
        entity->velocity.y = 0;
    } else if (ey > wy + sy && dy < 0 && ex - ew < wx + sx && ex + ew > wx) {
        entity->position.y = wy + sy + eh;
        entity->velocity.y = 0;
    }
    return true;
}

static void entity_update(Entity* entity, f64 dt)
{
    animation_update(&entity->animation, dt);
    entity->position = vec2_add(entity->position, vec2_scale(entity->velocity, dt));
}

static void collide_entities(void)
{
    Arena* arena;
    Entity* player = &global_ctx.player;

    arena = &global_ctx.projectile_arena;
    for (i32 i = 0; i < arena->size; i++)
    {
        Entity* proj = arena_at(arena, i);
        if (collide(proj, player) && flag_test(proj, FLAG_COLLIDE_DELETE))
            flag_set(proj, FLAG_DELETE);

        for (i32 i = 0; i < global_ctx.wall_arena.size; i++)
        {
            Wall* wall = arena_at(&global_ctx.wall_arena, i);
            if (collide_projectile_wall(proj, wall) && flag_test(proj, FLAG_COLLIDE_DELETE))
                flag_set(proj, FLAG_DELETE);
        }
    }

    arena = &global_ctx.wall_arena;
    for (i32 i = 0; i < arena->size; i++)
    {
        Wall* wall = arena_at(arena, i);
        collide_entity_wall(player, wall);
        wall->position.x += 10.0f / 60.0f;
    }
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

    player->prev_position = player->position;
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

void game_update(f64 dt)
{
    Arena* arena = &global_ctx.projectile_arena;

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

    screen_update(dt);

    // Update then collide to handle wall collisions before drawing
    update_entities(dt);
    collide_entities();
}
