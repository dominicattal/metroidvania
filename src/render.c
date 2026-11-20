#include "context.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define UBO_INDEX_WINDOW 1
#define TEXTURE_WIDTH 1024
#define TEXTURE_SIZE (TEXTURE_WIDTH * TEXTURE_WIDTH)
#define GLYPH_START 32
#define GLYPH_END 127
#define NUM_GLYPHS (GLYPH_END - GLYPH_START + 1)
#define FONT_SIZE 36
#define BITMAP_PADDING 1
#define MAX_TEXT_SIZE 1024
#define MAX_NUM_FRAMES 8

#define RECT_VERTEX_LENGTH   13
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
    TEXTURE_SPRITES,
    NUM_TEXTURES
} Textures;

typedef struct {
    vec2 position;
    vec2 size;
    vec2 uv_offset;
    vec2 uv_size;
    Color color;
    f32 rotation;
} RectVertex;

typedef struct {
    vec2 position;
    f32 radius;
} CircleVertex;

typedef struct {
    i32 num_frames;
    struct {
        f32 u, v, w, h;
        f64 duration;
    } frames[MAX_NUM_FRAMES];
} AnimationData;

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

    AnimationData animations[NUM_ANIMATIONS];

    // max number of objects in arena + player
    GLfloat rect_vertex_buffer[MAX_NUM_RECTS * RECT_VERTEX_LENGTH];
    GLfloat circle_vertex_buffer[MAX_NUM_RECTS * CIRCLE_VERTEX_LENGTH];

    struct {
        f32 u, v, w, h;
    } solid_color;

    GLuint fbo;
    GLuint rbo;

} RenderContext;

static RenderContext render_ctx;

static uint8_t font_data[] =
{
#embed "../assets/fonts/Noto_Sans/NotoSans-VariableFont_wdth,wght.ttf"
};

static u8 buffer[TEXTURE_SIZE];

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
    stbrp_node glyph_nodes[TEXTURE_WIDTH];
    stbrp_init_target(&packing_context, TEXTURE_WIDTH, TEXTURE_WIDTH, glyph_nodes, TEXTURE_WIDTH);
    bool all_rects_packed = stbrp_pack_rects(&packing_context, render_ctx.glyph_rects, NUM_GLYPHS);


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
    glUniform1i(glGetUniformLocation(render_ctx.programs[PROGRAM_TEXT], "glyphBitmap"), GL_TEXTURE0);
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
    glUniform1i(glGetUniformLocation(render_ctx.programs[PROGRAM_RECT], "spriteTextures"), GL_TEXTURE0);
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
    glUniform1i(glGetUniformLocation(render_ctx.programs[PROGRAM_SCREEN], "screenTex"), GL_TEXTURE0);
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
    GLfloat window_size[5] = {RESOLUTION_X, RESOLUTION_Y, 0.0f, 0.0f, 0.0f};
    glBindBuffer(GL_UNIFORM_BUFFER, render_ctx.ubos[UBO_WINDOW]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBO_INDEX_WINDOW, render_ctx.ubos[UBO_WINDOW]);
    glBufferData(GL_UNIFORM_BUFFER, 5 * sizeof(GLfloat), window_size, GL_STATIC_DRAW);
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
    glBufferData(GL_ARRAY_BUFFER, MAX_NUM_RECTS * 13 * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 13 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 13 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 13 * sizeof(GLfloat), (void*)(4 * sizeof(GLfloat)));
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 13 * sizeof(GLfloat), (void*)(8 * sizeof(GLfloat)));
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 13 * sizeof(GLfloat), (void*)(12 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);

    glBindVertexArray(render_ctx.vaos[VAO_CIRCLE]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_QUAD]);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_CIRCLE]);
    glBufferData(GL_ARRAY_BUFFER, MAX_NUM_RECTS * 3 * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
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

void animation_init(Animation* animation, AnimationType type)
{
    animation->type = type;
    animation->duration = 0;
    animation->frame = 0;
}

void animation_update(Animation* animation, f64 dt)
{
    if (animation->type == ANIMATION_SOLID_COLOR)
        return;

    AnimationData* data = render_ctx.animations + animation->type;
    animation->duration += dt;
    if (animation->duration > data->frames[animation->frame].duration)
    {
        animation->duration = fmod(animation->duration, data->frames[animation->frame].duration);
        animation->frame = (animation->frame + 1) % data->num_frames;
    }
}

static void upload_spritesheet(AnimationData* animation_data, JsonObject* root, stbrp_rect* rect)
{
    JsonValue* meta = json_object_get_value(root, "meta");
    assert(meta);
    assert(json_value_get_type(meta) == JTYPE_OBJECT);

    JsonObject* meta_object = json_value_get_object(meta);
    assert(meta_object);

    JsonValue* image = json_object_get_value(meta_object, "image");
    assert(image);
    assert(json_value_get_type(image) == JTYPE_STRING);

    const char* file_name = json_value_get_string(image);
    i32 img_width, img_height, img_channels;
    u8* img_data = stbi_load(file_name, &img_width, &img_height, &img_channels, 4);
    log_assert(img_data, "Could not load img %s", file_name);
    assert(img_width == rect->w);
    assert(img_height == rect->h);

    glTexSubImage2D(GL_TEXTURE_2D, 0, rect->x, rect->y, rect->w, rect->h, GL_RGBA, GL_UNSIGNED_BYTE, img_data);

    stbi_image_free(img_data);

    JsonValue* frames = json_object_get_value(root, "frames");
    assert(frames);
    assert(json_value_get_type(frames) == JTYPE_OBJECT);

    JsonObject* frames_object = json_value_get_object(frames);
    assert(frames_object);

    animation_data->num_frames = json_object_length(frames_object);
    assert(animation_data->num_frames < MAX_NUM_FRAMES);

    for (i32 frame_idx = 0; frame_idx < animation_data->num_frames; frame_idx++)
    {
        char frame_key[16];
        snprintf(frame_key, sizeof(frame_key), "%d", frame_idx);

        JsonValue* frame = json_object_get_value(frames_object, frame_key);
        log_assert(frame, "Could not parse frame %d. Frame keys should be numbered starting from 0", frame);
        assert(json_value_get_type(frame) == JTYPE_OBJECT);

        JsonObject* frame_object = json_value_get_object(frame);
        assert(frame_object);

        JsonValue* duration = json_object_get_value(frame_object, "duration");
        assert(duration);
        assert(json_value_get_type(duration) == JTYPE_INT);

        i32 ms = json_value_get_int(duration);
        animation_data->frames[frame_idx].duration = (f64)ms / 1000.0;

        JsonValue* frame_uv = json_object_get_value(frame_object, "frame");
        assert(frame_uv);
        assert(json_value_get_type(frame_uv) == JTYPE_OBJECT);

        JsonObject* frame_uv_object = json_value_get_object(frame_uv);
        assert(frame_uv_object);

        JsonValue* x_value = json_object_get_value(frame_uv_object, "x");
        JsonValue* y_value = json_object_get_value(frame_uv_object, "y");
        JsonValue* w_value = json_object_get_value(frame_uv_object, "w");
        JsonValue* h_value = json_object_get_value(frame_uv_object, "h");
        assert(x_value && json_value_get_type(x_value));
        assert(y_value && json_value_get_type(y_value));
        assert(w_value && json_value_get_type(w_value));
        assert(h_value && json_value_get_type(h_value));
        i32 x = json_value_get_int(x_value);
        i32 y = json_value_get_int(y_value);
        i32 w = json_value_get_int(w_value);
        i32 h = json_value_get_int(h_value);

        animation_data->frames[frame_idx].u = (f32)(rect->x + x) / TEXTURE_WIDTH;
        animation_data->frames[frame_idx].v = (f32)(rect->x + y) / TEXTURE_WIDTH;
        animation_data->frames[frame_idx].w = (f32)w / TEXTURE_WIDTH;
        animation_data->frames[frame_idx].h = (f32)h / TEXTURE_WIDTH;
    }
}

static void parse_spritesheet(JsonObject* root, stbrp_rect* rect)
{
    JsonValue* meta = json_object_get_value(root, "meta");
    assert(meta);
    assert(json_value_get_type(meta) == JTYPE_OBJECT);

    JsonObject* meta_object = json_value_get_object(meta);
    assert(meta_object);

    JsonValue* size = json_object_get_value(meta_object, "size");
    assert(size);
    assert(json_value_get_type(size) == JTYPE_OBJECT);

    JsonObject* size_object = json_value_get_object(size);
    assert(size_object);

    JsonValue* img_width = json_object_get_value(size_object, "w");
    assert(img_width);
    assert(json_value_get_type(img_width) == JTYPE_INT);

    JsonValue* img_height = json_object_get_value(size_object, "h");
    assert(img_height);
    assert(json_value_get_type(img_height) == JTYPE_INT);

    rect->w = json_value_get_int(img_width);
    rect->h = json_value_get_int(img_height);
}

static void create_sprite_textures(void)
{
    AnimationData* animations = render_ctx.animations;
    stbrp_context packing_context;
    JsonObject* animation_configs[NUM_ANIMATIONS];
    // add 1 for solid color
    stbrp_rect texture_rects[NUM_ANIMATIONS+1];
    stbrp_node texture_nodes[TEXTURE_WIDTH];

    animation_configs[ANIMATION_SOUL] = json_read("assets/sprites/soul.json");

    for (i32 anim = 0; anim < NUM_ANIMATIONS; anim++)
    {
        JsonObject* root = animation_configs[anim];
        stbrp_rect* rect = texture_rects + anim;
        assert(root);
        parse_spritesheet(root, rect);
    }

    texture_rects[NUM_ANIMATIONS].w = 1 + BITMAP_PADDING;
    texture_rects[NUM_ANIMATIONS].h = 1 + BITMAP_PADDING;

    stbrp_init_target(&packing_context, TEXTURE_WIDTH, TEXTURE_WIDTH, texture_nodes, TEXTURE_WIDTH);
    bool all_rects_packed = stbrp_pack_rects(&packing_context, texture_rects, NUM_ANIMATIONS+1);
    log_assert(all_rects_packed, "Could not pack all the textures");

    glBindTexture(GL_TEXTURE_2D, render_ctx.textures[TEXTURE_SPRITES]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXTURE_WIDTH, TEXTURE_WIDTH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    for (i32 anim = 0; anim < NUM_ANIMATIONS; anim++)
    {
        AnimationData* animation_data = render_ctx.animations + anim;
        JsonObject* root = animation_configs[anim];
        stbrp_rect* rect = texture_rects + anim;
        upload_spritesheet(animation_data, root, rect);
    }

    stbrp_rect* rect = texture_rects + NUM_ANIMATIONS;
    u8 white_data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    glTexSubImage2D(GL_TEXTURE_2D, 0, rect->x, rect->y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, white_data);

    render_ctx.solid_color.u = (f32)rect->x / TEXTURE_WIDTH;
    render_ctx.solid_color.v = (f32)rect->y / TEXTURE_WIDTH;
    render_ctx.solid_color.w = 1.0f / TEXTURE_WIDTH;
    render_ctx.solid_color.h = 1.0f / TEXTURE_WIDTH;

    for (i32 i = 0; i < NUM_ANIMATIONS; i++)
        json_object_destroy(animation_configs[i]);
}

void render_init(void)
{
    global_ctx.draw_entities = true;

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
    create_sprite_textures();
    create_framebuffer();
    create_window_ubo();
    create_shader_programs();
    create_vertex_arrays();
}

void render_text_ex(char* start, char* end, f32 x, f32 y, f32 scale, Color color)
{
    static GLfloat vertices[MAX_TEXT_SIZE * 6 * 4];

    char* cur = start;
    i32 i, n;
    n = 0;
    for (; (cur - start) < MAX_TEXT_SIZE && cur != end && *cur != '\0'; cur++)
    {
        char c = *cur;
        if (c < GLYPH_START || c > GLYPH_END)
            continue;

        i32 glyph_idx = c - GLYPH_START;

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
    if ((cur - start) == MAX_TEXT_SIZE)
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
    render_text_ex(text, NULL, x, y, 1.0f, (Color){1.0f, 1.0f, 1.0f, 1.0f});
}

void ui_render_text(char* text, f32 x, f32 y)
{
    GLfloat window_size[5] = {RESOLUTION_X, RESOLUTION_Y, 0, 0, global_ctx.time};
    glBindBuffer(GL_UNIFORM_BUFFER, render_ctx.ubos[UBO_WINDOW]);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 5 * sizeof(GLfloat), window_size);
    render_text_ex(text, NULL, x, RESOLUTION_Y - y, 1.0f, (Color){1.0f, 1.0f, 1.0f, 1.0f});
}

void ui_render_text_ex(char* start, char* end, f32 x, f32 y, f32 scale, Color color)
{
    GLfloat window_size[5] = {RESOLUTION_X, RESOLUTION_Y, 0, 0, global_ctx.time};
    glBindBuffer(GL_UNIFORM_BUFFER, render_ctx.ubos[UBO_WINDOW]);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 5 * sizeof(GLfloat), window_size);
    render_text_ex(start, end, x, RESOLUTION_Y - y, scale, color);
}

f32 calculate_text_height(f32 scale)
{
    return scale * FONT_SIZE;
}

f32 calculate_text_width(char* text, f32 scale)
{
    f32 width = 0;
    for (i32 i = 0; i < MAX_TEXT_SIZE && text[i] != '\0'; i++)
    {
        if (text[i] < GLYPH_START || text[i] > GLYPH_END)
            continue;

        i32 glyph_idx = text[i] - GLYPH_START;

        FT_Glyph_Metrics* metrics = render_ctx.glyph_metrics + glyph_idx;
        stbrp_rect* rect = render_ctx.glyph_rects + glyph_idx;

        width += metrics->horiAdvance * scale;
    }
    return width;
}

static GLfloat* add_rect_to_buffer(GLfloat* buffer, RectVertex rect)
{
    buffer[0]  = rect.position.x;
    buffer[1]  = rect.position.y;
    buffer[2]  = rect.size.x;
    buffer[3]  = rect.size.y;
    buffer[4]  = rect.uv_offset.x;
    buffer[5]  = rect.uv_offset.y;
    buffer[6]  = rect.uv_size.x;
    buffer[7]  = rect.uv_size.y;
    buffer[8]  = rect.color.r;
    buffer[9]  = rect.color.g;
    buffer[10] = rect.color.b;
    buffer[11] = rect.color.a;
    buffer[12] = rect.rotation;
    return buffer + RECT_VERTEX_LENGTH;
}

static GLfloat* add_circle_to_buffer(GLfloat* buffer, CircleVertex circle)
{
    buffer[0] = circle.position.x;
    buffer[1] = circle.position.y;
    buffer[2] = circle.radius;
    return buffer + CIRCLE_VERTEX_LENGTH;
}

void render_rect_ex(vec2 pos, vec2 size, Color color, Animation* animation)
{
    GLfloat buffer[RECT_VERTEX_LENGTH];
    buffer[0] = pos.x + size.x / 2;
    buffer[1] = pos.y - size.y / 2;
    buffer[2] = size.x;
    buffer[3] = size.y;

    if (animation->type != ANIMATION_SOLID_COLOR)
    {
        AnimationData* data = render_ctx.animations + animation->type;
        buffer[4] = data->frames[animation->frame].u;
        buffer[5] = data->frames[animation->frame].v;
        buffer[6] = data->frames[animation->frame].w;
        buffer[7] = data->frames[animation->frame].h;
    }
    else
    {
        buffer[4] = render_ctx.solid_color.u;
        buffer[5] = render_ctx.solid_color.v;
        buffer[6] = render_ctx.solid_color.w;
        buffer[7] = render_ctx.solid_color.h;
    }

    buffer[8] = color.r;
    buffer[9] = color.g;
    buffer[10] = color.b;
    buffer[11] = color.a;

    buffer[12] = 0.0f;

    GLfloat window_size[5] = {RESOLUTION_X, RESOLUTION_Y, 0, 0, global_ctx.time};
    glBindBuffer(GL_UNIFORM_BUFFER, render_ctx.ubos[UBO_WINDOW]);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 5 * sizeof(GLfloat), window_size);

    glUseProgram(render_ctx.programs[PROGRAM_RECT]);
    glBindVertexArray(render_ctx.vaos[VAO_RECT]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_RECT]);
    glBindTexture(GL_TEXTURE_2D, render_ctx.textures[TEXTURE_SPRITES]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, RECT_VERTEX_LENGTH * sizeof(GLfloat), buffer);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 1);
}

void render_rect(vec2 pos, vec2 size, Color color)
{
    Animation animation;
    animation.type = ANIMATION_SOLID_COLOR;
    render_rect_ex(pos, size, color, &animation);
}

static void get_animation_uv(Animation* animation, vec2* uv_offset, vec2* uv_size)
{
    if (animation->type != ANIMATION_SOLID_COLOR)
    {
        AnimationData* data = render_ctx.animations + animation->type;
        uv_offset->x = data->frames[animation->frame].u;
        uv_offset->y = data->frames[animation->frame].v;
        uv_size->x = data->frames[animation->frame].w;
        uv_size->y = data->frames[animation->frame].h;
    }
    else
    {
        uv_offset->x = render_ctx.solid_color.u;
        uv_offset->y = render_ctx.solid_color.v;
        uv_size->x = render_ctx.solid_color.w;
        uv_size->y = render_ctx.solid_color.h;
    }
}

static GLfloat* add_projectile_to_buffer(GLfloat* buffer, Projectile* proj)
{
    vec2 uv_offset, uv_size;
    get_animation_uv(&proj->animation, &uv_offset, &uv_size);
    return add_rect_to_buffer(buffer, (RectVertex) {
                .position  = proj->position,
                .size      = proj->size,
                .uv_offset = uv_offset,
                .uv_size   = uv_size,
                .color     = (Color){1.0f, 1.0f, 1.0f, 1.0f},
                .rotation  = fmod(proj->rotation, 2 * PI)
            });
}

static GLfloat* add_entity_to_buffer(GLfloat* buffer, Entity* entity)
{
    vec2 uv_offset, uv_size;
    get_animation_uv(&entity->animation, &uv_offset, &uv_size);
    return add_rect_to_buffer(buffer, (RectVertex) {
                .position  = entity->position,
                .size      = entity->size,
                .uv_offset = uv_offset,
                .uv_size   = uv_size,
                .color     = (Color){1.0f, 1.0f, 1.0f, 1.0f},
                .rotation  = fmod(entity->rotation, 2 * PI)
            });
}

static GLfloat* add_wall_to_buffer(GLfloat* buffer, Wall* wall)
{
    return add_rect_to_buffer(buffer, (RectVertex) {
                .position  = vec2_create(wall->position.x + wall->size.x / 2, wall->position.y - wall->size.y / 2),
                .size      = wall->size,
                .uv_offset = vec2_create(render_ctx.solid_color.u, render_ctx.solid_color.v),
                .uv_size   = vec2_create(render_ctx.solid_color.w, render_ctx.solid_color.h),
                .color     = (Color){1.0f, 1.0f, 1.0f, 1.0f},
                .rotation  = 0.0f
            });
}

void render_game_objects(void)
{
    Arena* arena;
    GLfloat* buffer = render_ctx.rect_vertex_buffer;

    arena = &global_ctx.projectile_arena;
    for (i32 i = 0; i < arena->size; i++)
        buffer = add_projectile_to_buffer(buffer, arena_at(arena, i));

    arena = &global_ctx.wall_arena;
    for (i32 i = 0; i < arena->size; i++)
        buffer = add_wall_to_buffer(buffer, arena_at(arena, i));

    arena = &global_ctx.entity_arena;
    for (i32 i = 0; i < arena->size; i++)
        buffer = add_entity_to_buffer(buffer, arena_at(arena, i));

    i32 num_entities = (buffer - render_ctx.rect_vertex_buffer) / RECT_VERTEX_LENGTH;
    glUseProgram(render_ctx.programs[PROGRAM_RECT]);
    glBindVertexArray(render_ctx.vaos[VAO_RECT]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_RECT]);
    glBindTexture(GL_TEXTURE_2D, render_ctx.textures[TEXTURE_SPRITES]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (buffer - render_ctx.rect_vertex_buffer) * sizeof(GLfloat), render_ctx.rect_vertex_buffer);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, num_entities);
}

static GLfloat* add_circle_hitbox_to_buffer(GLfloat* buffer, Entity* entity)
{
    buffer[0] = entity->position.x;
    buffer[1] = entity->position.y;
    buffer[2] = entity->hitbox_radius;
    return buffer + CIRCLE_VERTEX_LENGTH;
}

static GLfloat* add_rect_hitbox_to_buffer(GLfloat* buffer, Projectile* proj)
{
    buffer[0] = proj->position.x;
    buffer[1] = proj->position.y;
    buffer[2] = proj->hitbox_size.x;
    buffer[3] = proj->hitbox_size.y;
    buffer[4] = render_ctx.solid_color.u;
    buffer[5] = render_ctx.solid_color.v;
    buffer[6] = render_ctx.solid_color.w;
    buffer[7] = render_ctx.solid_color.h;
    buffer[8] = 1.0f;
    buffer[9] = 1.0f;
    buffer[10] = 1.0f;
    buffer[11] = 1.0f;
    buffer[12] = fmod(proj->rotation, 2 * PI);
    return buffer + RECT_VERTEX_LENGTH;
}

void render_game_object_hitboxes(void)
{
    Arena* arena;
    GLfloat* circle_buffer = render_ctx.circle_vertex_buffer;
    GLfloat* rect_buffer = render_ctx.rect_vertex_buffer;

    arena = &global_ctx.projectile_arena;
    for (i32 i = 0; i < arena->size; i++)
    {
        Projectile* proj = arena_at(arena, i);
        if (flag_test(&proj->flags, FLAG_RECT_HITBOX))
        {
            rect_buffer = add_rect_to_buffer(rect_buffer, (RectVertex) {
                        .position  = proj->position,
                        .size      = proj->hitbox_size,
                        .uv_offset = vec2_create(render_ctx.solid_color.u, render_ctx.solid_color.v),
                        .uv_size   = vec2_create(render_ctx.solid_color.w, render_ctx.solid_color.h),
                        .color     = (Color){1.0f, 0.0f, 0.0f, 1.0f},
                        .rotation  = fmod(proj->rotation, 2 * PI)
                    });
        }
        else
        {
            circle_buffer = add_circle_to_buffer(circle_buffer, (CircleVertex) {
                        .position = proj->position,
                        .radius   = proj->hitbox_radius
                    });
        }
    }

    arena = &global_ctx.entity_arena;
    for (i32 i = 0; i < arena->size; i++)
    {
        Entity* entity = arena_at(arena, i);
        circle_buffer = add_circle_to_buffer(circle_buffer, (CircleVertex) {
                        .position = entity->position,
                        .radius   = entity->hitbox_radius
                    });
    }

    i32 num_circles = (circle_buffer - render_ctx.circle_vertex_buffer) / CIRCLE_VERTEX_LENGTH;
    glUseProgram(render_ctx.programs[PROGRAM_CIRCLE]);
    glBindVertexArray(render_ctx.vaos[VAO_CIRCLE]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_CIRCLE]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (circle_buffer - render_ctx.circle_vertex_buffer) * sizeof(GLfloat), render_ctx.circle_vertex_buffer);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, num_circles);

    i32 num_rects = (rect_buffer - render_ctx.rect_vertex_buffer) / RECT_VERTEX_LENGTH;
    glUseProgram(render_ctx.programs[PROGRAM_RECT]);
    glBindVertexArray(render_ctx.vaos[VAO_RECT]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_RECT]);
    glBindTexture(GL_TEXTURE_2D, render_ctx.textures[TEXTURE_SPRITES]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (rect_buffer - render_ctx.rect_vertex_buffer) * sizeof(GLfloat), render_ctx.rect_vertex_buffer);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, num_rects);
}

void render_cleanup(void)
{
    glDeleteVertexArrays(NUM_VAOS, render_ctx.vaos);
    glDeleteBuffers(NUM_VBOS, render_ctx.vbos);
    glDeleteBuffers(NUM_UBOS, render_ctx.ubos);
    glDeleteTextures(NUM_TEXTURES, render_ctx.textures);
    glDeleteRenderbuffers(1, &render_ctx.rbo);
    glDeleteFramebuffers(1, &render_ctx.fbo);
}

void render()
{
    GLfloat window_size[5] = {RESOLUTION_X, RESOLUTION_Y, global_ctx.camera_center.x - RESOLUTION_X / 2.0f, global_ctx.camera_center.y - RESOLUTION_Y / 2.0f, global_ctx.time};
    glBindBuffer(GL_UNIFORM_BUFFER, render_ctx.ubos[UBO_WINDOW]);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 5 * sizeof(GLfloat), window_size);

    glBindFramebuffer(GL_FRAMEBUFFER, render_ctx.fbo);
    glViewport(0, 0, RESOLUTION_X, RESOLUTION_Y);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    if (global_ctx.draw_entities)
        render_game_objects();
    if (global_ctx.draw_hitboxes)
        render_game_object_hitboxes();

    screen_render();
    ui_draw();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, global_ctx.window_width, global_ctx.window_height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glUseProgram(render_ctx.programs[PROGRAM_SCREEN]);
    glBindVertexArray(render_ctx.vaos[VAO_SCREEN]);
    glBindBuffer(GL_ARRAY_BUFFER, render_ctx.vbos[VBO_SCREEN]);
    glBindTexture(GL_TEXTURE_2D, render_ctx.textures[TEXTURE_FBO]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
