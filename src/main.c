#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#include <dirent.h>
#include FT_FREETYPE_H
#include "util.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define FONT_SIZE 24
#define TEXTURE_WIDTH 1024
#define TEXTURE_SIZE (TEXTURE_WIDTH * TEXTURE_WIDTH)
#define GLYPH_START 32
#define GLYPH_END 127
#define NUM_GLYPHS (GLYPH_END - GLYPH_START + 1)
#define UBO_INDEX_WINDOW 1
#define MAX_TEXT_SIZE 1024

struct {

    FT_Library library;
    FT_Face face;

    stbrp_rect glyph_rects[NUM_GLYPHS];
    FT_Glyph_Metrics glyph_metrics[NUM_GLYPHS];
    
    GLuint shader_program;
    GLuint vao, vbo;
    GLuint window_ubo;
    GLuint tex;

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

        ctx.glyph_metrics[glyph - GLYPH_START] = ctx.face->glyph->metrics;

        stbrp_rect* rect = ctx.glyph_rects + glyph - GLYPH_START;
        rect->id = glyph;
        rect->w = ctx.face->glyph->bitmap.width;
        rect->h = ctx.face->glyph->bitmap.rows;
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

    glGenTextures(1, &ctx.tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, ctx.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ZERO);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, TEXTURE_WIDTH, TEXTURE_WIDTH, 0, GL_RED, GL_UNSIGNED_BYTE, buffer);
}

void render_text(char* text, f32 x, f32 y)
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
        
        GLfloat* vertex = vertices + n * 6 * 4;
        static f32 offset_x[4] = {0.0f,0.0f,1.0f,1.0f};
        static f32 offset_y[4] = {0.0f,1.0f,1.0f,0.0f};
        static i32 winding[6] = {0,1,2,0,2,3};
        for (i32 v = 0; v < 6; v++)
        {
            vertex[4*v]   = x + (metrics->horiBearingX >> 6) + offset_x[winding[v]] * ctx.glyph_rects[glyph_idx].w;
            vertex[4*v+1] = y +  offset_y[winding[v]] * ctx.glyph_rects[glyph_idx].h;
            vertex[4*v+2] = (float)(ctx.glyph_rects[glyph_idx].x + offset_x[winding[v]] * ctx.glyph_rects[glyph_idx].w) / TEXTURE_WIDTH;
            vertex[4*v+3] = (float)(ctx.glyph_rects[glyph_idx].y + offset_y[winding[v]] * ctx.glyph_rects[glyph_idx].h) / TEXTURE_WIDTH;
        }

        x += metrics->horiAdvance >> 6;
        n++;
    }
    if (i == MAX_TEXT_SIZE)
    {
        log(WARNING, "Render too much text");
        return;
    }
    glUseProgram(ctx.shader_program);
    glBindVertexArray(ctx.vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo);
    glBufferData(GL_ARRAY_BUFFER, n * 6 * 4 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, n * 6);
}

void create_shader_program()
{
    GLuint vert, frag;
    ctx.shader_program = glCreateProgram();
    vert = compile(GL_VERTEX_SHADER, "assets/shaders/text.vert");
    frag = compile(GL_FRAGMENT_SHADER, "assets/shaders/text.frag");
    glAttachShader(ctx.shader_program, vert);
    glAttachShader(ctx.shader_program, frag);
    link(ctx.shader_program);
    glDetachShader(ctx.shader_program, vert);
    glDetachShader(ctx.shader_program, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
    glUniform1i(glGetUniformLocation(ctx.shader_program, "glyphBitmap"), ctx.tex);
    glUniformBlockBinding(ctx.shader_program, glGetUniformBlockIndex(ctx.shader_program, "Window"), UBO_INDEX_WINDOW);
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
    glCreateVertexArrays(1, &ctx.vao);
    glGenBuffers(1, &ctx.vbo);
    glBindVertexArray(ctx.vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
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
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    initialize_font();
    create_window_ubo();
    create_shader_program();
    create_vertex_arrays();

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);

        render_text("Hello, World!", 40, 40);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
}
