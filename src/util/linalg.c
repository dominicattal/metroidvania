#include "linalg.h"
#include <math.h>
#include <stdio.h>

vec2 vec2_create(f32 x, f32 y)
{
    return (vec2) { .x = x, .y = y };
}

vec2 vec2_add(vec2 v1, vec2 v2)
{
    return (vec2) { .x = v1.x + v2.x, .y = v1.y + v2.y };
}

vec2 vec2_sub(vec2 v1, vec2 v2)
{
    return (vec2) { .x = v1.x - v2.x, .y = v1.y - v2.y };
}

vec2 vec2_normalize(vec2 vec)
{
    f32 mag = vec2_mag(vec);
    if (mag == 0)
        return vec;
    return (vec2) { .x = vec.x / mag, .y = vec.y / mag };

}

vec2 vec2_scale(vec2 vec, f32 scale)
{
    return (vec2) { .x = vec.x * scale, .y = vec.y * scale };
}

vec2 vec2_scale_to(vec2 vec, f32 scale)
{
    return vec2_scale(vec2_normalize(vec), scale);
}

vec2 vec2_direction(f32 rad)
{
    return (vec2) { .x = cos(rad), .y = sin(rad) };
}

vec2 vec2_rotate(vec2 vec, f32 rad)
{
    return (vec2) { 
        .x = vec.x * cos(rad) - vec.y * sin(rad),
        .y = vec.x * sin(rad) + vec.y * cos(rad)
    };
}

vec2 vec2_rotate180(vec2 vec)
{
    return (vec2) { 
        .x = -vec.x,
        .y = -vec.y
    };
}

f32 vec2_radians(vec2 vec)
{
    return atan2(vec.y, vec.x);
}

f32 vec2_mag(vec2 vec)
{
    return sqrt(vec.x * vec.x + vec.y * vec.y);
}

f32 vec2_distance(vec2 v1, vec2 v2)
{
    f32 dx = v2.x - v1.x;
    f32 dy = v2.y - v1.y;
    return sqrt(dx * dx + dy * dy);
}

f32 vec2_dot(vec2 v1, vec2 v2)
{
    return v1.x * v2.x + v1.y * v2.y;
}

f32 vec2_angle_between(vec2 v1, vec2 v2)
{
    f32 dot = vec2_dot(v1, v2);
    f32 m1 = vec2_mag(v1);
    f32 m2 = vec2_mag(v2);
    return acos(dot / m1 / m2);
}

bool vec2_equal(vec2 v1, vec2 v2)
{
    return v1.x == v2.x && v1.y == v2.y;
}

void vec2_print(vec2 vec)
{
    printf("(%.5f,%.5f)\n", vec.x, vec.y);
}
