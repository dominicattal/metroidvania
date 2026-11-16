#include "context.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define UBO_INDEX_WINDOW 1
#define TEXTURE_WIDTH 1024
#define TEXTURE_SIZE (TEXTURE_WIDTH * TEXTURE_WIDTH)
#define GLYPH_START 32
#define GLYPH_END 127
#define NUM_GLYPHS (GLYPH_END - GLYPH_START + 1)
#define FONT_SIZE 36
#define BITMAP_PADDING 1
#define MAX_TEXT_SIZE 1024

#define RECT_VERTEX_LENGTH   5
#define CIRCLE_VERTEX_LENGTH 3

typedef enum {
   PROGRAM_SCREEN,
   PROGRAM_TEXT,
   PROGRAM_RECT,
   PROGRAM_CIRCLE,
   NUM_PROGRAMS
} Program;

typedef enum {
    VAO_SCREEN,
    VAO_TEXT,
    VAO_RECT,
    VAO_CIRCLE,
    NUM_VAOS
} VAO;

typedef enum {
    VBO_SCREEN,
    VBO_TEXT,
    VBO_QUAD,
    VBO_RECT,
    VBO_CIRCLE,
    NUM_VBOS
} VBO;

typedef enum {
    UBO_WINDOW,
    NUM_UBOS
} UBO;

typedef enum {
    TEXTURE_GLYPH_BITMAP,
    TEXTURE_FBO,
    NUM_TEXTURES
} Textures;

typedef struct {

    FT_Library library;
    FT_Face face;
    FT_Glyph_Metrics glyph_metrics[NUM_GLYPHS];
    stbrp_rect glyph_rects[NUM_GLYPHS];

    GLuint programs[NUM_PROGRAMS];
    GLuint vaos[NUM_VAOS];
    GLuint vbos[NUM_VBOS];
    GLuint ubos[NUM_UBOS];
    GLuint textures[NUM_TEXTURES];

    // max number of objects in pool + player
    GLfloat rect_vertex_buffer[MAX_NUM_ENTITIES * RECT_VERTEX_LENGTH];
    GLfloat circle_vertex_buffer[MAX_NUM_ENTITIES * CIRCLE_VERTEX_LENGTH];

    GLuint fbo;
    GLuint rbo;

} RenderContext;

RenderContext render_ctx;

static u32 compile(GLenum type, const char* shader_code)
{
    GLuint shader;
    char info_log[512];
    i32 success;

    shader = glCreateShader(type);
    glShaderSource(shader, 1, &shader_code, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        log(FATAL, "Failed to compile shader\n%s", info_log);
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

static void create_font(void)
{
    FT_Init_FreeType(&render_ctx.library);

    const uint8_t font_data[] =
    {
    #embed "../assets/fonts/Noto_Sans/NotoSans-VariableFont_wdth,wght.ttf"
    };

    FT_New_Memory_Face(render_ctx.library, font_data, sizeof(font_data), 0, &render_ctx.face);
    FT_Set_Pixel_Sizes(render_ctx.face, 0, FONT_SIZE);

    if (render_ctx.face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
    {
        FT_Error error = FT_Render_Glyph(render_ctx.face->glyph, FT_RENDER_MODE_NORMAL);
    }

    for (i32 glyph = GLYPH_START; glyph <= GLYPH_END; glyph++)
    {
        FT_Load_Char(render_ctx.face, glyph, FT_LOAD_RENDER);

        FT_Glyph_Metrics* metrics = render_ctx.glyph_metrics + glyph - GLYPH_START;
        metrics->width = render_ctx.face->glyph->metrics.width >> 6;
        metrics->height = render_ctx.face->glyph->metrics.height >> 6;
        metrics->horiBearingX = render_ctx.face->glyph->metrics.horiBearingX >> 6;
        metrics->horiBearingY = render_ctx.face->glyph->metrics.horiBearingY >> 6;
        metrics->horiAdvance = render_ctx.face->glyph->metrics.horiAdvance >> 6;

        stbrp_rect* rect = render_ctx.glyph_rects + glyph - GLYPH_START;
        rect->id = glyph;
        rect->w = render_ctx.face->glyph->bitmap.width + BITMAP_PADDING;
        rect->h = render_ctx.face->glyph->bitmap.rows + BITMAP_PADDING;
    }

    stbrp_context packing_context;
    stbrp_node glyph_nodes[NUM_GLYPHS];
    stbrp_init_target(&packing_context, TEXTURE_WIDTH, TEXTURE_WIDTH, glyph_nodes, NUM_GLYPHS);
    bool all_rects_packed = stbrp_pack_rects(&packing_context, render_ctx.glyph_rects, NUM_GLYPHS);

    u8 buffer[TEXTURE_SIZE];
    memset(buffer, 0, TEXTURE_SIZE);

    i32 num_packed = 0;
    for (i32 glyph = GLYPH_START; glyph <= GLYPH_END; glyph++)
    {
        FT_Load_Char(render_ctx.face, glyph, FT_LOAD_RENDER);
        stbrp_rect* rect = render_ctx.glyph_rects + glyph - GLYPH_START;
        log_assert(rect->was_packed, "Failed to pack glyph %d %c", glyph, glyph);
        for (i32 y = 0; y < render_ctx.face->glyph->bitmap.rows; y++)
        {
            for (i32 x = 0; x < render_ctx.face->glyph->bitmap.width; x++)
            {
                i32 bitmap_idx = (y + rect->y) * TEXTURE_WIDTH + (x + rect->x);
                i32 glyph_bitmap_idx = (render_ctx.face->glyph->bitmap.rows - y - 1) * render_ctx.face->glyph->bitmap.width + x;
                buffer[bitmap_idx] = render_ctx.face->glyph->bitmap.buffer[glyph_bitmap_idx];
            }
        }
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, render_ctx.textures[TEXTURE_GLYPH_BITMAP]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ZERO);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, TEXTURE_WIDTH, TEXTURE_WIDTH, 0, GL_RED, GL_UNSIGNED_BYTE, buffer);
}

static void create_shader_program_text()
{
    GLuint vert, frag;
    const char source_vert[] =
    {
    #embed "shaders/text.vert"
    , '\0'
    };
    const char source_frag[] =
    {
    #embed "shaders/text.frag"
    , '\0'
    };
    vert = compile(GL_VERTEX_SHADER, source_vert);
    frag = compile(GL_FRAGMENT_SHADER, source_frag);
    glAttachShader(render_ctx.programs[PROGRAM_TEXT], vert);
    glAttachShader(render_ctx.programs[PROGRAM_TEXT], frag);
    link(render_ctx.programs[PROGRAM_TEXT]);
    glDetachShader(render_ctx.programs[PROGRAM_TEXT], vert);
    glDetachShader(render_ctx.programs[PROGRAM_TEXT], frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
    glUniform1i(glGetUniformLocation(render_ctx.programs[PROGRAM_TEXT], "glyphBitmap"), render_ctx.textures[TEXTURE_GLYPH_BITMAP]);
    glUniformBlockBinding(render_ctx.programs[PROGRAM_TEXT], glGetUniformBlockIndex(render_ctx.programs[PROGRAM_TEXT], "Window"), UBO_INDEX_WINDOW);
}

static void create_shader_program_rect()
{
    GLuint vert, frag;
    const char source_vert[] =
    {
    #embed "shaders/rect.vert"
    , '\0'
    };
    const char source_frag[] =
    {
    #embed "shaders/rect.frag"
    , '\0'
    };
    create_shader_program_text();
    vert = compile(GL_VERTEX_SHADER, source_vert);
    frag = compile(GL_FRAGMENT_SHADER, source_frag);
    glAttachShader(render_ctx.programs[PROGRAM_RECT], vert);
    glAttachShader(render_ctx.programs[PROGRAM_RECT], frag);
    link(render_ctx.programs[PROGRAM_RECT]);
    glDetachShader(render_ctx.programs[PROGRAM_RECT], vert);
    glDetachShader(render_ctx.programs[PROGRAM_RECT], frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
    glUniformBlockBinding(render_ctx.programs[PROGRAM_RECT], glGetUniformBlockIndex(render_ctx.programs[PROGRAM_RECT], "Window"), UBO_INDEX_WINDOW);
}

static void create_shader_program_circle()
{
    GLuint vert, frag;
    const char source_vert[] =
    {
    #embed "shaders/circle.vert"
    , '\0'
    };
    const char source_frag[] =
    {
    #embed "shaders/circle.frag"
    , '\0'
    };
    create_shader_program_text();
    vert = compile(GL_VERTEX_SHADER, source_vert);
    frag = compile(GL_FRAGMENT_SHADER, source_frag);
    glAttachShader(render_ctx.programs[PROGRAM_CIRCLE], vert);
    glAttachShader(render_ctx.programs[PROGRAM_CIRCLE], frag);
    link(render_ctx.programs[PROGRAM_CIRCLE]);
    glDetachShader(render_ctx.programs[PROGRAM_CIRCLE], vert);
    glDetachShader(render_ctx.programs[PROGRAM_CIRCLE], frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
    glUniformBlockBinding(render_ctx.programs[PROGRAM_CIRCLE], glGetUniformBlockIndex(render_ctx.programs[PROGRAM_CIRCLE], "Window"), UBO_INDEX_WINDOW);
}

static void create_shader_program_screen()
{
    GLuint vert, frag;
    const char source_vert[] =
    {
    #embed "shaders/screen.vert"
    , '\0'
    };
    const char source_frag[] =
    {
    #embed "shaders/screen.frag"
    , '\0'
    };
    vert = compile(GL_VERTEX_SHADER, source_vert);
    frag = compile(GL_FRAGMENT_SHADER, source_frag);
    glAttachShader(render_ctx.programs[PROGRAM_SCREEN], vert);
    glAttachShader(render_ctx.programs[PROGRAM_SCREEN], frag);
    link(render_ctx.programs[PROGRAM_SCREEN]);
    glDetachShader(render_ctx.programs[PROGRAM_SCREEN], vert);
    glDetachShader(render_ctx.programs[PROGRAM_SCREEN], frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
    glUniform1i(glGetUniformLocation(render_ctx.programs[PROGRAM_SCREEN], "screenTex"), render_ctx.textures[TEXTURE_FBO]);
}

static void create_shader_programs(void)
{
    create_shader_program_text();
    create_shader_program_rect();
    create_shader_program_circle();
    create_shader_program_screen();
}

static void create_window_ubo(void)
{
    GLfloat window_size[2] = {RESOLUTION_X, RESOLUTION_Y};
    glBindBuffer(GL_UNIFORM_BUFFER, render_ctx.ubos[UBO_WINDOW]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBO_INDEX_WINDOW, render_ctx.ubos[UBO_WINDOW]);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(GLfloat), window_size, GL_STATIC_DRAW);
}

static void create_vertex_arrays(void)
{
    GLfloat quad_data[] = {
        -0.5f, -0.5f,
        -0.5f,  0.5f,
         0.5f,  0.5f,
         0.5f, -0.5f,
    };
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_QUAD]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);

    glBindVertexArray(render_ctx.vaos[VAO_TEXT]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_TEXT]);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(render_ctx.vaos[VAO_RECT]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_QUAD]);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_RECT]);
    glBufferData(GL_ARRAY_BUFFER, MAX_NUM_ENTITIES * 5 * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(render_ctx.vaos[VAO_CIRCLE]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_QUAD]);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_CIRCLE]);
    glBufferData(GL_ARRAY_BUFFER, MAX_NUM_ENTITIES * 3 * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);

    GLfloat screen_data[] = {
        -1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
         1.0f, -1.0f,
    };
    glBindVertexArray(render_ctx.vaos[VAO_SCREEN]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vaos[VBO_SCREEN]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_data), screen_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
}

static void create_framebuffer(void)
{
    glBindRenderbuffer(GL_RENDERBUFFER, render_ctx.rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, RESOLUTION_X, RESOLUTION_Y);

    glBindFramebuffer(GL_FRAMEBUFFER, render_ctx.fbo);
    glBindTexture(GL_TEXTURE_2D, render_ctx.textures[TEXTURE_FBO]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, RESOLUTION_X, RESOLUTION_Y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_ctx.textures[TEXTURE_FBO], 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, render_ctx.rbo);
}

void render_init(void)
{
    for (i32 i = 0; i < NUM_PROGRAMS; i++)
        render_ctx.programs[i] = glCreateProgram();

    glCreateVertexArrays(NUM_VAOS, render_ctx.vaos);
    glGenBuffers(NUM_VBOS, render_ctx.vbos);
    glGenBuffers(NUM_UBOS, render_ctx.ubos);
    glGenTextures(NUM_TEXTURES, render_ctx.textures);
    glGenRenderbuffers(1, &render_ctx.rbo);
    glGenFramebuffers(1, &render_ctx.fbo);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    create_font();
    create_framebuffer();
    create_window_ubo();
    create_shader_programs();
    create_vertex_arrays();
}

void render_text_ex(char* text, f32 x, f32 y, f32 scale, Color color)
{
    static GLfloat vertices[MAX_TEXT_SIZE * 6 * 4];

    i32 i, n;
    n = 0;
    for (i = 0; i < MAX_TEXT_SIZE && text[i] != '\0'; i++)
    {
        if (text[i] < GLYPH_START || text[i] > GLYPH_END)
            continue;

        i32 glyph_idx = text[i] - GLYPH_START;

        FT_Glyph_Metrics* metrics = render_ctx.glyph_metrics + glyph_idx;
        stbrp_rect* rect = render_ctx.glyph_rects + glyph_idx;

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

    glUseProgram(render_ctx.programs[PROGRAM_TEXT]);
    glUniform4f(glGetUniformLocation(render_ctx.programs[PROGRAM_TEXT], "color"), color.r, color.g, color.b, color.a);
    glBindVertexArray(render_ctx.vaos[VAO_TEXT]);
    glBindTexture(GL_TEXTURE_2D, render_ctx.textures[TEXTURE_GLYPH_BITMAP]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_TEXT]);
    glBufferData(GL_ARRAY_BUFFER, n * 6 * 4 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, n * 6);
}

void render_text(char* text, f32 x, f32 y)
{
    render_text_ex(text, x, y, 1.0f, (Color){0.0f, 0.0f, 0.0f, 1.0f});
}

static GLfloat* add_entity_to_buffer(GLfloat* buffer, Entity* entity)
{
    buffer[0] = entity->position.x;
    buffer[1] = entity->position.y;
    buffer[2] = entity->size.x;
    buffer[3] = entity->size.y;
    buffer[4] = fmod(entity->rotation, 2 * M_PI);
    return buffer + RECT_VERTEX_LENGTH;
}

void render_entities(void)
{
    Pool* pool = &global_ctx.projectile_pool;
    GLfloat* buffer = render_ctx.rect_vertex_buffer;

    for (i32 i = 0; i < pool->size; i++)
        buffer = add_entity_to_buffer(buffer, pool->entities + pool->dense[i]);

    buffer = add_entity_to_buffer(buffer, &global_ctx.player);

    // render each entity in pool + player
    i32 num_entities = pool->size + 1;

    glUseProgram(render_ctx.programs[PROGRAM_RECT]);
    glBindVertexArray(render_ctx.vaos[VAO_RECT]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_RECT]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_entities * RECT_VERTEX_LENGTH * sizeof(GLfloat), render_ctx.rect_vertex_buffer);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, num_entities);
}

static GLfloat* add_entity_hitbox_to_buffer(GLfloat* buffer, Entity* entity)
{
    buffer[0] = entity->position.x;
    buffer[1] = entity->position.y;
    buffer[2] = entity->hitbox_radius;
    return buffer + CIRCLE_VERTEX_LENGTH;
}

void render_entity_hitboxes(void)
{
    Pool* pool = &global_ctx.projectile_pool;
    GLfloat* buffer = render_ctx.circle_vertex_buffer;
    for (i32 i = 0; i < pool->size; i++)
        buffer = add_entity_hitbox_to_buffer(buffer, pool->entities + pool->dense[i]);

    buffer = add_entity_hitbox_to_buffer(buffer, &global_ctx.player);

    // render each entity in pool + player
    i32 num_entities = pool->size + 1;

    glUseProgram(render_ctx.programs[PROGRAM_CIRCLE]);
    glBindVertexArray(render_ctx.vaos[VAO_CIRCLE]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_CIRCLE]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_entities * CIRCLE_VERTEX_LENGTH * sizeof(GLfloat), render_ctx.circle_vertex_buffer);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, num_entities);
}

void render()
{
    glBindFramebuffer(GL_FRAMEBUFFER, render_ctx.fbo);
    glViewport(0, 0, RESOLUTION_X, RESOLUTION_Y);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);

    render_entities();
    if (global_ctx.draw_hitboxes)
        render_entity_hitboxes();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, global_ctx.window_width, global_ctx.window_height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glUseProgram(render_ctx.programs[PROGRAM_SCREEN]);
    glBindVertexArray(render_ctx.vaos[VAO_SCREEN]);
    glBindTexture(GL_TEXTURE_2D, render_ctx.textures[TEXTURE_FBO]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
