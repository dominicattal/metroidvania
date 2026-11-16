#include <stdio.h>
#include <math.h>
#include <glad/glad.h>
#include <sys/time.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#include FT_FREETYPE_H
#include "util.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define FONT_SIZE 36
#define TEXTURE_WIDTH 1024
#define TEXTURE_SIZE (TEXTURE_WIDTH * TEXTURE_WIDTH)
#define GLYPH_START 32
#define GLYPH_END 127
#define NUM_GLYPHS (GLYPH_END - GLYPH_START + 1)
#define UBO_INDEX_WINDOW 1
#define MAX_TEXT_SIZE 1024
#define BITMAP_PADDING 1
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

struct {

    FT_Library library;
    FT_Face face;

    stbrp_rect glyph_rects[NUM_GLYPHS];
    FT_Glyph_Metrics glyph_metrics[NUM_GLYPHS];

    GLuint window_ubo;
    
    GLuint text_shader_program;
    GLuint text_vao;
    GLuint text_vbo;
    GLuint glyph_bitmap_texture;

    GLuint rect_shader_program;
    GLuint rect_vao;
    GLuint rect_vbo;
    GLuint rect_instance_vbo;

    Rect rects[NUM_OBJECTS];
    i32 num_rects;

} ctx;

void window_error_callback(int x, const char* message)
{
    log(ERROR, message);
}

static char* read_file(const char* path)
{
    FILE* ptr;
    char* content;
    i32 len;
    static char buffer[1028];
    ptr = fopen(path, "r");
    fseek(ptr, 0, SEEK_END);
    len = ftell(ptr);
    log_assert(len != 0, "File %s is empty", path);
    fseek(ptr, 0, SEEK_SET);
    len = fread(buffer, 1, len, ptr);
    fclose(ptr);
    buffer[len] = '\0';
    return buffer;
}

static u32 compile(GLenum type, const char* path)
{
    u32 shader;
    char* shader_code;
    char info_log[512];
    i32 success;

    shader = glCreateShader(type);
    shader_code = read_file(path);
    glShaderSource(shader, 1, (const char* const*)&shader_code, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        log(FATAL, "Failed to compile %s\n%s", path, info_log);
    }
    return shader;
}

static void link(GLuint program)
{
    char info_log[512];
    i32 success;
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, info_log);
        log(FATAL, "Failed to link shader %d\n%s", program, info_log);
    }
}

void initialize_font()
{
    FT_Init_FreeType(&ctx.library);
    FT_New_Face(ctx.library, "assets/fonts/Noto_Sans/NotoSans-VariableFont_wdth,wght.ttf", 0, &ctx.face);
    FT_Set_Pixel_Sizes(ctx.face, 0, FONT_SIZE);

    if (ctx.face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
    {
        FT_Error error = FT_Render_Glyph(ctx.face->glyph, FT_RENDER_MODE_NORMAL);
    }

    for (i32 glyph = GLYPH_START; glyph <= GLYPH_END; glyph++)
    {
        FT_Load_Char(ctx.face, glyph, FT_LOAD_RENDER);

        FT_Glyph_Metrics* metrics = ctx.glyph_metrics + glyph - GLYPH_START;
        metrics->width = ctx.face->glyph->metrics.width >> 6;
        metrics->height = ctx.face->glyph->metrics.height >> 6;
        metrics->horiBearingX = ctx.face->glyph->metrics.horiBearingX >> 6;
        metrics->horiBearingY = ctx.face->glyph->metrics.horiBearingY >> 6;
        metrics->horiAdvance = ctx.face->glyph->metrics.horiAdvance >> 6;

        stbrp_rect* rect = ctx.glyph_rects + glyph - GLYPH_START;
        rect->id = glyph;
        rect->w = ctx.face->glyph->bitmap.width + BITMAP_PADDING;
        rect->h = ctx.face->glyph->bitmap.rows + BITMAP_PADDING;
    }

    stbrp_context packing_context;
    stbrp_node glyph_nodes[NUM_GLYPHS];
    stbrp_init_target(&packing_context, TEXTURE_WIDTH, TEXTURE_WIDTH, glyph_nodes, NUM_GLYPHS);
    bool all_rects_packed = stbrp_pack_rects(&packing_context, ctx.glyph_rects, NUM_GLYPHS);

    u8 buffer[TEXTURE_SIZE];
    memset(buffer, 0, TEXTURE_SIZE);

    i32 num_packed = 0;
    for (i32 glyph = GLYPH_START; glyph <= GLYPH_END; glyph++)
    {
        FT_Load_Char(ctx.face, glyph, FT_LOAD_RENDER);
        stbrp_rect* rect = ctx.glyph_rects + glyph - GLYPH_START;
        log_assert(rect->was_packed, "Failed to pack glyph %d %c", glyph, glyph);
        for (i32 y = 0; y < ctx.face->glyph->bitmap.rows; y++)
        {
            for (i32 x = 0; x < ctx.face->glyph->bitmap.width; x++)
            {
                i32 bitmap_idx = (y + rect->y) * TEXTURE_WIDTH + (x + rect->x);
                i32 glyph_bitmap_idx = (ctx.face->glyph->bitmap.rows - y - 1) * ctx.face->glyph->bitmap.width + x;
                buffer[bitmap_idx] = ctx.face->glyph->bitmap.buffer[glyph_bitmap_idx];
            }
        }
    }

    glGenTextures(1, &ctx.glyph_bitmap_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, ctx.glyph_bitmap_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ZERO);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, TEXTURE_WIDTH, TEXTURE_WIDTH, 0, GL_RED, GL_UNSIGNED_BYTE, buffer);
}

void render_text_ex(char* text, f32 x, f32 y, f32 scale, vec4 color)
{
    GLfloat vertices[MAX_TEXT_SIZE * 6 * 4];

    i32 i, n;
    n = 0;
    for (i = 0; i < MAX_TEXT_SIZE && text[i] != '\0'; i++)
    {
        if (text[i] < GLYPH_START || text[i] > GLYPH_END)
            continue;

        i32 glyph_idx = text[i] - GLYPH_START;

        FT_Glyph_Metrics* metrics = ctx.glyph_metrics + glyph_idx;
        stbrp_rect* rect = ctx.glyph_rects + glyph_idx;

        GLfloat* vertex = vertices + n * 6 * 4;
        static f32 offset_x[4] = {0.0f,0.0f,1.0f,1.0f};
        static f32 offset_y[4] = {0.0f,1.0f,1.0f,0.0f};
        static i32 winding[6] = {0,1,2,0,2,3};
        for (i32 v = 0; v < 6; v++)
        {
            vertex[4*v]   = x + (metrics->horiBearingX + offset_x[winding[v]] * rect->w) * scale;
            vertex[4*v+1] = y - ((rect->h - metrics->horiBearingY) - offset_y[winding[v]] * rect->h) * scale;
            vertex[4*v+2] = (float)(rect->x + offset_x[winding[v]] * metrics->width) / TEXTURE_WIDTH;
            vertex[4*v+3] = (float)(rect->y + offset_y[winding[v]] * metrics->height) / TEXTURE_WIDTH;
        }

        x += metrics->horiAdvance * scale;
        n++;
    }
    if (i == MAX_TEXT_SIZE)
    {
        log(WARNING, "Render too much text");
        return;
    }

    glUseProgram(ctx.text_shader_program);
    glUniform4f(glGetUniformLocation(ctx.text_shader_program, "color"), color.x, color.y, color.z, color.w);
    glBindVertexArray(ctx.text_vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx.text_vbo);
    glBufferData(GL_ARRAY_BUFFER, n * 6 * 4 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, n * 6);
}

void render_text(char* text, f32 x, f32 y)
{
    render_text_ex(text, x, y, 1.0f, (vec4){0.0f, 0.0f, 0.0f, 1.0f});
}

void create_shader_programs()
{
    GLuint vert, frag;
    ctx.text_shader_program = glCreateProgram();
    vert = compile(GL_VERTEX_SHADER, "assets/shaders/text.vert");
    frag = compile(GL_FRAGMENT_SHADER, "assets/shaders/text.frag");
    glAttachShader(ctx.text_shader_program, vert);
    glAttachShader(ctx.text_shader_program, frag);
    link(ctx.text_shader_program);
    glDetachShader(ctx.text_shader_program, vert);
    glDetachShader(ctx.text_shader_program, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
    glUniform1i(glGetUniformLocation(ctx.text_shader_program, "glyphBitmap"), ctx.glyph_bitmap_texture);
    glUniformBlockBinding(ctx.text_shader_program, glGetUniformBlockIndex(ctx.text_shader_program, "Window"), UBO_INDEX_WINDOW);

    ctx.rect_shader_program = glCreateProgram();
    vert = compile(GL_VERTEX_SHADER, "assets/shaders/rect.vert");
    frag = compile(GL_FRAGMENT_SHADER, "assets/shaders/rect.frag");
    glAttachShader(ctx.rect_shader_program, vert);
    glAttachShader(ctx.rect_shader_program, frag);
    link(ctx.rect_shader_program);
    glDetachShader(ctx.rect_shader_program, vert);
    glDetachShader(ctx.rect_shader_program, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
    glUniformBlockBinding(ctx.rect_shader_program, glGetUniformBlockIndex(ctx.text_shader_program, "Window"), UBO_INDEX_WINDOW);
}

void create_window_ubo()
{
    GLfloat window_size[2] = {WINDOW_WIDTH, WINDOW_HEIGHT};
    glGenBuffers(1, &ctx.window_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ctx.window_ubo);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBO_INDEX_WINDOW, ctx.window_ubo);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(GLfloat), window_size, GL_STATIC_DRAW);
}

void create_vertex_arrays()
{
    glCreateVertexArrays(1, &ctx.text_vao);
    glGenBuffers(1, &ctx.text_vbo);
    glBindVertexArray(ctx.text_vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx.text_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    GLfloat rect_instance_data[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
    };
    glCreateVertexArrays(1, &ctx.rect_vao);
    glGenBuffers(1, &ctx.rect_vbo);
    glGenBuffers(1, &ctx.rect_instance_vbo);
    glBindVertexArray(ctx.rect_vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx.rect_instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rect_instance_data), rect_instance_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, ctx.rect_vbo);
    glBufferData(GL_ARRAY_BUFFER, NUM_OBJECTS * 5 * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
}

void framebuffer_size_callback(GLFWwindow* window, i32 width, i32 height)
{
    glViewport(0, 0, width, height);
    GLfloat window_size[2] = {width, height};
    glBindBuffer(GL_UNIFORM_BUFFER, ctx.window_ubo);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(GLfloat), window_size, GL_STATIC_DRAW);
}

void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
{
}

void render_rects()
{
    f32 buffer[NUM_OBJECTS * 5];

    for (i32 i = 0; i < ctx.num_rects; i++)
    {
        buffer[5*i] = ctx.rects[i].center.x;
        buffer[5*i+1] = ctx.rects[i].center.y;
        buffer[5*i+2] = ctx.rects[i].size.x;
        buffer[5*i+3] = ctx.rects[i].size.y;
        buffer[5*i+4] = ctx.rects[i].rotation;
    }

    glUseProgram(ctx.rect_shader_program);
    glBindVertexArray(ctx.rect_vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx.rect_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, ctx.num_rects * 5 * sizeof(GLfloat), buffer);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, ctx.num_rects);
}

int main()
{
    glfwSetErrorCallback(window_error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello World", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSwapInterval(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    initialize_font();
    create_window_ubo();
    create_shader_programs();
    create_vertex_arrays();

    Rect rects[] = {
        {10.0f, 20.0f, 5.0f, 5.0f, 0.0f},
        {200.0f, 300.0f, 40.0f, 40.0f, 0.2f},
        {500.0f, 100.0f, 100.0f, 20.0f, 1.6f},
        {300, 200, 10.0f, 80.0f, 0.2}
    };
    memcpy(ctx.rects, rects, sizeof(rects));
    ctx.num_rects = 4;

    f64 timestep = 1.0 / 60.0;
    f64 time = 0;

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);

        render_text("The quick, brown fox jumps over the lazy dog.?!", 40, 120);

        f64 v = sin(time);
        render_text_ex("Hello, World!", 40, 40, 0.8 + 0.4 * v, (vec4){1.0f, 0.0f, 1.0f, 1.0f});

        ctx.rects[1].rotation = time;
        ctx.rects[2].rotation = sin(time);
        render_rects();

        glfwSwapBuffers(window);
        glfwPollEvents();

        time += timestep;
    }
    glfwTerminate();
}
