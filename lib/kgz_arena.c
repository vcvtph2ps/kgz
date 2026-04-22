#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "kgz_priv.h"

void kgz_arena_init(kgz_arena_t* arena, size_t capacity) {
    if(KGZ_UNLIKELY(!arena)) return;

    arena->buffer = (uint8_t*) KGZ_MALLOC(capacity);
    if(KGZ_UNLIKELY(!arena->buffer)) {
        arena->capacity = 0;
        arena->offset = 0;
        return;
    }

    arena->capacity = capacity;
    arena->offset = 0;
}

void kgz_arena_reset(kgz_arena_t* arena) {
    if(KGZ_UNLIKELY(!arena)) return;
    arena->offset = 0;
}

void* kgz_arena_allocate(kgz_arena_t* arena, size_t size, size_t alignment) {
    if(KGZ_UNLIKELY(!arena || !arena->buffer)) return NULL;
    if(KGZ_UNLIKELY(alignment == 0 || (alignment & (alignment - 1)) != 0)) return NULL;

    uintptr_t base = (uintptr_t) arena->buffer;
    uintptr_t current = base + arena->offset;

    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t new_offset = (size_t) ((aligned - base) + size);

    if(KGZ_UNLIKELY(new_offset > arena->capacity)) { return NULL; }

    void* ptr = (void*) aligned;

    arena->offset = new_offset;
    return ptr;
}

void kgz_arena_free(kgz_arena_t* arena) {
    if(KGZ_UNLIKELY(!arena)) return;

    KGZ_FREE(arena->buffer, arena->capacity);
    arena->buffer = NULL;
    arena->capacity = 0;
    arena->offset = 0;
}
