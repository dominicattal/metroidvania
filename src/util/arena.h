#ifndef ARENA_H
#define ARENA_H

#include "type.h"
#include <stddef.h>

typedef struct {
    void* instances;
    i32* dense;
    i32* sparse;
    size_t instance_size;
    i32 size;
    i32 capacity;
} Arena;

// Initialize the arena with room for capacity instances of size instance_size
// Will allocate an extra dummy instance that returns when the Arena is full.
void    arena_init(Arena* arena, size_t instance_size, i32 capacity);

// Allocate an instance in the arena and return its identifier, which is its sprase_idx.
// If the arena is full, then it will return the arena's capacity, which can
// be safetly derefenced
i32     arena_alloc(Arena* arena);

// Set's the arena's size to 0
void    arena_clear(Arena* arena);

// Derefences an identifier returned from arena_alloc.
void*   arena_deref(Arena* arena, i32 id);

// Derefences the idx instance, which is its dense_idx.
// Can be used to iterate through valid instances in the arena.
void*   arena_at(Arena* arena, i32 idx);

// Returns a dummy instance that can be safetly derefenced but is not considered
// in the arena.
void*   arena_dummy(Arena* arena);

// Gets the identifier / sparse_idx from the dense_idx
i32     arena_id(Arena* arena, i32 idx);

// Frees an instance to be allocated from another call to arena_alloc
void    arena_free(Arena* arena, i32 id);

#endif
