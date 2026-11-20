#include "context.h"

#define MAX_NUM_ANIMATIONS 100

typedef struct {
    Arena animations;

    Animation* lives;
} UIContext;

static UIContext ui_ctx;

// Flip y
static void ui_rect(vec2 pos, vec2 size, Color color)
{
    pos.y = RESOLUTION_Y - pos.y;
    render_rect(pos, size, color);
}

static void ui_rect_ex(vec2 pos, vec2 size, Color color, Animation* animation)
{
    pos.y = RESOLUTION_Y - pos.y;
    render_rect_ex(pos, size, color, animation);
}

static void draw_main_menu(void)
{
    static i32 selected_idx;
    char* list[] = {
        "Start",
        "Test",
        "Quit"
    };

    i32 list_length = sizeof(list) / sizeof(char*);

    if (key_pressed(GLFW_KEY_ENTER))
    {
        if (selected_idx == 0)
            screen_load(SCREEN_1);// start game
        else if (selected_idx == 2)
            glfwSetWindowShouldClose(global_ctx.window, true);
    }

    if (key_pressed(GLFW_KEY_W) && selected_idx - 1 >= 0) 
        --selected_idx;
    if (key_pressed(GLFW_KEY_S) && selected_idx + 1 < list_length) 
        ++selected_idx;

    Color selected_color = (Color){1.0f, 0.0f, 0.0f, 1.0f};
    Color unselected_color = (Color){1.0f, 1.0f, 1.0f, 1.0f};

    f32 y = 350.0f;
    f32 text_height = calculate_text_height(1.0f);
    for (i32 idx = 0; idx < list_length; idx++)
    {
        f32 text_width = calculate_text_width(list[idx], 1.0f, FONT_DEFAULT);
        ui_render_text_ex(list[idx], NULL, (RESOLUTION_X - text_width) / 2.0f, y, 1.0f,
            (selected_idx == idx) ? selected_color : unselected_color);
        y += text_height * 1.2;
    }
}

static void draw_game(void)
{
    vec2 rect_pos = vec2_create(10, 10);
    vec2 rect_size = vec2_create(48, 48);
    Color color = (Color){1.0f, 1.0f, 1.0f, 1.0f};
    for (i32 i = 0; i < 3; i++)
    {
        ui_rect_ex(rect_pos, rect_size, color, ui_ctx.lives);
        rect_pos.x += 60.0f;
    }

    rect_pos = vec2_create(10, 64);
    rect_size = vec2_create(150, 20);
    color = (Color){0.5f, 1.0f, 1.0f, 1.0f};
    ui_rect(rect_pos, rect_size, color);
}

void ui_init(void)
{
    arena_init(&ui_ctx.animations, sizeof(Animation), MAX_NUM_ANIMATIONS);
}

void ui_load(UI ui)
{
    arena_clear(&ui_ctx.animations);
    global_ctx.ui = ui;
    switch (ui)
    {
        case UI_GAME:
        {
            ui_ctx.lives = arena_deref(&ui_ctx.animations, arena_alloc(&ui_ctx.animations));
            break;
        }
        default:
            break;
    }
}

void ui_draw(void)
{
    switch (global_ctx.ui)
    {
        case UI_MAIN_MENU: return draw_main_menu();
        case UI_GAME: return draw_game();
        default: break;
    }
}

void ui_update(f64 dt)
{
    Arena* arena = &ui_ctx.animations;
    for (i32 i = 0; i < arena->size; i++)
        animation_update(arena_at(arena, i), dt);
}
