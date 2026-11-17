#include "context.h"
#include <miniaudio.h>

typedef struct {

    ma_engine engine;

} AudioContext;

extern GlobalContext global_ctx;
AudioContext audio_ctx;

void audio_init(void)
{
    ma_engine_init(NULL, &audio_ctx.engine);
}

void audio_play(Sound sound)
{
}
