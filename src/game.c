#include "context.h"
#include <stdlib.h>
#include <string.h>

extern GlobalContext global_ctx;

bool flag_test(u32* flags, Flag flag)
{
    return !!(*flags & flag);
}

void flag_set(u32* flags, Flag flag)
{
    *flags |= flag;
}

void flag_clear(u32* flags, Flag flag)
{
    *flags &= ~flag;
}

void flag_toggle(u32* flags, Flag flag)
{
    *flags ^= flag;
}

void game_init(void)
{
    arena_init(&global_ctx.entity_arena, sizeof(Entity), MAX_NUM_ENTITIES);
    arena_init(&global_ctx.wall_arena, sizeof(Wall), MAX_NUM_WALLS);
    arena_init(&global_ctx.projectile_arena, sizeof(Projectile), MAX_NUM_PROJECTILES);

    global_ctx.player.entity = arena_dummy(&global_ctx.entity_arena);

    ui_load(UI_MAIN_MENU);
    screen_load(SCREEN_NONE);
}

static bool collide_entity_projectile(Entity* entity, Projectile* proj)
{
    bool rect_hitbox = flag_test(&proj->flags, FLAG_RECT_HITBOX);
    if (!rect_hitbox)
        return vec2_distance(entity->position, proj->position) < (entity->hitbox_radius + proj->hitbox_radius);

    // axis 1
    f64 tan_theta = tan(proj->rotation + PI / 2);
    f64 b = proj->position.y - tan_theta * proj->position.x;
    f64 axis_1_dis = fabs(tan_theta * entity->position.x - entity->position.y + b) / sqrt(tan_theta * tan_theta + 1);
    // axis 2
    tan_theta = tan(proj->rotation);
    b = proj->position.y - tan_theta * proj->position.x;
    f64 axis_2_dis = fabs(tan_theta * entity->position.x - entity->position.y + b) / sqrt(tan_theta * tan_theta + 1);
    //log(DEBUG, "%f %f %f %f", axis_1_dis, axis_2_dis);
    return axis_1_dis < proj->hitbox_size.x / 2 + entity->hitbox_radius
        && axis_2_dis < proj->hitbox_size.y / 2 + entity->hitbox_radius;
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

static void collide_game_objects(void)
{
    Arena* arena;
    Entity* player_entity = global_ctx.player.entity;

    arena = &global_ctx.projectile_arena;
    for (i32 i = 0; i < arena->size; i++)
    {
        Projectile* proj = arena_at(arena, i);
        if (collide_entity_projectile(player_entity, proj) && flag_test(&proj->flags, FLAG_COLLIDE_DELETE))
        {
            flag_set(&proj->flags, FLAG_DELETE);
        }

        for (i32 i = 0; i < global_ctx.wall_arena.size; i++)
        {
            Wall* wall = arena_at(&global_ctx.wall_arena, i);
            if (collide_projectile_wall(proj, wall) && flag_test(&proj->flags, FLAG_COLLIDE_DELETE))
                flag_set(&proj->flags, FLAG_DELETE);
        }
    }
}

static void update_player(f64 dt)
{
    f32 speed = 400.0f;
    Entity* player_entity = global_ctx.player.entity;

    player_entity->velocity = VEC2_ZERO;

    if (key_down(GLFW_KEY_SPACE))
        speed = 150.0f;
    if (key_down(GLFW_KEY_D))
        player_entity->velocity.x += 1.0f;
    if (key_down(GLFW_KEY_A))
        player_entity->velocity.x -= 1.0f;
    if (key_down(GLFW_KEY_W))
        player_entity->velocity.y += 1.0f;
    if (key_down(GLFW_KEY_S))
        player_entity->velocity.y -= 1.0f;

    player_entity->velocity = vec2_scale_to(player_entity->velocity, speed);

    player_entity->prev_position = player_entity->position;
    animation_update(&player_entity->animation, dt);
    player_entity->position = vec2_add(player_entity->position, vec2_scale(player_entity->velocity, dt));
    if (player_entity->position.x - player_entity->size.x / 2 < 0)
        player_entity->position.x = player_entity->size.x / 2;
    if (player_entity->position.x + player_entity->size.x / 2 > RESOLUTION_X)
        player_entity->position.x = RESOLUTION_X - player_entity->size.x / 2;
    if (player_entity->position.y - player_entity->size.y / 2 < 0)
        player_entity->position.y = player_entity->size.y / 2;
    if (player_entity->position.y + player_entity->size.y / 2 > RESOLUTION_Y)
        player_entity->position.y = RESOLUTION_Y - player_entity->size.y / 2;
}

static void update_projectiles(f64 dt)
{
    i32 i = 0;
    Arena* arena = &global_ctx.projectile_arena;
    while (i < arena->size)
    {
        Projectile* proj = arena_at(arena, i);
        if (flag_test(&proj->flags, FLAG_DELETE) || (flag_test(&proj->flags, FLAG_EXPIRE) && global_ctx.time > proj->expire_time))
        {
            arena_free(arena, arena_id(arena, i));
            continue;
        }
        animation_update(&proj->animation, dt);
        proj->position = vec2_add(proj->position, vec2_scale(proj->velocity, dt));
        proj->rotation += dt;
        i++;
    }
}

static void update_walls(f64 dt)
{
    Arena* arena = &global_ctx.wall_arena;
    Entity* player_entity = global_ctx.player.entity;
    for (i32 i = 0; i < arena->size; i++)
    {
        Wall* wall = arena_at(arena, i);
        collide_entity_wall(player_entity, wall);
        //wall->position.x += 10.0f / 60.0f;
    }
}

static void update_game_objects(f64 dt)
{
    update_player(dt);
    update_projectiles(dt);
    update_walls(dt);
}

void game_update(f64 dt)
{
    Arena* arena = &global_ctx.projectile_arena;

    screen_update(dt);

    // Update then collide to handle wall collisions before drawing
    update_game_objects(dt);
    collide_game_objects();
}
