#ifndef TYPE_H
#define TYPE_H

#include <stdint.h>

typedef uint8_t   u8;
typedef int32_t   i32;
typedef int64_t   i64;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef float     f32;
typedef double    f64;

typedef struct {
    f32 r, g, b, a;
} Color;

#endif
