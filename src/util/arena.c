#include "arena.h"
#include <stdlib.h>
#include <string.h>

void arena_init(Arena* arena, size_t instance_size, i32 capacity)
{
    arena->instances = malloc((capacity + 1) * (instance_size + 2 * sizeof(i32)));
    arena->sparse = (i32*)((u8*)arena->instances + (capacity + 1) * instance_size);
    arena->dense = (i32*)((u8*)arena->instances + (capacity + 1) * (instance_size + sizeof(i32)));
    arena->capacity = capacity;
    arena->size = 0;
    arena->instance_size = instance_size;
    for (i32 i = 0; i < capacity; i++)
        arena->dense[i] = i;
}

i32 arena_alloc(Arena* arena)
{
    if (arena->size == arena->capacity)
        return arena->capacity;

    i32 dense_idx = arena->size++;
    i32 sparse_idx = arena->dense[dense_idx];
    arena->sparse[sparse_idx] = dense_idx;
    memset((u8*)arena->instances + sparse_idx * arena->instance_size, 0, arena->instance_size);
    return sparse_idx;
}

void arena_clear(Arena* arena)
{
    arena->size = 0;
}

void* arena_deref(Arena* arena, i32 sparse_idx)
{
    return (u8*)arena->instances + sparse_idx * arena->instance_size;
}

void* arena_at(Arena* arena, i32 dense_idx)
{
    return (u8*)arena->instances + arena->dense[dense_idx] * arena->instance_size;
}

void* arena_dummy(Arena* arena)
{
    void* dummy = (u8*)arena->instances + arena->capacity * arena->instance_size;
    memset(dummy, 0, arena->instance_size);
    return dummy;
}

i32 arena_id(Arena* arena, i32 dense_idx)
{
    return arena->dense[dense_idx];
}

void arena_free(Arena* arena, i32 sparse_idx)
{
    if (sparse_idx == arena->capacity)
        return;

    i32 dense_idx = arena->sparse[sparse_idx];
    i32 dense_idx_swap = arena->size - 1;
    i32 sparse_idx_swap = arena->dense[dense_idx_swap];

    arena->sparse[sparse_idx] = dense_idx_swap;
    arena->sparse[sparse_idx_swap] = dense_idx;
    arena->dense[dense_idx] = sparse_idx_swap;
    arena->dense[dense_idx_swap] = sparse_idx;

    --arena->size;
}
