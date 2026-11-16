#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdio.h>
#include <math.h>
#include <glad/glad.h>
#include <sys/time.h>
#include <GLFW/glfw3.h>
#include "util.h"

#define RESOLUTION_X 1920
#define RESOLUTION_Y 1080
#define UBO_INDEX_WINDOW 1
#define NUM_OBJECTS 1000

typedef struct {
    f32 x, y, z, w;
} vec4;

typedef struct {
    f32 x, y;
} vec2;

typedef struct {
    vec2 center;
    vec2 size;
    f32 rotation;
} Rect;

typedef struct {
    GLFWwindow* window;
    i32 window_width;
    i32 window_height;

    Rect rects[NUM_OBJECTS];
    i32 num_rects;

    f64 time;
    f64 dt;

} GlobalContext;

extern GlobalContext global_ctx;

void render_init(void);
void render(void);
void render_text(char* text, f32 x, f32 y);
void render_text_ex(char* text, f32 x, f32 y, f32 scale, vec4 color);
void render_rects(void);

#endif
