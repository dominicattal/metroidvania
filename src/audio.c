#include "context.h"
#include <assert.h>
#include <miniaudio.h>

#define MAX_NUM_PLAYING_SOUNDS 64

typedef struct {

    ma_engine engine;

    ma_sound sounds[NUM_SOUNDS];

    Arena sound_arena;

} AudioContext;

extern GlobalContext global_ctx;
AudioContext audio_ctx;

void audio_init(void)
{
    arena_init(&audio_ctx.sound_arena, sizeof(ma_sound), MAX_NUM_PLAYING_SOUNDS);

    ma_result res = ma_engine_init(NULL, &audio_ctx.engine);
    assert(res == MA_SUCCESS);
    res = ma_sound_init_from_file(&audio_ctx.engine, "assets/audio/400 Sounds Pack/Materials/bamboo_drop.wav", 0, NULL, NULL, audio_ctx.sounds + SOUND_TEST);
}

void audio_update(void)
{
    i32 i = 0;
    while (i < audio_ctx.sound_arena.size)
    {
        ma_sound* sound = arena_at(&audio_ctx.sound_arena, i);
        if (!ma_sound_is_playing(sound))
        {
            ma_sound_uninit(sound);
            arena_free(&audio_ctx.sound_arena, arena_id(&audio_ctx.sound_arena, i));
            continue;
        }
        i++;
    }
}

void audio_play(Sound sound)
{
    i32 id = arena_alloc(&audio_ctx.sound_arena);
    if (id == audio_ctx.sound_arena.capacity)
    {
        log(DEBUG, "Too many sounds playing");
        return;
    }

    ma_sound* copied_sound = arena_deref(&audio_ctx.sound_arena, id);
    ma_sound_init_copy(&audio_ctx.engine, audio_ctx.sounds + sound, 0, NULL, copied_sound);
    ma_sound_start(copied_sound);
}
