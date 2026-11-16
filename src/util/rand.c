#include <stdlib.h>
#include "type.h"

f32 randf(void)
{
    return (f32)(rand()) / (f32)RAND_MAX;
}

i32 randi_range(i32 low, i32 high)
{
    return rand() % (high-low+1) + low;
}

f32 randf_range(f32 low, f32 high)
{
    return (f32)(rand()) / (f32)RAND_MAX * (high-low) + low;
}

