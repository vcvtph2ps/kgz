#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "kgz_priv.h"

void kgz_arena_init(kgz_arena_t *arena, size_t capacity) {
    if (!arena) return;

    arena->buffer = (uint8_t*)KGZ_MALLOC(capacity);
    if (!arena->buffer) {
        arena->capacity = 0;
        arena->offset = 0;
        return;
    }

    arena->capacity = capacity;
    arena->offset = 0;
}

void kgz_arena_reset(kgz_arena_t *arena) {
    if (!arena) return;
    arena->offset = 0;
}

void *kgz_arena_allocate(kgz_arena_t *arena, size_t size, size_t alignment) {
    if (!arena || !arena->buffer) return nullptr;
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) return nullptr;

    uintptr_t base = (uintptr_t)arena->buffer;
    uintptr_t current = base + arena->offset;

    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t new_offset = (size_t)((aligned - base) + size);

    if (new_offset > arena->capacity) {
        return nullptr;
    }

    void *ptr = (void *)aligned;
    KGZ_MEMSET(ptr, 0, size);

    arena->offset = new_offset;
    return ptr;
}

void kgz_arena_free(kgz_arena_t *arena) {
    if (!arena) return;

    KGZ_FREE(arena->buffer);
    arena->buffer = nullptr;
    arena->capacity = 0;
    arena->offset = 0;
}
