#include "../context.h"

typedef enum {
    START,
    WOKE_UP,
    PLAYER_MOVED,
    PLAYER_TRIED_ONCE,
    PLAYER_TRIED_TWICE,
    PLAYER_FREED
} State;

static struct {
    Wall* walls[4];
    AnimatedRect* wasd_rect;
    AnimatedRect* e_rect;
    State state;
    f64 next_time;
    bool show_start_msg;
    bool show_woke_up_msg;
    bool show_player_moved_msg;
    bool show_tried_once_msg;
    bool show_tried_twice_msg;
    bool show_tried_twice_resp_msg;
    bool show_walls;
} ctx;

void screen_room_1_load(void)
{
    memset(&ctx, 0, sizeof(ctx));

    ctx.state = START;

    ui_load(UI_NONE);
    player_init(0.0, 0.0);
    flag_clear(&global_ctx.player.flags, PLAYER_FLAG_CAN_MOVE); 
    flag_clear(&global_ctx.player.flags, PLAYER_FLAG_CAN_EXPLODE); 
    global_ctx.player.entity->animation.color = (Color){1.0, 1.0, 1.0, 0.0};

    ctx.walls[0] = wall_create(30.0f, 30.0f, 8.0f, 60.0f);
    ctx.walls[0]->color.a = 0.0f;
    flag_set(&ctx.walls[0]->flags, WALL_FLAG_EXPLOSION_DELETE);

    ctx.walls[1] = wall_create(-38.0f, 30.0f, 8.0f, 60.0f);
    ctx.walls[1]->color.a = 0.0f;
    flag_set(&ctx.walls[1]->flags, WALL_FLAG_EXPLOSION_DELETE);

    ctx.walls[2] = wall_create(-38.0f, 38.0f, 76.0f, 8.0f);
    ctx.walls[2]->color.a = 0.0f;
    flag_set(&ctx.walls[2]->flags, WALL_FLAG_EXPLOSION_DELETE);

    ctx.walls[3] = wall_create(-38.0f, -30.0f, 76.0f, 8.0f);
    ctx.walls[3]->color.a = 0.0f;
    flag_set(&ctx.walls[3]->flags, WALL_FLAG_EXPLOSION_DELETE);
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
            if (ctx.wasd_rect == NULL && !ctx.show_walls && ctx.walls[0]->color.a < 1.0f)
            {
                ctx.walls[0]->color.a += 0.2f * dt;
                if (ctx.walls[0]->color.a > 1.0f)
                {
                    ctx.walls[0]->color.a = 1.0f;
                    ctx.show_walls = true;
                }
                ctx.walls[1]->color.a = ctx.walls[0]->color.a;
                ctx.walls[2]->color.a = ctx.walls[0]->color.a;
                ctx.walls[3]->color.a = ctx.walls[0]->color.a;
            }
            if (ctx.show_walls && !ctx.show_player_moved_msg)
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
                    //flag_set(&global_ctx.player.flags, PLAYER_FLAG_CAN_EXPLODE);
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
                ctx.show_tried_twice_msg = true;
                ctx.next_time = global_ctx.time + 8.0;
            }
            if (ctx.show_tried_twice_msg && !ctx.show_tried_twice_resp_msg && ctx.next_time < global_ctx.time)
            {
                char* msg = "It's not your nature to understand.";
                TimedText* text = timed_text_create(msg, global_ctx.time + 5.0, global_ctx.time + 18.0, 0.1);
                text->sound = SOUND_BREAK_DOOR;
                text->x = -calculate_text_width(msg, 1.0f, text->font) / 2.0f;
                text->y = 100.0f;
                ctx.show_tried_twice_resp_msg = true;
                ctx.next_time = global_ctx.time + 18.0;
            }
            if (ctx.show_tried_twice_resp_msg && ctx.next_time < global_ctx.time && key_pressed(KEY_EXPLOSION))
            {
                global_ctx.player.entity->animation.color.a += 0.03;
                audio_play(SOUND_BREAK_DOOR);
                if (global_ctx.player.entity->animation.color.a > 1.0f)
                {
                    global_ctx.player.entity->animation.color.a = 1.0f;
                    flag_set(&global_ctx.player.flags, PLAYER_FLAG_CAN_EXPLODE);
                    ctx.state = PLAYER_FREED;
                }
            }
            break;
        }
        case PLAYER_FREED:
        {
            break;
        }
        default:
            break;
    }
}
