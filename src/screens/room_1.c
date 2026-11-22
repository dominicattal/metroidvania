#include "../context.h"

#define PROJ_FLAG_GROW PROJ_FLAG_GENERIC_1
#define PROJ_FLAG_START_MOVING PROJ_FLAG_GENERIC_1

typedef enum {
    START,
    WOKE_UP,
    PLAYER_MOVED,
    PLAYER_TRIED_ONCE,
    PLAYER_TRIED_TWICE,
    PLAYER_FREED,
    ATTACK_1,
    ATTACK_2,
    ATTACK_3,
    ATTACK_4
} State;

static struct {
    Wall* inner_walls[4];
    Wall* outer_walls[4];
    AnimatedRect* wasd_rect;
    AnimatedRect* e_rect;
    State state;
    f64 next_time;

    Projectile* crosses[4];
    f64 cross_next_fire_times[4];
    f64 cross_next_switch_times[4];
    i32 cross_directions[4];

    bool show_start_msg;
    bool show_woke_up_msg;
    bool show_player_moved_msg;
    bool show_tried_once_msg;
    bool show_tried_twice_msg;
    bool show_freed_msg;
    bool show_inner_walls;
    bool show_outer_walls;
} ctx;

static vec2 targets[4] = {
    (vec2){RESOLUTION_X / 2 - 100.0f, RESOLUTION_Y / 2 - 100.0f},
    (vec2){RESOLUTION_X / 2 - 100.0f, -RESOLUTION_Y / 2 + 100.0f},
    (vec2){-RESOLUTION_X / 2 + 100.0f, -RESOLUTION_Y / 2 + 100.0f},
    (vec2){-RESOLUTION_X / 2 + 100.0f, RESOLUTION_Y / 2 - 100.0f}
};

static f64 speed = 400.0;

void screen_room_1_load(void)
{
    memset(&ctx, 0, sizeof(ctx));

    ctx.state = START;

    ui_load(UI_NONE);
    player_init(0.0, 0.0);
    ctx.state = ATTACK_4;
    ctx.next_time = global_ctx.time + 30.0;
    //flag_clear(&global_ctx.player.flags, PLAYER_FLAG_CAN_MOVE); 
    //flag_clear(&global_ctx.player.flags, PLAYER_FLAG_CAN_EXPLODE); 
    //global_ctx.player.entity->animation.color = (Color){1.0, 1.0, 1.0, 0.0};

    //ctx.inner_walls[0] = wall_create(30.0f, 30.0f, 8.0f, 60.0f);
    //ctx.inner_walls[0]->color.a = 0.0f;
    //flag_set(&ctx.inner_walls[0]->flags, WALL_FLAG_EXPLOSION_DELETE);

    //ctx.inner_walls[1] = wall_create(-38.0f, 30.0f, 8.0f, 60.0f);
    //ctx.inner_walls[1]->color.a = 0.0f;
    //flag_set(&ctx.inner_walls[1]->flags, WALL_FLAG_EXPLOSION_DELETE);

    //ctx.inner_walls[2] = wall_create(-38.0f, 38.0f, 76.0f, 8.0f);
    //ctx.inner_walls[2]->color.a = 0.0f;
    //flag_set(&ctx.inner_walls[2]->flags, WALL_FLAG_EXPLOSION_DELETE);

    //ctx.inner_walls[3] = wall_create(-38.0f, -30.0f, 76.0f, 8.0f);
    //ctx.inner_walls[3]->color.a = 0.0f;
    //flag_set(&ctx.inner_walls[3]->flags, WALL_FLAG_EXPLOSION_DELETE);

    ctx.outer_walls[0] = wall_create(RESOLUTION_X / 2.0f - 18.0f, RESOLUTION_Y / 2.0f - 18.0f, 8.0f, RESOLUTION_Y - 28.0f);
    ctx.outer_walls[0]->color.a = 1.0f;

    ctx.outer_walls[1] = wall_create(-RESOLUTION_X / 2.0f + 10.0f, RESOLUTION_Y / 2.0f - 18.0f, 8.0f, RESOLUTION_Y - 28.0f);
    ctx.outer_walls[1]->color.a = 1.0f;

    ctx.outer_walls[2] = wall_create(-RESOLUTION_X / 2.0f + 10.0f, RESOLUTION_Y / 2.0f - 10.0f, RESOLUTION_X - 20.0f, 8.0f);
    ctx.outer_walls[2]->color.a = 1.0f;

    ctx.outer_walls[3] = wall_create(-RESOLUTION_X / 2.0f + 10.0f, -RESOLUTION_Y / 2.0f + 10.0f, RESOLUTION_X - 20.0f, 8.0f);
    ctx.outer_walls[3]->color.a = 1.0f;
}

static void cross_1_update(Projectile* proj, f64 dt)
{
    i32* direction = &ctx.cross_directions[0];
    f64* next_fire_time = &ctx.cross_next_fire_times[0];
    f64* switch_time = &ctx.cross_next_switch_times[0];

    if (!flag_test(&proj->flags, PROJ_FLAG_GROW) && proj->size.x < 128.0)
    {
        proj->size.x += 128.0 * dt;
        if (proj->size.x > 128.0)
        {
            proj->size.x = 128.0;
            flag_set(&proj->flags, PROJ_FLAG_GROW);
        }
        proj->size.y = proj->size.x;
    }

    if (*switch_time < global_ctx.time)
    {
        *direction = (*direction + 1) % 4;
        proj->velocity = vec2_scale_to(vec2_sub(targets[*direction], proj->position), speed);
        f64 dis = vec2_distance(proj->position, targets[*direction]);
        *switch_time = global_ctx.time + dis / speed;
    }

    if (*next_fire_time < global_ctx.time)
    {
        Projectile* bullet = projectile_create(proj->position.x, proj->position.y, 20.0f, 20.0f);
        bullet->velocity = vec2_scale_to(vec2_sub(global_ctx.player.entity->position, bullet->position), 300.0);
        bullet->rotation = vec2_radians(bullet->velocity);
        flag_set(&bullet->flags, PROJ_FLAG_COLLIDE_DELETE);
        bullet->hitbox_radius = 5.0f;
        bullet->expire_time = global_ctx.time + 5.0f;
        *next_fire_time += 0.7;

        Projectile* copy = projectile_dup(bullet);
        copy->velocity = vec2_rotate(copy->velocity, 0.2);
        copy->rotation = vec2_radians(copy->velocity);

        copy = projectile_dup(bullet);
        copy->velocity = vec2_rotate(copy->velocity, -0.2);
        copy->rotation = vec2_radians(copy->velocity);
    }

    proj->rotation += 3.0 * dt;
}

static void cross_2_update(Projectile* proj, f64 dt)
{
    i32* direction = &ctx.cross_directions[2];
    f64* next_fire_time = &ctx.cross_next_fire_times[2];
    f64* switch_time = &ctx.cross_next_switch_times[2];

    if (!flag_test(&proj->flags, PROJ_FLAG_GROW) && proj->size.x < 128.0)
    {
        proj->size.x += 128.0 * dt;
        if (proj->size.x > 128.0)
        {
            proj->size.x = 128.0;
            flag_set(&proj->flags, PROJ_FLAG_GROW);
        }
        proj->size.y = proj->size.x;
    }

    if (*switch_time < global_ctx.time)
    {
        *direction = (*direction + 1) % 4;
        proj->velocity = vec2_scale_to(vec2_sub(targets[*direction], proj->position), speed);
        f64 dis = vec2_distance(proj->position, targets[*direction]);
        *switch_time = global_ctx.time + dis / speed;
    }

    if (*next_fire_time < global_ctx.time)
    {
        vec2 dir = vec2_sub(VEC2_ZERO, proj->position);
        Projectile* bullet = projectile_create(proj->position.x, proj->position.y, 20.0f, 20.0f);
        bullet->velocity = vec2_scale_to(vec2_rotate(dir, randf_range(-0.7, 0.7)), randf_range(100.0f, 200.0f));
        bullet->rotation = vec2_radians(bullet->velocity);
        flag_set(&bullet->flags, PROJ_FLAG_COLLIDE_DELETE);
        bullet->hitbox_radius = 5.0f;
        bullet->expire_time = global_ctx.time + 20.0f;
        *next_fire_time += 0.15;
    }

    proj->rotation += 3.0 * dt;
}

typedef struct {
    f64 move_time;
} Cross3BulletData;

static void cross_3_bullet_update(Projectile* proj, f64 dt)
{
    Cross3BulletData* data = projectile_data(proj);
    proj->rotation += 3.0 * dt;
    if (!flag_test(&proj->flags, PROJ_FLAG_START_MOVING) && data->move_time < global_ctx.time)
    {
        proj->velocity = vec2_scale_to(vec2_sub(global_ctx.player.entity->position, proj->position), 1.0);
        flag_set(&proj->flags, PROJ_FLAG_START_MOVING);
    }
    if (flag_test(&proj->flags, PROJ_FLAG_START_MOVING))
    {
        proj->velocity = vec2_add(proj->velocity, vec2_scale_to(proj->velocity, 500.0 * dt));
    }
}

static void cross_3_update(Projectile* proj, f64 dt)
{
    i32* direction = &ctx.cross_directions[1];
    f64* next_fire_time = &ctx.cross_next_fire_times[1];
    f64* switch_time = &ctx.cross_next_switch_times[1];

    if (!flag_test(&proj->flags, PROJ_FLAG_GROW) && proj->size.x < 128.0)
    {
        proj->size.x += 128.0 * dt;
        if (proj->size.x > 128.0)
        {
            proj->size.x = 128.0;
            flag_set(&proj->flags, PROJ_FLAG_GROW);
        }
        proj->size.y = proj->size.x;
    }

    if (*switch_time < global_ctx.time)
    {
        *direction = (*direction + 1) % 4;
        proj->velocity = vec2_scale_to(vec2_sub(targets[*direction], proj->position), speed);
        f64 dis = vec2_distance(proj->position, targets[*direction]);
        *switch_time = global_ctx.time + dis / speed;
    }

    if (*next_fire_time < global_ctx.time)
    {
        Projectile* bullet = projectile_create(proj->position.x, proj->position.y, 10.0f, 100.0f);
        Cross3BulletData* data = projectile_data(bullet);
        data->move_time = global_ctx.time + 5.0;
        bullet->rotation = vec2_radians(bullet->velocity);
        bullet->update_callback = cross_3_bullet_update;
        flag_set(&bullet->flags, PROJ_FLAG_RECT_HITBOX);
        flag_set(&bullet->flags, PROJ_FLAG_COLLIDE_DELETE);
        bullet->hitbox_size = vec2_create(7.0f, 80.0f);
        bullet->expire_time = global_ctx.time + 10.0;
        *next_fire_time += 0.7;
    }

    proj->rotation += 3.0 * dt;
}

static void cross_4_update(Projectile* proj, f64 dt)
{
    i32* direction = &ctx.cross_directions[3];
    f64* next_fire_time = &ctx.cross_next_fire_times[3];
    f64* switch_time = &ctx.cross_next_switch_times[3];

    if (!flag_test(&proj->flags, PROJ_FLAG_GROW) && proj->size.x < 128.0)
    {
        proj->size.x += 128.0 * dt;
        if (proj->size.x > 128.0)
        {
            proj->size.x = 128.0;
            flag_set(&proj->flags, PROJ_FLAG_GROW);
        }
        proj->size.y = proj->size.x;
    }

    if (*switch_time < global_ctx.time)
    {
        *direction = (*direction + 1) % 4;
        proj->velocity = vec2_scale_to(vec2_sub(targets[*direction], proj->position), speed);
        f64 dis = vec2_distance(proj->position, targets[*direction]);
        *switch_time = global_ctx.time + dis / speed;
    }

    if (*next_fire_time < global_ctx.time)
    {
        Beam* beam = beam_create(proj->position.x, proj->position.y, 3000.0f, 20.0f,
                vec2_radians(vec2_sub(global_ctx.player.entity->position, proj->position)));
        beam->expire_time = global_ctx.time + 0.5;
        *next_fire_time += 0.7;
    }

    proj->rotation += 3.0 * dt;
}

static void create_crosses()
{
    for (i32 i = 0; i < 4; i++)
    {
        if (ctx.crosses[i] == NULL)
        {
            ctx.cross_directions[i] = i;
            ctx.crosses[i] = projectile_create(targets[ctx.cross_directions[i]].x, targets[ctx.cross_directions[i]].y, 0.0, 0.0);
            ctx.crosses[i]->velocity = vec2_scale_to(vec2_sub(targets[(ctx.cross_directions[i] + 1) % 4], ctx.crosses[i]->position), speed);
            animation_init(&ctx.crosses[i]->animation, ANIMATION_CROSS);
            flag_clear(&ctx.crosses[i]->flags, PROJ_FLAG_COLLIDE_DELETE);
            flag_clear(&ctx.crosses[i]->flags, PROJ_FLAG_EXPLOSION_DELETE);
            f64 dis = vec2_distance(ctx.crosses[i]->position, targets[(ctx.cross_directions[i] + 1) % 4]);
            ctx.cross_next_switch_times[i] = global_ctx.time + dis / speed;
            ctx.cross_next_fire_times[i] += global_ctx.time + 3.0;
            ctx.crosses[i]->hitbox_radius = 56.0f;
        }
    }
}

static void attack_1_update(f64 dt)
{
    create_crosses();
    ctx.crosses[0]->update_callback = cross_1_update;
}

static void attack_2_update(f64 dt)
{
    create_crosses();
    ctx.crosses[0]->update_callback = cross_1_update;
    ctx.crosses[2]->update_callback = cross_2_update;
}

static void attack_3_update(f64 dt)
{
    create_crosses();
    ctx.crosses[0]->update_callback = cross_1_update;
    ctx.crosses[2]->update_callback = cross_2_update;
    ctx.crosses[1]->update_callback = cross_3_update;
}

static void attack_4_update(f64 dt)
{
    create_crosses();
    ctx.crosses[0]->update_callback = cross_1_update;
    ctx.crosses[2]->update_callback = cross_2_update;
    ctx.crosses[1]->update_callback = cross_3_update;
    ctx.crosses[3]->update_callback = cross_4_update;
}

void screen_room_1_update(f64 dt)
{
    switch (ctx.state)
    {
        case START:
        {
            if (!ctx.show_start_msg)
            {
                char* msg = "...";
                TimedText* text = timed_text_create(msg, global_ctx.time + 5.0, global_ctx.time + 8.0, 0.1);
                text->sound = SOUND_BREAK_DOOR;
                text->x = -calculate_text_width(msg, 1.0f, text->font) / 2.0f;
                text->y = 100.0f;
                ctx.show_start_msg = true;
            }
            if (global_ctx.time > 12.0)
            {
                global_ctx.player.entity->animation.color.a += 0.04 * dt;
            }
            if (global_ctx.player.entity->animation.color.a > 0.2)
            {
                global_ctx.player.entity->animation.color.a = 0.2;
                ctx.state = WOKE_UP;
            }
            break;
        }
        case WOKE_UP:
        {
            if (!ctx.show_woke_up_msg)
            {
                char* msg = "...";
                TimedText* text = timed_text_create(msg, global_ctx.time + 5.0, global_ctx.time + 8.0, 0.1);
                text->sound = SOUND_BREAK_DOOR;
                text->x = -calculate_text_width(msg, 1.0f, text->font) / 2.0f;
                text->y = 100.0f;
                ctx.show_woke_up_msg = true;
                ctx.next_time = global_ctx.time + 12.0;
            }
            if (ctx.next_time < global_ctx.time && ctx.wasd_rect == NULL)
            {
                ctx.wasd_rect = animated_rect_create(ANIMATION_WASD, 0, -150, 144, 96);
                ctx.wasd_rect->animation.color.a = 0;
                ctx.wasd_rect->animation.looping = true;
                flag_set(&global_ctx.player.flags, PLAYER_FLAG_CAN_MOVE);
            }
            if (ctx.wasd_rect != NULL)
            {
                ctx.wasd_rect->animation.color.a += 0.5f * dt;
                if (ctx.wasd_rect->animation.color.a > 1.0f && (key_down(KEY_MOVE_UP) || key_down(KEY_MOVE_LEFT) || key_down(KEY_MOVE_DOWN) || key_down(KEY_MOVE_RIGHT)))
                {
                    ctx.wasd_rect->animation.color.a = 1.0f;
                    ctx.state = PLAYER_MOVED;
                }
            }
            break;
        }
        case PLAYER_MOVED:
        {
            if (ctx.wasd_rect != NULL)
            {
                ctx.wasd_rect->animation.color.a -= 0.5f * dt;
                if (ctx.wasd_rect->animation.color.a < 0.0f)
                {
                    flag_set(&ctx.wasd_rect->flags, ANIMATED_RECT_FLAG_DELETE);
                    ctx.wasd_rect = NULL;
                }
            }
            if (ctx.wasd_rect == NULL && !ctx.show_inner_walls && ctx.inner_walls[0]->color.a < 1.0f)
            {
                ctx.inner_walls[0]->color.a += 0.2f * dt;
                if (ctx.inner_walls[0]->color.a > 1.0f)
                {
                    ctx.inner_walls[0]->color.a = 1.0f;
                    ctx.show_inner_walls = true;
                }
                ctx.inner_walls[1]->color.a = ctx.inner_walls[0]->color.a;
                ctx.inner_walls[2]->color.a = ctx.inner_walls[0]->color.a;
                ctx.inner_walls[3]->color.a = ctx.inner_walls[0]->color.a;
            }
            if (ctx.show_inner_walls && !ctx.show_player_moved_msg)
            {
                char* msg = "Haven't you had enough?";
                TimedText* text = timed_text_create(msg, global_ctx.time + 5.0, global_ctx.time + 12.0, 0.1);
                text->font = FONT_OUTLINE;
                text->sound = SOUND_BREAK_DOOR;
                text->x = -calculate_text_width(msg, 1.0f, text->font) / 2.0f;
                text->y = -100.0f;
                ctx.show_player_moved_msg = true;
                ctx.next_time = global_ctx.time + 16.0;
            }
            if (ctx.show_player_moved_msg && ctx.next_time < global_ctx.time && ctx.e_rect == NULL)
            {
                ctx.e_rect = animated_rect_create(ANIMATION_E, 0, -150, 48, 48);
                ctx.e_rect->animation.color.a = 0;
                ctx.e_rect->animation.looping = true;
            }
            if (ctx.e_rect != NULL)
            {
                ctx.e_rect->animation.color.a += 0.5f * dt;
                if (ctx.e_rect->animation.color.a > 1.0f)
                    ctx.e_rect->animation.color.a = 1.0f;
            }
            if (ctx.e_rect != NULL && key_pressed(KEY_EXPLOSION))
            {
                global_ctx.player.entity->animation.color.a += 0.03;
                audio_play(SOUND_BREAK_DOOR);
                if (global_ctx.player.entity->animation.color.a > 0.5f)
                {
                    global_ctx.player.entity->animation.color.a = 0.5f;
                    ctx.state = PLAYER_TRIED_ONCE;
                }
            }
            break;
        }
        case PLAYER_TRIED_ONCE:
        {
            if (ctx.e_rect != NULL)
            {
                ctx.e_rect->animation.color.a -= 0.5f * dt;
                if (ctx.e_rect->animation.color.a < 0.0f)
                {
                    flag_set(&ctx.e_rect->flags, ANIMATED_RECT_FLAG_DELETE);
                    ctx.e_rect = NULL;
                }
            }
            if (!ctx.show_tried_once_msg)
            {
                char* msg = "Why do you keep coming back?";
                TimedText* text = timed_text_create(msg, global_ctx.time + 5.0, global_ctx.time + 12.0, 0.1);
                text->font = FONT_OUTLINE;
                text->sound = SOUND_BREAK_DOOR;
                text->x = -calculate_text_width(msg, 1.0f, text->font) / 2.0f;
                text->y = -100.0f;
                ctx.show_tried_once_msg = true;
                ctx.next_time = global_ctx.time + 16.0;
            }
            if (ctx.show_tried_once_msg && ctx.next_time < global_ctx.time && key_pressed(KEY_EXPLOSION))
            {
                global_ctx.player.entity->animation.color.a += 0.03;
                audio_play(SOUND_BREAK_DOOR);
                if (global_ctx.player.entity->animation.color.a > 0.8f)
                {
                    global_ctx.player.entity->animation.color.a = 0.8f;
                    ctx.state = PLAYER_TRIED_TWICE;
                }
            }
            break;
        }
        case PLAYER_TRIED_TWICE:
        {
            if (!ctx.show_tried_twice_msg)
            {
                char* msg = "How much more must we beat you down?";
                TimedText* text = timed_text_create(msg, global_ctx.time + 5.0, global_ctx.time + 20.0, 0.1);
                text->sound = SOUND_BREAK_DOOR;
                text->font = FONT_OUTLINE;
                text->x = -calculate_text_width(msg, 1.0f, text->font) / 2.0f;
                text->y = -100.0f;

                msg = "...";
                text = timed_text_create(msg, global_ctx.time + 10.0, global_ctx.time + 20.0, 0.1);
                text->sound = SOUND_BREAK_DOOR;
                text->x = -calculate_text_width(msg, 1.0f, text->font) / 2.0f;
                text->y = 100.0f;

                ctx.show_tried_twice_msg = true;
                ctx.next_time = global_ctx.time + 20.0;
            }
            if (ctx.show_tried_twice_msg && ctx.next_time < global_ctx.time && key_pressed(KEY_EXPLOSION))
            {
                global_ctx.player.entity->animation.color.a += 0.03;
                audio_play(SOUND_BREAK_DOOR);
                if (global_ctx.player.entity->animation.color.a > 1.0f)
                {
                    global_ctx.player.entity->animation.color.a = 1.0f;
                    flag_set(&global_ctx.player.flags, PLAYER_FLAG_CAN_EXPLODE);
                    ctx.next_time = global_ctx.time + 5.0;
                    ctx.state = PLAYER_FREED;
                }
            }
            break;
        }
        case PLAYER_FREED:
        {
            if (!ctx.show_freed_msg)
            {
                char* msg = "Give up";
                TimedText* text = timed_text_create(msg, global_ctx.time + 5.0, global_ctx.time + 20.0, 0.1);
                text->sound = SOUND_BREAK_DOOR;
                text->font = FONT_OUTLINE;
                text->x = -800;
                text->y = 400.0f;
                ctx.show_freed_msg = true;

                msg = "There's still hope";
                text = timed_text_create(msg, global_ctx.time + 9.0, global_ctx.time + 20.0, 0.1);
                text->sound = SOUND_BREAK_DOOR;
                text->x = 340;
                text->y = -400.0f;
                ctx.show_freed_msg = true;

                ctx.next_time = global_ctx.time + 20.0;
            }
            if (ctx.show_freed_msg && ctx.next_time < global_ctx.time)
            {
                ctx.outer_walls[0]->color.a += 0.2f * dt;
                if (ctx.outer_walls[0]->color.a > 1.0f)
                {
                    ctx.outer_walls[0]->color.a = 1.0f;
                    ctx.state = ATTACK_1;
                }
                ctx.outer_walls[1]->color.a = ctx.outer_walls[0]->color.a;
                ctx.outer_walls[2]->color.a = ctx.outer_walls[0]->color.a;
                ctx.outer_walls[3]->color.a = ctx.outer_walls[0]->color.a;
            }
            break;
        }
        case ATTACK_1:
        {
            attack_1_update(dt);
            if (ctx.next_time < global_ctx.time)
            {
                ctx.state = ATTACK_2;
                ctx.next_time = global_ctx.time + 30.0;
            }
            break;
        }
        case ATTACK_2:
        {
            attack_2_update(dt);
            if (ctx.next_time < global_ctx.time)
            {
                ctx.state = ATTACK_3;
                ctx.next_time = global_ctx.time + 30.0;
            }
            break;
        }
        case ATTACK_3:
        {
            attack_3_update(dt);
            break;
        }
        case ATTACK_4:
        {
            attack_4_update(dt);
            break;
        }
        default:
            break;
    }
}
