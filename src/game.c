#include "context.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

extern GlobalContext global_ctx;

void game_key_callback(i32 key, i32 action)
{
    Player* player = &global_ctx.player;
    Entity* entity = player->entity;
    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    {
        if (global_ctx.time > player->small_clear_next_time)
        {
            flag_set(&entity->flags, FLAG_SMALL_CLEAR);
            player->small_clear_next_time += 5.0;
        }
    }
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        global_ctx.camera_locked = !global_ctx.camera_locked; 
    }
    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        global_ctx.camera_center = global_ctx.player.entity->position;
    }
}

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

void player_init(f64 x, f64 y)
{
    assert(global_ctx.player.entity == arena_dummy(&global_ctx.entity_arena));
    i32 id = arena_alloc(&global_ctx.entity_arena);
    global_ctx.player.entity = arena_deref(&global_ctx.entity_arena, id);
    global_ctx.player.entity->position = vec2_create(x, y);
    global_ctx.player.entity->prev_position = global_ctx.player.entity->position;
    global_ctx.player.entity->size = vec2_create(48.0f, 48.0f);
    global_ctx.player.entity->rotation = 0.0f;
    global_ctx.player.entity->hitbox_radius = 10.0f;
}

void timed_text_init(TimedText* text, char* start, f64 start_time, f64 end_time, f64 time_between_chars)
{
    text->start = text->end = start;
    text->next_char_time = start_time; 
    text->time_between_chars = time_between_chars; 
    text->end_time = end_time; 
    text->scale = 1.0f;
    text->color = (Color){1.0f, 1.0f, 1.0f, 1.0f};
    text->font = FONT_DEFAULT;
}

void timed_text_update(TimedText* text, f64 dt)
{
    if (*text->end != '\0' && text->next_char_time < global_ctx.time)
    {
        text->next_char_time += text->time_between_chars;
        text->end++;
    }
}

void game_init(void)
{
    arena_init(&global_ctx.entity_arena, sizeof(Entity), MAX_NUM_ENTITIES);
    arena_init(&global_ctx.wall_arena, sizeof(Wall), MAX_NUM_WALLS);
    arena_init(&global_ctx.projectile_arena, sizeof(Projectile), MAX_NUM_PROJECTILES);
    arena_init(&global_ctx.timed_text_arena, sizeof(TimedText), MAX_TIMED_TEXTS);

    global_ctx.player.entity = arena_dummy(&global_ctx.entity_arena);

    //ui_load(UI_MAIN_MENU);
    screen_load(SCREEN_ROOM_1);
}

static bool collide_circle_circle(vec2 pos1, f64 r1, vec2 pos2, f64 r2)
{
    return vec2_distance(pos1, pos2) < (r1 + r2);
}

static bool collide_circle_obb(vec2 pos1, f64 radius1, vec2 pos2, vec2 size2, f64 rotation2)
{
    // axis 1
    f64 tan_theta = tan(rotation2 + PI / 2);
    f64 b = pos2.y - tan_theta * pos2.x;
    f64 axis_1_dis = fabs(tan_theta * pos1.x - pos1.y + b) / sqrt(tan_theta * tan_theta + 1);
    // axis 2
    tan_theta = tan(rotation2);
    b = pos2.y - tan_theta * pos2.x;
    f64 axis_2_dis = fabs(tan_theta * pos1.x - pos1.y + b) / sqrt(tan_theta * tan_theta + 1);
    //log(DEBUG, "%f %f %f %f", axis_1_dis, axis_2_dis);
    return axis_1_dis < size2.x / 2 + radius1
        && axis_2_dis < size2.y / 2 + radius1;
}

static bool collide_entity_projectile(Entity* entity, Projectile* proj)
{
    bool rect_hitbox = flag_test(&proj->flags, FLAG_RECT_HITBOX);
    if (!rect_hitbox)
        return collide_circle_circle(entity->position, entity->hitbox_radius, proj->position, proj->hitbox_radius);
    return collide_circle_obb(entity->position, entity->hitbox_radius, proj->position, proj->hitbox_size, proj->rotation);
}

static bool collide_player_small_clear_projectile(Player* player, Projectile* proj)
{
    bool rect_hitbox = flag_test(&proj->flags, FLAG_RECT_HITBOX);
    if (!rect_hitbox)
        return collide_circle_circle(player->entity->position, player->small_clear_radius, proj->position, proj->hitbox_radius);
    return collide_circle_obb(player->entity->position, player->small_clear_radius, proj->position, proj->hitbox_size, proj->rotation);
}

static bool collide_entity_wall(Entity* entity, Wall* wall)
{
    f32 ex, ey, ew, eh, dx, dy, wx, wy, sx, sy;
    ex = entity->position.x;
    ey = entity->position.y;
    ew = entity->size.x / 2;
    eh = entity->size.y / 2;
    wx = wall->position.x;
    wy = wall->position.y - wall->size.y;
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
        if (collide_player_small_clear_projectile(&global_ctx.player, proj))
        {
            flag_set(&proj->flags, FLAG_DELETE);
            continue;
        }
        if (collide_entity_projectile(player_entity, proj) && flag_test(&proj->flags, FLAG_COLLIDE_DELETE))
        {
            flag_set(&proj->flags, FLAG_DELETE);
        }
    }

    arena = &global_ctx.wall_arena;
    for (i32 i = 0; i < arena->size; i++)
    {
        collide_entity_wall(player_entity, arena_at(arena, i));
    }
}

static void update_player(f64 dt)
{
    f32 speed = 400.0f;
    Player* player = &global_ctx.player;
    Entity* entity = player->entity;

    entity->velocity = VEC2_ZERO;

    if (key_down(GLFW_KEY_SPACE))
        speed = 150.0f;
    if (key_down(GLFW_KEY_D))
        entity->velocity.x += 1.0f;
    if (key_down(GLFW_KEY_A))
        entity->velocity.x -= 1.0f;
    if (key_down(GLFW_KEY_W))
        entity->velocity.y += 1.0f;
    if (key_down(GLFW_KEY_S))
        entity->velocity.y -= 1.0f;

    entity->velocity = vec2_scale_to(entity->velocity, speed);

    entity->prev_position = entity->position;
    animation_update(&entity->animation, dt);
    entity->position = vec2_add(entity->position, vec2_scale(entity->velocity, dt));
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
}

static void update_center(f64 dt)
{
    if (global_ctx.camera_locked)
    {
        global_ctx.camera_center = global_ctx.player.entity->position;
        return;
    }

    vec2 velocity = VEC2_ZERO;
    if (key_down(GLFW_KEY_RIGHT))
        velocity.x += 1.0f;
    if (key_down(GLFW_KEY_LEFT))
        velocity.x -= 1.0f;
    if (key_down(GLFW_KEY_UP))
        velocity.y += 1.0f;
    if (key_down(GLFW_KEY_DOWN))
        velocity.y -= 1.0f;

    velocity = vec2_scale_to(velocity, 300.0f * dt);
    global_ctx.camera_center = vec2_add(global_ctx.camera_center, velocity);
}

static void update_timed_texts(f64 dt)
{
    Arena* arena = &global_ctx.timed_text_arena;

    i32 i = 0;
    while (i < arena->size)
    {
        TimedText* text = arena_at(arena, i);
        if (text->end_time < global_ctx.time)
        {
            arena_free(arena, arena_id(arena, i));
            continue;
        }
        timed_text_update(text, dt);
        i++;
    }
}

static void update_game_objects(f64 dt)
{
    update_player(dt);
    update_projectiles(dt);
    update_walls(dt);
    update_timed_texts(dt);
}

void game_update(f64 dt)
{
    Arena* arena = &global_ctx.projectile_arena;

    screen_update(dt);

    // Update then collide to handle wall collisions before drawing
    update_game_objects(dt);
    collide_game_objects();
    update_center(dt);
}
