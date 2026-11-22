#include "../context.h"

static void update_beam(Beam* beam, f64 dt)
{
    beam->rotation += dt;
}

void screen_screen_2_load(void)
{
    ui_load(UI_GAME);
    player_init(0.0, 0.0);
    wall_create(400.0f, 400.0f, 100.0f, 100.0f);
    Beam* beam = beam_create(0.0, 0.0, 100.0, 40.0, 0.0);
    beam->expire_time = global_ctx.time + 10.0;
    beam->update_callback = update_beam;
}

void screen_screen_2_update(f64 dt)
{
}
