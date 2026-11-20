#include "../context.h"

typedef struct {
    f64 next_char_time;
    char* start;
    char* end;
} Data;

static Data data;

void screen_room_1_load(void)
{
    ui_load(UI_NONE);
    player_init(0.0, 0.0);
    Wall* wall = arena_deref(&global_ctx.wall_arena, arena_alloc(&global_ctx.wall_arena));
    wall->position = vec2_create(30.0f, 30.0f);
    wall->size = vec2_create(8.0f, 60.0f);
    wall = arena_deref(&global_ctx.wall_arena, arena_alloc(&global_ctx.wall_arena));
    wall->position = vec2_create(-38.0f, 30.0f);
    wall->size = vec2_create(8.0f, 60.0f);
    wall = arena_deref(&global_ctx.wall_arena, arena_alloc(&global_ctx.wall_arena));
    wall->position = vec2_create(-38.0f, 38.0f);
    wall->size = vec2_create(76.0f, 8.0f);
    wall = arena_deref(&global_ctx.wall_arena, arena_alloc(&global_ctx.wall_arena));
    wall->position = vec2_create(-38.0f, -30.0f);
    wall->size = vec2_create(76.0f, 8.0f);
    data.start = "Hello, World!";
    data.end = data.start;
    data.next_char_time = global_ctx.time + 0.2;
}

void screen_room_1_update(f64 dt)
{
    if (data.next_char_time < global_ctx.time && *data.end != '\0')
    {
        data.next_char_time += 0.2;
        data.end++;
    }
}

void screen_room_1_render(void)
{
    render_text_ex(data.start, data.end, 50, 50, 1.0f, (Color){1.0f, 1.0f, 1.0f, 1.0f});
}
