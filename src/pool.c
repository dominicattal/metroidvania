#include "context.h"
#include <stdlib.h>
#include <string.h>

void pool_init(Pool* pool, i32 capacity)
{
    pool->entities = malloc((capacity + 1) * sizeof(Entity));
    pool->sparse = malloc((capacity + 1) * sizeof(i32));
    pool->dense = malloc((capacity + 1) * sizeof(i32));
    pool->capacity = capacity;
    pool->size = 0;

    for (i32 i = 0; i < capacity; i++)
        pool->dense[i] = i;
}

i32 pool_alloc(Pool* pool)
{
    if (pool->size == pool->capacity)
    {
        log(ERROR, "Pool alloc failed");
        return pool->capacity;
    }

    i32 dense_idx = pool->size++;
    i32 sparse_idx = pool->dense[dense_idx];
    pool->sparse[sparse_idx] = dense_idx;
    memset(pool->entities + sparse_idx, 0, sizeof(Entity));
    return sparse_idx;
}

Entity* pool_ref(Pool* pool, i32 sparse_idx)
{
    return pool->entities + sparse_idx;
}

void pool_free(Pool* pool, i32 sparse_idx)
{
    if (sparse_idx == pool->capacity)
        return;

    i32 dense_idx = pool->sparse[sparse_idx];
    i32 dense_idx_swap = pool->size - 1;
    i32 sparse_idx_swap = pool->dense[dense_idx_swap];

    pool->sparse[sparse_idx] = dense_idx_swap;
    pool->sparse[sparse_idx_swap] = dense_idx;
    pool->dense[dense_idx] = sparse_idx_swap;
    pool->dense[dense_idx_swap] = sparse_idx;

    --pool->size;
}

