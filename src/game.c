#include "context.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

extern GlobalContext global_ctx;

void game_key_callback(i32 key, i32 action)
{
    Player* player = &global_ctx.player;
    Entity* entity = player->entity;
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_R)
        {
            global_ctx.camera_locked = !global_ctx.camera_locked; 
        }
        if (key == GLFW_KEY_T)
        {
            global_ctx.camera_center = global_ctx.player.entity->position;
        }
    }
}

static void keybinds_init(void)
{
    global_ctx.keybinds[KEY_MOVE_UP] = GLFW_KEY_W;
    global_ctx.keybinds[KEY_MOVE_LEFT] = GLFW_KEY_A;
    global_ctx.keybinds[KEY_MOVE_DOWN] = GLFW_KEY_S;
    global_ctx.keybinds[KEY_MOVE_RIGHT] = GLFW_KEY_D;
    global_ctx.keybinds[KEY_MOVE_SLOW] = GLFW_KEY_SPACE;
    global_ctx.keybinds[KEY_EXPLOSION] = GLFW_KEY_E;
}

bool flag_test(u32* flags, u32 flag)
{
    return !!(*flags & flag);
}

void flag_set(u32* flags, u32 flag)
{
    *flags |= flag;
}

void flag_clear(u32* flags, u32 flag)
{
    *flags &= ~flag;
}

void flag_toggle(u32* flags, u32 flag)
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
    animation_init(&global_ctx.player.entity->animation, ANIMATION_SOUL);
    flag_set(&global_ctx.player.flags, PLAYER_FLAG_CAN_MOVE); 
    flag_set(&global_ctx.player.flags, PLAYER_FLAG_CAN_EXPLODE); 
}

TimedText* timed_text_create(char* start, f64 start_time, f64 end_time, f64 time_between_chars)
{
    TimedText* text = arena_deref(&global_ctx.timed_text_arena, arena_alloc(&global_ctx.timed_text_arena));
    flag_set(&text->flags, TIMED_TEXT_FLAG_FADE_OUT);
    text->start = text->end = start;
    text->next_char_time = start_time; 
    text->time_between_chars = time_between_chars; 
    text->end_time = end_time; 
    text->scale = 1.0f;
    text->color = WHITE;
    text->font = FONT_DEFAULT;
    return text;
}

Wall* wall_create(f64 x, f64 y, f64 w, f64 h)
{
    Wall* wall = arena_deref(&global_ctx.wall_arena, arena_alloc(&global_ctx.wall_arena));
    wall->position = vec2_create(x, y);
    wall->size = vec2_create(w, h);
    wall->color = WHITE;
    return wall;
}

Entity* entity_create(f64 x, f64 y, f64 w, f64 h)
{
    Entity* entity = arena_deref(&global_ctx.entity_arena, arena_alloc(&global_ctx.entity_arena));
    entity->position.x = x;
    entity->position.y = y;
    entity->size.x = w;
    entity->size.y = h;
    return entity;
}

AnimatedRect* animated_rect_create(AnimationType type, f64 x, f64 y, f64 w, f64 h)
{
    AnimatedRect* rect = arena_deref(&global_ctx.animated_rect_arena, arena_alloc(&global_ctx.animated_rect_arena));
    rect->position.x = x;
    rect->position.y = y;
    rect->size.x = w;
    rect->size.y = h;
    animation_init(&rect->animation, type);
    rect->animation.looping = true;
    rect->end_time = INFINITY;
    return rect;
}

Projectile* projectile_create(f64 x, f64 y, f64 w, f64 h)
{
    Projectile* proj = arena_deref(&global_ctx.projectile_arena, arena_alloc(&global_ctx.projectile_arena));
    animation_init(&proj->animation, ANIMATION_SOLID_COLOR);
    proj->position.x = x;
    proj->position.y = y;
    proj->size.x = w;
    proj->size.y = h;
    flag_set(&proj->flags, PROJ_FLAG_EXPLOSION_DELETE);
    flag_set(&proj->flags, PROJ_FLAG_EXPIRE);
    return proj;
}

Beam* beam_create(f64 x, f64 y, f64 w, f64 h, f64 rot)
{
    Beam* beam = arena_deref(&global_ctx.beam_arena, arena_alloc(&global_ctx.beam_arena));
    animation_init(&beam->animation, ANIMATION_SOLID_COLOR);
    beam->position.x = x;
    beam->position.y = y;
    beam->size.x = w;
    beam->size.y = h;
    beam->hitbox_size = beam->size;
    beam->rotation = rot;
    return beam;
}

void* projectile_data(Projectile* proj)
{
    Arena* arena = &global_ctx.projectile_arena;
    return (u8*)global_ctx.projectile_data + 
        ((u8*)proj - (u8*)arena->instances) / arena->instance_size * MAX_PROJECTILE_DATA_SIZE;
}

void* beam_data(Beam* beam)
{
    Arena* arena = &global_ctx.beam_arena;
    return (u8*)global_ctx.beam_data + 
        ((u8*)beam - (u8*)arena->instances) / arena->instance_size * MAX_BEAM_DATA_SIZE;
}

Projectile* projectile_dup(Projectile* proj)
{
    Projectile* copy = arena_deref(&global_ctx.projectile_arena, arena_alloc(&global_ctx.projectile_arena));
    memcpy(copy, proj, sizeof(Projectile));
    return copy;
}

Beam* beam_dup(Beam* beam)
{
    Beam* copy = arena_deref(&global_ctx.beam_arena, arena_alloc(&global_ctx.beam_arena));
    memcpy(copy, beam, sizeof(Beam));
    return copy;
}

void game_init(void)
{
    keybinds_init();

    size_t proj_data_size = (MAX_NUM_PROJECTILES + 1) * MAX_PROJECTILE_DATA_SIZE;
    size_t beam_data_size = (MAX_NUM_BEAMS + 1) * MAX_BEAM_DATA_SIZE;
    u8* data = malloc(proj_data_size + beam_data_size);

    global_ctx.projectile_data = data;
    global_ctx.beam_data = data + proj_data_size;

    arena_init(&global_ctx.animated_rect_arena, sizeof(AnimatedRect), MAX_NUM_ANIMATED_RECTS);
    arena_init(&global_ctx.entity_arena, sizeof(Entity), MAX_NUM_ENTITIES);
    arena_init(&global_ctx.beam_arena, sizeof(Beam), MAX_NUM_BEAMS);
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

static bool collide_entity_beam(Entity* entity, Beam* beam)
{
    return collide_circle_obb(
            entity->position,
            entity->hitbox_radius,
            vec2_scale(vec2_direction(beam->rotation), beam->hitbox_size.x / 2.0),
            beam->hitbox_size,
            beam->rotation);
}

static bool collide_entity_projectile(Entity* entity, Projectile* proj)
{
    if (!flag_test(&proj->flags, PROJ_FLAG_RECT_HITBOX))
        return collide_circle_circle(entity->position, entity->hitbox_radius, proj->position, proj->hitbox_radius);
    return collide_circle_obb(entity->position, entity->hitbox_radius, proj->position, proj->hitbox_size, proj->rotation);
}

static bool collide_player_explosion_projectile(Player* player, Projectile* proj)
{
    if (!flag_test(&proj->flags, PROJ_FLAG_RECT_HITBOX))
        return collide_circle_circle(player->entity->position, player->explosion_radius, proj->position, proj->hitbox_radius);
    return collide_circle_obb(player->entity->position, player->explosion_radius, proj->position, proj->hitbox_size, proj->rotation);
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

static bool collide_player_explosion_wall(Player* player, Wall* wall)
{
    f32 ex, ey, er, wx, wy, sx, sy;
    ex = player->explosion_position.x;
    ey = player->explosion_position.y;
    er = player->explosion_radius;
    wx = wall->position.x;
    wy = wall->position.y - wall->size.y;
    sx = wall->size.x;
    sy = wall->size.y;
    return ex + er > wx && ex - er < wx + sx && ey + er > wy && ey - er < wy + sy;
}

static void collide_game_objects(void)
{
    Arena* arena;
    Entity* player_entity = global_ctx.player.entity;

    arena = &global_ctx.projectile_arena;
    for (i32 i = 0; i < arena->size; i++)
    {
        Projectile* proj = arena_at(arena, i);
        if (flag_test(&proj->flags, PROJ_FLAG_EXPLOSION_DELETE) && collide_player_explosion_projectile(&global_ctx.player, proj))
        {
            flag_set(&proj->flags, PROJ_FLAG_DELETE);
            continue;
        }
        if (flag_test(&proj->flags, PROJ_FLAG_COLLIDE_DELETE) && collide_entity_projectile(player_entity, proj))
        {
            flag_set(&proj->flags, PROJ_FLAG_DELETE);
            continue;
        }
    }

    arena = &global_ctx.beam_arena;
    for (i32 i = 0; i < arena->size; i++)
    {
        Beam* beam = arena_at(arena, i);
        if (flag_test(&beam->flags, BEAM_FLAG_COLLIDE) && global_ctx.time < beam->start_time && collide_entity_beam(player_entity, beam))
        {
        }
    }

    arena = &global_ctx.wall_arena;
    for (i32 i = 0; i < arena->size; i++)
    {
        Wall* wall = arena_at(arena, i);
        collide_entity_wall(player_entity, wall);
        if (flag_test(&wall->flags, WALL_FLAG_EXPLOSION_DELETE) && collide_player_explosion_wall(&global_ctx.player, wall))
        {
            flag_set(&wall->flags, WALL_FLAG_DELETE);
        }
    }
}

static void update_player(f64 dt)
{
    Player* player = &global_ctx.player;
    Entity* entity = player->entity;

    entity->prev_position = entity->position;
    if (flag_test(&player->flags, PLAYER_FLAG_CAN_MOVE))
    {
        f32 speed = 400.0f;

        entity->velocity = VEC2_ZERO;

        if (key_down(KEY_MOVE_SLOW))
            speed = 150.0f;
        if (key_down(KEY_MOVE_RIGHT))
            entity->velocity.x += 1.0f;
        if (key_down(KEY_MOVE_LEFT))
            entity->velocity.x -= 1.0f;
        if (key_down(KEY_MOVE_UP))
            entity->velocity.y += 1.0f;
        if (key_down(KEY_MOVE_DOWN))
            entity->velocity.y -= 1.0f;

        entity->velocity = vec2_scale_to(entity->velocity, speed);
        entity->position = vec2_add(entity->position, vec2_scale(entity->velocity, dt));
    }

    if (key_pressed(KEY_EXPLOSION) && flag_test(&player->flags, PLAYER_FLAG_CAN_EXPLODE) && global_ctx.time > player->explosion_next_time)
    {
        AnimatedRect* anim = animated_rect_create(ANIMATION_EXPLOSION, player->entity->position.x, player->entity->position.y, 100.0f, 100.0f);
        anim->animation.looping = false;
        flag_set(&player->flags, PLAYER_FLAG_EXPLODED);
        player->explosion_next_time += 1.0;
        player->explosion_position = player->entity->position;
        player->explosion_radius = 100.0f;
    }

    animation_update(&entity->animation, dt);
}

static void update_projectiles(f64 dt)
{
    i32 i = 0;
    Arena* arena = &global_ctx.projectile_arena;
    while (i < arena->size)
    {
        Projectile* proj = arena_at(arena, i);
        bool delete = flag_test(&proj->flags, PROJ_FLAG_DELETE)
                   || (flag_test(&proj->flags, PROJ_FLAG_EXPIRE) && global_ctx.time > proj->expire_time);
        if (delete)
        {
            arena_free(arena, arena_id(arena, i));
            continue;
        }
        if (proj->update_callback != NULL)
            proj->update_callback(proj, dt);
        animation_update(&proj->animation, dt);
        proj->position = vec2_add(proj->position, vec2_scale(proj->velocity, dt));
        i++;
    }
}

static void update_beams(f64 dt)
{
    i32 i = 0;
    Arena* arena = &global_ctx.beam_arena;
    while (i < arena->size)
    {
        Beam* beam = arena_at(arena, i);
        bool delete = flag_test(&beam->flags, BEAM_FLAG_DELETE)
                   || global_ctx.time > beam->expire_time;
        if (delete)
        {
            arena_free(arena, arena_id(arena, i));
            continue;
        }
        if (beam->update_callback != NULL)
            beam->update_callback(beam, dt);
        animation_update(&beam->animation, dt);
        i++;
    }
}

static void update_walls(f64 dt)
{
    i32 i = 0;
    Arena* arena = &global_ctx.wall_arena;
    while (i < arena->size)
    {
        Wall* wall = arena_at(arena, i);
        if (flag_test(&wall->flags, WALL_FLAG_DELETE))
        {
            audio_play(SOUND_BREAK_DOOR);
            arena_free(arena, arena_id(arena, i));
            continue;
        }
        i++;
    }
}

static void update_center(f64 dt)
{
    if (global_ctx.camera_locked)
    {
        global_ctx.camera_center = global_ctx.player.entity->position;
        return;
    }

    vec2 velocity = VEC2_ZERO;
    if (glfw_key_down(GLFW_KEY_RIGHT))
        velocity.x += 1.0f;
    if (glfw_key_down(GLFW_KEY_LEFT))
        velocity.x -= 1.0f;
    if (glfw_key_down(GLFW_KEY_UP))
        velocity.y += 1.0f;
    if (glfw_key_down(GLFW_KEY_DOWN))
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
            if (text->end_callback != NULL)
                text->end_callback(text);
            arena_free(arena, arena_id(arena, i));
            continue;
        }
        if (flag_test(&text->flags, TIMED_TEXT_FLAG_FADE_OUT))
        {
            text->color.a = (text->end_time - global_ctx.time > 1.0f) ? 1.0f : text->end_time - global_ctx.time;
        }
        if (*text->end != '\0' && text->next_char_time < global_ctx.time)
        {
            audio_play(text->sound);
            text->next_char_time += text->time_between_chars;
            text->end++;
        }
        i++;
    }
}

static void update_animated_rects(f64 dt)
{
    Arena* arena = &global_ctx.animated_rect_arena;

    i32 i = 0;
    while (i < arena->size)
    {
        AnimatedRect* rect = arena_at(arena, i);
        if (rect->animation.finished || rect->end_time < global_ctx.time || flag_test(&rect->flags, ANIMATED_RECT_FLAG_DELETE))
        {
            arena_free(arena, arena_id(arena, i));
            continue;
        }
        animation_update(&rect->animation, dt);
        i++;
    }
}

static void update_game_objects(f64 dt)
{
    update_player(dt);
    update_projectiles(dt);
    update_beams(dt);
    update_walls(dt);
    update_timed_texts(dt);
    update_animated_rects(dt);
}

void game_update(f64 dt)
{
    Arena* arena = &global_ctx.projectile_arena;

    screen_update(dt);

    // Update then collide to handle wall collisions before drawing
    update_game_objects(dt);
    collide_game_objects();
    update_center(dt);

    global_ctx.player.explosion_radius = 0.0f;
}
