#ifndef VEC_H
#define VEC_H

#include "type.h"

#define VEC2_ZERO (vec2){0}

typedef struct {
    f64 x, y;
} vec2;

vec2 vec2_create(f64 x, f64 y);
vec2 vec2_add(vec2 v1, vec2 v2);
vec2 vec2_sub(vec2 v1, vec2 v2);
vec2 vec2_normalize(vec2 vec);
vec2 vec2_scale(vec2 vec, f64 scale);
vec2 vec2_scale_to(vec2 vec, f64 scale);
vec2 vec2_direction(f64 rad);
vec2 vec2_rotate(vec2 vec, f64 rad);
vec2 vec2_rotate180(vec2 vec2);
f64  vec2_radians(vec2 vec);
f64  vec2_mag(vec2 vec);
f64  vec2_distance(vec2 v1, vec2 v2);
f64  vec2_dot(vec2 v1, vec2 v2);
f64  vec2_angle_between(vec2 v1, vec2 v2);
bool vec2_equal(vec2 v1, vec2 v2);
void vec2_print(vec2 vec);

#endif
